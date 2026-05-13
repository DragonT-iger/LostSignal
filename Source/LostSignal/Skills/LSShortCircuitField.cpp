#include "Skills/LSShortCircuitField.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/LSCharacterSkillRow.h"
#include "Engine/EngineTypes.h"
#include "Engine/StaticMesh.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"
#include "Net/UnrealNetwork.h"
#include "Skills/LSShortCircuitSkill.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	ELSBreakPowerTier ToShortCircuitBreakPowerTier(int32 Value, ELSBreakPowerTier Fallback)
	{
		if (Value >= static_cast<int32>(ELSBreakPowerTier::HardCrowdControl))
		{
			return ELSBreakPowerTier::HardCrowdControl;
		}

		if (Value >= static_cast<int32>(ELSBreakPowerTier::SpecialAttack))
		{
			return ELSBreakPowerTier::SpecialAttack;
		}

		if (Value >= static_cast<int32>(ELSBreakPowerTier::NormalAttack))
		{
			return ELSBreakPowerTier::NormalAttack;
		}

		return Fallback;
	}

	constexpr float DebugSphereMeshBaseDiameter = 100.0f;
	constexpr float DefaultShortCircuitAttackCoefficient = 1.5f;
}

ALSShortCircuitField::ALSShortCircuitField()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	AreaComponent = CreateDefaultSubobject<USphereComponent>(TEXT("AreaComponent"));
	SetRootComponent(AreaComponent);
	AreaComponent->InitSphereRadius(350.0f);
	AreaComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AreaComponent->SetCollisionObjectType(ECC_WorldDynamic);
	AreaComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	DebugFieldSphereMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugFieldSphereMesh"));
	DebugFieldSphereMeshComponent->SetupAttachment(AreaComponent);
	DebugFieldSphereMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugFieldSphereMeshComponent->SetGenerateOverlapEvents(false);
	DebugFieldSphereMeshComponent->SetCastShadow(false);
	DebugFieldSphereMeshComponent->SetHiddenInGame(true);
	DebugFieldSphereMeshComponent->SetVisibility(false, true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		DebugFieldSphereMeshComponent->SetStaticMesh(SphereMeshFinder.Object);
	}
}

void ALSShortCircuitField::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		StartField();
	}

	OnRep_DebugFieldMeshRadius();
}

void ALSShortCircuitField::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALSShortCircuitField, DebugFieldMeshRadius);
}

void ALSShortCircuitField::InitializeField(AActor* InSourceActor, ULSShortCircuitSkill* InSkillDefinition)
{
	SourceActor = InSourceActor;
	SkillDefinition = InSkillDefinition;

	if (HasAuthority() && SkillDefinition && SkillDefinition->bEnableDebugVisualization)
	{
		UE_LOG(LogLS, Log, TEXT("[ShortCircuit] Field initialized: Field=%s Source=%s Skill=%s BegunPlay=%d"),
			*GetNameSafe(this),
			*GetNameSafe(SourceActor),
			*GetNameSafe(SkillDefinition),
			HasActorBegunPlay() ? 1 : 0);
	}

	if (HasAuthority() && HasActorBegunPlay())
	{
		StartField();
	}
}

void ALSShortCircuitField::StartField()
{
	if (bFieldStarted || !SourceActor || !SkillDefinition)
	{
		if (SkillDefinition && SkillDefinition->bEnableDebugVisualization)
		{
			UE_LOG(LogLS, Log, TEXT("[ShortCircuit] Field start skipped: Field=%s AlreadyStarted=%d Source=%s Skill=%s"),
				*GetNameSafe(this),
				bFieldStarted ? 1 : 0,
				*GetNameSafe(SourceActor),
				*GetNameSafe(SkillDefinition));
		}
		return;
	}

	bFieldStarted = true;
	ConfigureFromSkillData();

	if (SkillDefinition->bEnableDebugVisualization)
	{
		const float Radius = AreaComponent ? AreaComponent->GetScaledSphereRadius() : 350.0f;
		UE_LOG(LogLS, Log, TEXT("[ShortCircuit] Field started: Field=%s Location=%s Radius=%.2f Pulses=%d Interval=%.2f"),
			*GetNameSafe(this),
			*GetActorLocation().ToCompactString(),
			Radius,
			PulsesRemaining,
			SkillDefinition->FieldPulseInterval);
	}

	if (SkillDefinition->bEnableDebugVisualization)
	{
		const float Radius = AreaComponent ? AreaComponent->GetScaledSphereRadius() : 350.0f;
		DebugFieldMeshRadius = Radius;
		SetDebugFieldMeshVisible(false, Radius);
	}

	ApplyPulse();

	if (PulsesRemaining > 0)
	{
		const float Interval = FMath::Max(SkillDefinition->FieldPulseInterval, 1.0f);
		GetWorldTimerManager().SetTimer(PulseTimerHandle, this, &ALSShortCircuitField::ApplyPulse, Interval, true);
	}
}

