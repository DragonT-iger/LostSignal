#include "Skills/LSShortCircuitField.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Components/SphereComponent.h"
#include "Data/LSCharacterSkillRow.h"
#include "Data/LSGameDataSubsystem.h"
#include "Engine/EngineTypes.h"
#include "Engine/GameInstance.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Skills/LSShortCircuitSkillDataAsset.h"
#include "Skills/LSSkillDataAsset.h"
#include "TimerManager.h"

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

	constexpr float DefaultShortCircuitAttackCoefficient = 1.5f;
	constexpr bool bDefaultShortCircuitCanCrit = false;

	const FName ShortCircuitFieldRadiusParameterName(TEXT("User.FieldRadius"));
	const FName ShortCircuitFieldDurationParameterName(TEXT("User.FieldDuration"));
	const FName ShortCircuitPulseIntervalParameterName(TEXT("User.PulseInterval"));

	const FLSCharacterSkillRow* ResolveShortCircuitSkillRow(const UObject* WorldContextObject, const ULSSkillDataAsset* SkillData, const TCHAR* Context)
	{
		const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
		return GameDataSubsystem && SkillData ? GameDataSubsystem->FindActiveSkillRowByID(SkillData->GetSkillID(), Context) : nullptr;
	}
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

	FieldNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FieldNiagara"));
	FieldNiagaraComponent->SetupAttachment(AreaComponent);
	FieldNiagaraComponent->SetAutoActivate(true);
	FieldNiagaraComponent->SetRelativeLocation(FVector::ZeroVector);
}

void ALSShortCircuitField::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		StartField();
	}

	OnRep_FieldVisualParams();
}

void ALSShortCircuitField::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALSShortCircuitField, FieldRadius);
	DOREPLIFETIME(ALSShortCircuitField, FieldDurationSeconds);
	DOREPLIFETIME(ALSShortCircuitField, FieldPulseIntervalSeconds);
}

void ALSShortCircuitField::InitializeField(AActor* InSourceActor, ULSShortCircuitSkillDataAsset* InSkillData)
{
	SourceActor = InSourceActor;
	SkillData = InSkillData;

	if (HasAuthority() && SkillData && SkillData->bEnableDebugVisualization)
	{
		UE_LOG(LogLS, Log, TEXT("[ShortCircuit] Field initialized: Field=%s Source=%s Skill=%s BegunPlay=%d"),
			*GetNameSafe(this),
			*GetNameSafe(SourceActor),
			*GetNameSafe(SkillData),
			HasActorBegunPlay() ? 1 : 0);
	}

	if (HasAuthority() && HasActorBegunPlay())
	{
		StartField();
	}
}

bool ALSShortCircuitField::ExplodeByExecution(
	AActor* InstigatorActor,
	const ULSSkillDataAsset* ExecutionSkillData,
	float AttackCoefficient,
	bool bCanCrit,
	ELSBreakPowerTier BreakPower,
	float RadiusOverride,
	bool bDestroyAfterExplosion)
{
	if (!HasAuthority() || !InstigatorActor || !ExecutionSkillData || !ExecutionSkillData->DamageEffectClass)
	{
		return false;
	}

	ULSCharacterCombatComponent* SourceCombatComponent = InstigatorActor->FindComponentByClass<ULSCharacterCombatComponent>();
	if (!SourceCombatComponent)
	{
		return false;
	}

	const float Radius = RadiusOverride > 0.0f
		? RadiusOverride
		: AreaComponent ? AreaComponent->GetScaledSphereRadius() : 350.0f;
	if (Radius <= 0.0f)
	{
		return false;
	}

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(InstigatorActor);
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

	int32 ValidHitCount = 0;
	TSet<AActor*> UniqueTargets;
	for (AActor* TargetActor : OverlappedActors)
	{
		if (!TargetActor || UniqueTargets.Contains(TargetActor))
		{
			continue;
		}

		if (SourceCombatComponent->ApplyDamageEffectToTarget(
			TargetActor,
			ExecutionSkillData->DamageEffectClass,
			1.0f,
			0.0f,
			AttackCoefficient,
			bCanCrit,
			BreakPower))
		{
			UniqueTargets.Add(TargetActor);
			++ValidHitCount;
		}
	}

	UE_LOG(
		LogLS,
		Log,
		TEXT("[ShortCircuit] Field exploded by Execution. Field=%s Source=%s Radius=%.2f RawTargets=%d ValidHits=%d Coef=%.2f"),
		*GetNameSafe(this),
		*GetNameSafe(InstigatorActor),
		Radius,
		OverlappedActors.Num(),
		ValidHitCount,
		AttackCoefficient);

	MulticastPlayExplosionEffect(GetActorLocation(), Radius);

	if (bDestroyAfterExplosion)
	{
		GetWorldTimerManager().ClearTimer(PulseTimerHandle);
		Destroy();
	}

	return ValidHitCount > 0;
}