void ALSShortCircuitField::OnRep_DebugFieldMeshRadius()
{
	SetDebugFieldMeshVisible(false, DebugFieldMeshRadius);
}

void ALSShortCircuitField::SetDebugFieldMeshVisible(bool bVisible, float Radius)
{
	if (!DebugFieldSphereMeshComponent)
	{
		return;
	}

	const float SafeRadius = FMath::Max(Radius, 0.0f);
	const float Scale = SafeRadius > 0.0f ? (SafeRadius * 2.0f) / DebugSphereMeshBaseDiameter : 1.0f;
	DebugFieldSphereMeshComponent->SetRelativeLocation(FVector::ZeroVector);
	DebugFieldSphereMeshComponent->SetRelativeScale3D(FVector(Scale));
	DebugFieldSphereMeshComponent->SetHiddenInGame(!bVisible);
	DebugFieldSphereMeshComponent->SetVisibility(bVisible, true);
}

void ALSShortCircuitField::HideDebugFieldMesh()
{
	SetDebugFieldMeshVisible(false, DebugFieldMeshRadius);
}

void ALSShortCircuitField::ConfigureFromSkillData()
{
	float Radius = 350.0f;
	float Duration = SkillDefinition ? SkillDefinition->FieldDuration : 5.0f;
	float Interval = SkillDefinition ? FMath::Max(SkillDefinition->FieldPulseInterval, 0.01f) : 1.0f;
	int32 HitCount = FMath::Max(1, FMath::RoundToInt(Duration / Interval));

	if (SkillDefinition)
	{
		FLSSkillAreaPreviewSpec PreviewSpec = SkillDefinition->BuildPreviewSpec();
		Radius = PreviewSpec.Radius > 0.0f ? PreviewSpec.Radius : Radius;

		FLSCharacterSkillRow Row;
		if (SkillDefinition->TryGetSkillRow(Row))
		{
			HitCount = Row.Skill_HitCount > 0 ? Row.Skill_HitCount : HitCount;
			Interval = Row.Skill_HitRate > 0.0f ? Row.Skill_HitRate : Interval;
			Duration = HitCount * Interval;
		}
	}

	if (AreaComponent)
	{
		AreaComponent->SetSphereRadius(Radius, true);
	}

	PulsesRemaining = HitCount;
	SetLifeSpan(Duration + 0.25f);
}