void ALSShortCircuitField::StartField()
{
	if (bFieldStarted || !SourceActor || !SkillData)
	{
		if (SkillData && SkillData->bEnableDebugVisualization)
		{
			UE_LOG(LogLS, Log, TEXT("[ShortCircuit] Field start skipped: Field=%s AlreadyStarted=%d Source=%s Skill=%s"),
				*GetNameSafe(this),
				bFieldStarted ? 1 : 0,
				*GetNameSafe(SourceActor),
				*GetNameSafe(SkillData));
		}
		return;
	}

	bFieldStarted = true;
	ConfigureFromSkillData();

	if (SkillData->bEnableDebugVisualization)
	{
		const float Radius = AreaComponent ? AreaComponent->GetScaledSphereRadius() : 350.0f;
		UE_LOG(LogLS, Log, TEXT("[ShortCircuit] Field started: Field=%s Location=%s Radius=%.2f Pulses=%d Interval=%.2f"),
			*GetNameSafe(this),
			*GetActorLocation().ToCompactString(),
			Radius,
			PulsesRemaining,
			FieldPulseIntervalSeconds);
	}

	ApplyPulse();

	if (PulsesRemaining > 0)
	{
		const float Interval = FMath::Max(FieldPulseIntervalSeconds, 1.0f);
		GetWorldTimerManager().SetTimer(PulseTimerHandle, this, &ALSShortCircuitField::ApplyPulse, Interval, true);
	}
}

void ALSShortCircuitField::OnRep_FieldVisualParams()
{
	ApplyFieldRadius(FieldRadius);
}

void ALSShortCircuitField::ApplyFieldRadius(float Radius)
{
	FieldRadius = FMath::Max(Radius, 0.0f);

	if (AreaComponent && FieldRadius > 0.0f)
	{
		AreaComponent->SetSphereRadius(FieldRadius, true);
	}

	ApplyFieldNiagaraParameters();
}

void ALSShortCircuitField::ApplyFieldNiagaraParameters()
{
	if (!FieldNiagaraComponent)
	{
		return;
	}

	FieldNiagaraComponent->SetFloatParameter(ShortCircuitFieldRadiusParameterName, FieldRadius);
	FieldNiagaraComponent->SetFloatParameter(
		ShortCircuitFieldDurationParameterName,
		FieldDurationSeconds);
	FieldNiagaraComponent->SetFloatParameter(
		ShortCircuitPulseIntervalParameterName,
		FieldPulseIntervalSeconds);
}

void ALSShortCircuitField::ConfigureFromSkillData()
{
	float Radius = 350.0f;
	float Duration = SkillData ? SkillData->FieldDuration : 5.0f;
	float Interval = SkillData ? FMath::Max(SkillData->FieldPulseInterval, 0.01f) : 1.0f;
	int32 HitCount = FMath::Max(1, FMath::RoundToInt(Duration / Interval));

	if (SkillData)
	{
		if (const FLSCharacterSkillRow* Row = ResolveShortCircuitSkillRow(this, SkillData, TEXT("ShortCircuitField.ConfigureFromSkillData")))
		{
			Radius = Row->Range_X > 0.0f ? Row->Range_X : Radius;
			HitCount = Row->Skill_HitCount > 0 ? Row->Skill_HitCount : HitCount;
			Interval = Row->Skill_HitRate > 0.0f ? Row->Skill_HitRate : Interval;
			Duration = HitCount * Interval;
		}
		else
		{
			const FLSSkillAreaPreviewSpec PreviewSpec = SkillData->BuildPreviewSpec();
			Radius = PreviewSpec.Radius > 0.0f ? PreviewSpec.Radius : Radius;
		}
	}

	FieldDurationSeconds = Duration;
	FieldPulseIntervalSeconds = Interval;
	ApplyFieldRadius(Radius);
	PulsesRemaining = HitCount;
	SetLifeSpan(Duration + 0.25f);
}

void ALSShortCircuitField::ApplyPulse()
{
	if (!HasAuthority() || !SourceActor || !SkillData || PulsesRemaining <= 0)
	{
		if (SkillData && SkillData->bEnableDebugVisualization)
		{
			UE_LOG(LogLS, Log, TEXT("[ShortCircuit] Pulse stopped: Field=%s Authority=%d Source=%s Skill=%s PulsesRemaining=%d"),
				*GetNameSafe(this),
				HasAuthority() ? 1 : 0,
				*GetNameSafe(SourceActor),
				*GetNameSafe(SkillData),
				PulsesRemaining);
		}
		GetWorldTimerManager().ClearTimer(PulseTimerHandle);
		return;
	}

	ULSCharacterCombatComponent* SourceCombatComponent = SourceActor->FindComponentByClass<ULSCharacterCombatComponent>();
	if (!SourceCombatComponent || !SkillData->DamageEffectClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[ShortCircuit] Pulse stopped: SourceCombatComponent or DamageEffectClass missing. Field=%s Source=%s Combat=%s DamageEffect=%s"),
			*GetNameSafe(this),
			*GetNameSafe(SourceActor),
			*GetNameSafe(SourceCombatComponent),
			*GetNameSafe(SkillData->DamageEffectClass.Get()));
		GetWorldTimerManager().ClearTimer(PulseTimerHandle);
		return;
	}

	const FLSCharacterSkillRow* Row = ResolveShortCircuitSkillRow(this, SkillData, TEXT("ShortCircuitField.ApplyPulse"));
	const float AttackCoefficient = Row && Row->Skill_Multiplier > 0.0f ? Row->Skill_Multiplier : DefaultShortCircuitAttackCoefficient;
	const ELSBreakPowerTier BreakPower = Row ? ToShortCircuitBreakPowerTier(Row->Skill_Impact, ELSBreakPowerTier::NormalAttack) : ELSBreakPowerTier::NormalAttack;
	const float Radius = AreaComponent ? AreaComponent->GetScaledSphereRadius() : 350.0f;
	if (!Row || Row->Skill_Multiplier <= 0.0f)
	{
		UE_LOG(
			LogLS,
			Log,
			TEXT("[ShortCircuit] Pulse coefficient fallback check. Field=%s Skill=%s HasRow=%d RowMultiplier=%.2f ResolvedCoef=%.2f"),
			*GetNameSafe(this),
			*GetNameSafe(SkillData),
			Row ? 1 : 0,
			Row ? Row->Skill_Multiplier : 0.0f,
			AttackCoefficient);
	}

	MulticastPlayPulseEffect(Radius);

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

	if (SkillData->bEnableDebugVisualization)
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
			SkillData->DamageEffectClass,
			1.0f,
			0.0f,
			AttackCoefficient,
			bDefaultShortCircuitCanCrit,
			BreakPower))
		{
			const float AfterHealth = TargetASC
				? TargetASC->GetNumericAttribute(ULSCombatAttributeSet::GetCurrentHealthAttribute())
				: 0.0f;
			UE_LOG(
				LogLS,
				Log,
				TEXT("[ShortCircuit] Pulse Damaged: Actor=%s | Coef=%.2f BreakPower=%d | HP %.2f -> %.2f (ActualDamage %.2f)"),
				*GetNameSafe(TargetActor),
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
	if (!SkillData || !SkillData->SlowEffectClass || !SourceActor || !TargetActor)
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

	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(SkillData->SlowEffectClass, 1.0f, EffectContext);
	if (SpecHandle.IsValid())
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void ALSShortCircuitField::MulticastPlayPulseEffect_Implementation(float Radius)
{
	UWorld* World = GetWorld();
	if (!World || !PulseNiagaraSystem)
	{
		return;
	}

	UNiagaraComponent* PulseComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		PulseNiagaraSystem,
		GetActorLocation(),
		GetActorRotation());

	if (PulseComponent)
	{
		PulseComponent->SetFloatParameter(ShortCircuitFieldRadiusParameterName, FMath::Max(Radius, 0.0f));
		PulseComponent->SetFloatParameter(ShortCircuitFieldDurationParameterName, FieldDurationSeconds);
		PulseComponent->SetFloatParameter(ShortCircuitPulseIntervalParameterName, FieldPulseIntervalSeconds);
	}
}

void ALSShortCircuitField::MulticastPlayExplosionEffect_Implementation(FVector_NetQuantize EffectLocation, float Radius)
{
	UWorld* World = GetWorld();
	if (!World || !ExplosionNiagaraSystem)
	{
		return;
	}

	UNiagaraComponent* ExplosionComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		ExplosionNiagaraSystem,
		FVector(EffectLocation),
		GetActorRotation());

	if (ExplosionComponent)
	{
		ExplosionComponent->SetFloatParameter(ShortCircuitFieldRadiusParameterName, FMath::Max(Radius, 0.0f));
		ExplosionComponent->SetFloatParameter(ShortCircuitFieldDurationParameterName, FieldDurationSeconds);
		ExplosionComponent->SetFloatParameter(ShortCircuitPulseIntervalParameterName, FieldPulseIntervalSeconds);
	}
}