void ALSShortCircuitField::ApplyPulse()
{
	if (!HasAuthority() || !SourceActor || !SkillDefinition || PulsesRemaining <= 0)
	{
		if (SkillDefinition && SkillDefinition->bEnableDebugVisualization)
		{
			UE_LOG(LogLS, Log, TEXT("[ShortCircuit] Pulse stopped: Field=%s Authority=%d Source=%s Skill=%s PulsesRemaining=%d"),
				*GetNameSafe(this),
				HasAuthority() ? 1 : 0,
				*GetNameSafe(SourceActor),
				*GetNameSafe(SkillDefinition),
				PulsesRemaining);
		}
		GetWorldTimerManager().ClearTimer(PulseTimerHandle);
		return;
	}

	ULSCharacterCombatComponent* SourceCombatComponent = SourceActor->FindComponentByClass<ULSCharacterCombatComponent>();
	if (!SourceCombatComponent || !SkillDefinition->DamageEffectClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[ShortCircuit] Pulse stopped: SourceCombatComponent or DamageEffectClass missing. Field=%s Source=%s Combat=%s DamageEffect=%s"),
			*GetNameSafe(this),
			*GetNameSafe(SourceActor),
			*GetNameSafe(SourceCombatComponent),
			*GetNameSafe(SkillDefinition->DamageEffectClass.Get()));
		GetWorldTimerManager().ClearTimer(PulseTimerHandle);
		return;
	}

	FLSCharacterSkillRow Row;
	const bool bHasRow = SkillDefinition->TryGetSkillRow(Row);
	const float FallbackAttackCoefficient = SkillDefinition->AttackCoefficient > 0.0f
		? SkillDefinition->AttackCoefficient
		: DefaultShortCircuitAttackCoefficient;
	const float AttackCoefficient = bHasRow && Row.Skill_Multiplier > 0.0f ? Row.Skill_Multiplier : FallbackAttackCoefficient;
	const ELSBreakPowerTier BreakPower = bHasRow ? ToShortCircuitBreakPowerTier(Row.Skill_Impact, SkillDefinition->BreakPower) : SkillDefinition->BreakPower;
	const float Radius = AreaComponent ? AreaComponent->GetScaledSphereRadius() : 350.0f;
	if (!bHasRow || Row.Skill_Multiplier <= 0.0f || SkillDefinition->AttackCoefficient <= 0.0f)
	{
		UE_LOG(
			LogLS,
			Log,
			TEXT("[ShortCircuit] Pulse coefficient fallback check. Field=%s Skill=%s HasRow=%d RowMultiplier=%.2f AssetFallbackCoef=%.2f ResolvedCoef=%.2f Fixed=%.2f"),
			*GetNameSafe(this),
			*GetNameSafe(SkillDefinition),
			bHasRow ? 1 : 0,
			bHasRow ? Row.Skill_Multiplier : 0.0f,
			SkillDefinition->AttackCoefficient,
			AttackCoefficient,
			SkillDefinition->FixedDamage);
	}

	if (SkillDefinition->bEnableDebugVisualization)
	{
		MulticastBlinkDebugFieldMesh(
			Radius,
			FMath::Min(FMath::Max(SkillDefinition->FieldPulseInterval * 0.35f, 0.08f), 0.35f));
	}

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(SourceActor);
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		GetActorLocation(),
		Radius,
		ObjectTypes,
		nullptr,
		ActorsToIgnore,
		OverlappedActors);

	if (SkillDefinition->bEnableDebugVisualization)
	{
		UE_LOG(LogLS, Log, TEXT("[ShortCircuit] Pulse overlap: Field=%s Radius=%.2f RawTargets=%d PulsesRemaining=%d"),
			*GetNameSafe(this),
			Radius,
			OverlappedActors.Num(),
			PulsesRemaining);
	}

	TSet<AActor*> UniqueTargets;
	for (AActor* TargetActor : OverlappedActors)
	{
		if (!TargetActor || UniqueTargets.Contains(TargetActor))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		const float BeforeHealth = TargetASC
			? TargetASC->GetNumericAttribute(ULSCombatAttributeSet::GetCurrentHealthAttribute())
			: 0.0f;

		if (SourceCombatComponent->ApplyDamageEffectToTarget(
			TargetActor,
			SkillDefinition->DamageEffectClass,
			1.0f,
			SkillDefinition->FixedDamage,
			AttackCoefficient,
			SkillDefinition->bCanCrit,
			BreakPower))
		{
			const float AfterHealth = TargetASC
				? TargetASC->GetNumericAttribute(ULSCombatAttributeSet::GetCurrentHealthAttribute())
				: 0.0f;
			UE_LOG(
				LogLS,
				Log,
				TEXT("[ShortCircuit] Pulse Damaged: Actor=%s | Fixed=%.2f Coef=%.2f BreakPower=%d | HP %.2f -> %.2f (ActualDamage %.2f)"),
				*GetNameSafe(TargetActor),
				SkillDefinition->FixedDamage,
				AttackCoefficient,
				static_cast<int32>(BreakPower),
				BeforeHealth,
				AfterHealth,
				BeforeHealth - AfterHealth);
			UniqueTargets.Add(TargetActor);
			ApplySlowEffect(TargetActor);
		}
	}

	--PulsesRemaining;
	if (PulsesRemaining <= 0)
	{
		GetWorldTimerManager().ClearTimer(PulseTimerHandle);
		Destroy();
	}
}

void ALSShortCircuitField::ApplySlowEffect(AActor* TargetActor) const
{
	if (!SkillDefinition || !SkillDefinition->SlowEffectClass || !SourceActor || !TargetActor)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor);
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(SkillDefinition->SlowEffectClass, 1.0f, EffectContext);
	if (SpecHandle.IsValid())
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void ALSShortCircuitField::MulticastBlinkDebugFieldMesh_Implementation(float Radius, float VisibleSeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	DebugFieldMeshRadius = Radius;
	SetDebugFieldMeshVisible(true, Radius);
	World->GetTimerManager().ClearTimer(DebugFieldMeshBlinkTimerHandle);
	World->GetTimerManager().SetTimer(
		DebugFieldMeshBlinkTimerHandle,
		this,
		&ALSShortCircuitField::HideDebugFieldMesh,
		FMath::Max(VisibleSeconds, 0.01f),
		false);
}
