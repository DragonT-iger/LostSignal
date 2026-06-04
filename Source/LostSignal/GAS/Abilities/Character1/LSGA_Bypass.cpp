#include "GAS/Abilities/Character1/LSGA_Bypass.h"

#include "AbilitySystemComponent.h"
#include "Characters/LSEnemyCharacter.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Combat/LSPlayerCombatComponent.h"
#include "Data/LSCharacterSkillRow.h"
#include "Data/LSGameDataSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineTypes.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GAS/LSGameplayTags.h"
#include "GameplayEffect.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"
#include "Skills/LSBypassHologramActor.h"
#include "Skills/LSBypassSkillDataAsset.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "Skills/LSSkillDataAsset.h"
#include "TimerManager.h"

namespace
{
	const FLSCharacterSkillRow* ResolveBypassSkillRow(const UObject* WorldContextObject, const ULSSkillDataAsset* SkillData, const TCHAR* Context)
	{
		const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
		return GameDataSubsystem && SkillData ? GameDataSubsystem->FindActiveSkillRowByID(SkillData->GetSkillID(), Context) : nullptr;
	}
}

ULSGA_Bypass::ULSGA_Bypass()
{
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Stunned);
	ActivationBlockedTags.AddTag(LSGameplayTags::Combat_Attacking);
	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_Attacking);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void ULSGA_Bypass::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	AActor* SourceActor = SourceCharacter;
	ULSPlayerSkillComponent* SkillComponent = SourceActor ? SourceActor->FindComponentByClass<ULSPlayerSkillComponent>() : nullptr;
	if (!SourceActor || !SourceActor->HasAuthority() || !SourceCharacter || !SkillComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FLSSkillActivationContext SkillContext;
	if (!SkillComponent->ConsumePendingAbilityContext(GetClass(), SkillContext) || !SkillContext.SkillData)
	{
		UE_LOG(LogLS, Warning, TEXT("%s Bypass ability missing pending skill context."), *GetNameSafe(SourceActor));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	ActiveSkillData = SkillContext.SkillData;

	float Distance = 0.0f;
	float Duration = 0.0f;
	if (!ResolveMovementParams(SkillContext.SkillData, SkillContext.bHasSkillRow ? &SkillContext.SkillRow : nullptr, Distance, Duration))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector AimDirection = SkillContext.TargetLocation - SourceActor->GetActorLocation();
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = FRotator(0.0f, SkillContext.AimYaw, 0.0f).Vector();
	}

	AimDirection = AimDirection.GetSafeNormal2D();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = SourceActor->GetActorForwardVector().GetSafeNormal2D();
	}

	UCharacterMovementComponent* MovementComponent = SourceCharacter->GetCharacterMovement();
	if (AimDirection.IsNearlyZero() || !MovementComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	SkillComponent->ApplySkillCooldown(SkillContext.SkillData);

	const FVector StartLocation = SourceActor->GetActorLocation();
	SetInvincibleTagActive(true);
	ApplyBypassStartEffects(Duration);
	ApplySpoofingStartEffects(StartLocation);
	IgnoreEnemiesForBypass(SourceCharacter, StartLocation, AimDirection, Distance);

	const float SlideSpeed = Distance / Duration;
	TSharedPtr<FRootMotionSource_ConstantForce> RootMotion = MakeShared<FRootMotionSource_ConstantForce>();
	RootMotion->InstanceName = FName("Bypass");
	RootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	RootMotion->Priority = 5;
	RootMotion->Force = AimDirection * SlideSpeed;
	RootMotion->Duration = Duration;
	RootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	RootMotionSourceID = MovementComponent->ApplyRootMotionSource(RootMotion);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BypassTimerHandle, this, &ULSGA_Bypass::FinishBypass, Duration, false);
	}

	if (bEnableDebugLog)
	{
		UE_LOG(
			LogLS,
			Log,
			TEXT("[GA_Bypass] Source=%s Direction=%s Distance=%.2f Duration=%.2f Speed=%.2f"),
			*GetNameSafe(SourceActor),
			*AimDirection.ToCompactString(),
			Distance,
			Duration,
			SlideSpeed);
	}
}

bool ULSGA_Bypass::ResolveMovementParams(const ULSSkillDataAsset* SkillData, const FLSCharacterSkillRow* SkillRow, float& OutDistance, float& OutDuration) const
{
	const ULSBypassSkillDataAsset* BypassData = Cast<ULSBypassSkillDataAsset>(SkillData);
	OutDistance = SkillRow && SkillRow->Range_X > 0.0f
		? SkillRow->Range_X
		: BypassData ? BypassData->FallbackDistance : FallbackDistance;
	OutDuration = SkillRow && SkillRow->Skill_Time > 0.0f
		? SkillRow->Skill_Time
		: BypassData ? BypassData->FallbackDuration : FallbackDuration;
	return OutDistance > 0.0f && OutDuration > 0.0f;
}

void ULSGA_Bypass::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BypassTimerHandle);
	}

	if (ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		ClearIgnoredEnemiesForBypass(SourceCharacter);

		if (UCharacterMovementComponent* MovementComponent = SourceCharacter->GetCharacterMovement())
		{
			MovementComponent->RemoveRootMotionSourceByID(RootMotionSourceID);
		}
	}

	SetInvincibleTagActive(false);
	RootMotionSourceID = 0;
	IgnoredEnemyActors.Reset();
	ActiveSkillData = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULSGA_Bypass::FinishBypass()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void ULSGA_Bypass::ApplyBypassStartEffects(float Duration)
{
	AActor* SourceActor = GetAvatarActorFromActorInfo();
	const ULSBypassSkillDataAsset* BypassData = Cast<ULSBypassSkillDataAsset>(ActiveSkillData);
	if (!SourceActor || !SourceActor->HasAuthority() || !BypassData || !BypassData->bSetComboIndexOverrideOnFinish)
	{
		return;
	}

	ULSPlayerCombatComponent* PlayerCombatComponent = SourceActor->FindComponentByClass<ULSPlayerCombatComponent>();
	if (!PlayerCombatComponent)
	{
		return;
	}

	PlayerCombatComponent->SetPendingBasicAttackComboIndexOverride(
		BypassData->ComboIndexOverride,
		Duration + BypassData->ComboIndexOverrideWindowSeconds);
}

void ULSGA_Bypass::ApplySpoofingStartEffects(const FVector& HologramLocation)
{
	ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	ULSBypassSkillDataAsset* BypassData = Cast<ULSBypassSkillDataAsset>(ActiveSkillData);
	if (!SourceCharacter || !SourceCharacter->HasAuthority() || !BypassData)
	{
		return;
	}

	if (BypassData->bSpawnHologramOnStart && BypassData->HologramActorClass)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = SourceCharacter;
		SpawnParameters.Instigator = SourceCharacter;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const FTransform SpawnTransform(SourceCharacter->GetActorRotation(), HologramLocation);
		if (ALSBypassHologramActor* HologramActor = SourceCharacter->GetWorld()->SpawnActor<ALSBypassHologramActor>(
			BypassData->HologramActorClass,
			SpawnTransform,
			SpawnParameters))
		{
			HologramActor->InitializeFromCharacter(SourceCharacter, BypassData->HologramMaterial, BypassData->HologramLifeSeconds);
		}
	}

	if (BypassData->bEnableSpoofingDebugLog && BypassData->PullRadius > 0.0f)
	{
		DrawDebugCircle(
			SourceCharacter->GetWorld(),
			HologramLocation,
			BypassData->PullRadius,
			64,
			FColor::Cyan,
			false,
			BypassData->HologramLifeSeconds,
			0,
			2.0f,
			FVector::ForwardVector,
			FVector::RightVector,
			false);
	}

	if (BypassData->bPullTargetsToHologram)
	{
		if (UWorld* World = GetWorld())
		{
			TWeakObjectPtr<AActor> WeakSourceActor(SourceCharacter);
			TWeakObjectPtr<ULSBypassSkillDataAsset> WeakBypassData(BypassData);
			FTimerDelegate TimerDelegate = FTimerDelegate::CreateLambda([WeakSourceActor, WeakBypassData, HologramLocation]()
			{
				ULSGA_Bypass::PullTargetsToHologram(WeakSourceActor.Get(), HologramLocation, WeakBypassData.Get());
			});

			const float PullDelay = FMath::Max(0.01f, BypassData->HologramLifeSeconds);
			World->GetTimerManager().SetTimer(BypassSpoofingTimerHandle, TimerDelegate, PullDelay, false);

			if (BypassData->bEnableSpoofingDebugLog)
			{
				UE_LOG(LogLS, Log, TEXT("[GA_Bypass] Spoofing pull timer scheduled. Source=%s Hologram=%s Delay=%.2f"),
					*GetNameSafe(SourceCharacter),
					*HologramLocation.ToCompactString(),
					PullDelay);
			}
		}
	}
}

void ULSGA_Bypass::IgnoreEnemiesForBypass(ACharacter* SourceCharacter, const FVector& StartLocation, const FVector& Direction, float Distance)
{
	if (!SourceCharacter || !SourceCharacter->HasAuthority() || Distance <= 0.0f || Direction.IsNearlyZero())
	{
		return;
	}

	UCapsuleComponent* SourceCapsule = SourceCharacter->GetCapsuleComponent();
	if (!SourceCapsule)
	{
		return;
	}

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(SourceCharacter);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	const float PathHalfLength = Distance * 0.5f;
	const float PathHalfWidth = SourceCapsule->GetScaledCapsuleRadius() * 2.0f;
	const FVector AreaCenter = StartLocation + (Direction * PathHalfLength);
	const float QueryRadius = FMath::Sqrt(FMath::Square(PathHalfLength) + FMath::Square(PathHalfWidth));

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		AreaCenter,
		QueryRadius,
		ObjectTypes,
		ALSEnemyCharacter::StaticClass(),
		ActorsToIgnore,
		OverlappedActors);

	const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal2D();
	for (AActor* EnemyActor : OverlappedActors)
	{
		if (!EnemyActor)
		{
			continue;
		}

		const FVector ToEnemy = EnemyActor->GetActorLocation() - StartLocation;
		const float ForwardDistance = FVector::DotProduct(ToEnemy, Direction);
		const float LateralDistance = FMath::Abs(FVector::DotProduct(ToEnemy, RightDirection));
		if (ForwardDistance < 0.0f || ForwardDistance > Distance || LateralDistance > PathHalfWidth)
		{
			continue;
		}

		SourceCapsule->IgnoreActorWhenMoving(EnemyActor, true);
		SourceCharacter->MoveIgnoreActorAdd(EnemyActor);

		if (ACharacter* EnemyCharacter = Cast<ACharacter>(EnemyActor))
		{
			if (UCapsuleComponent* EnemyCapsule = EnemyCharacter->GetCapsuleComponent())
			{
				EnemyCapsule->IgnoreActorWhenMoving(SourceCharacter, true);
			}
			EnemyCharacter->MoveIgnoreActorAdd(SourceCharacter);
		}

		IgnoredEnemyActors.AddUnique(EnemyActor);
	}
}

void ULSGA_Bypass::ClearIgnoredEnemiesForBypass(ACharacter* SourceCharacter)
{
	if (!SourceCharacter)
	{
		IgnoredEnemyActors.Reset();
		return;
	}

	if (UCapsuleComponent* SourceCapsule = SourceCharacter->GetCapsuleComponent())
	{
		for (const TWeakObjectPtr<AActor>& IgnoredActor : IgnoredEnemyActors)
		{
			if (AActor* EnemyActor = IgnoredActor.Get())
			{
				SourceCapsule->IgnoreActorWhenMoving(EnemyActor, false);
			}
		}
	}

	for (const TWeakObjectPtr<AActor>& IgnoredActor : IgnoredEnemyActors)
	{
		AActor* EnemyActor = IgnoredActor.Get();
		if (!EnemyActor)
		{
			continue;
		}

		SourceCharacter->MoveIgnoreActorRemove(EnemyActor);
		if (ACharacter* EnemyCharacter = Cast<ACharacter>(EnemyActor))
		{
			if (UCapsuleComponent* EnemyCapsule = EnemyCharacter->GetCapsuleComponent())
			{
				EnemyCapsule->IgnoreActorWhenMoving(SourceCharacter, false);
			}
			EnemyCharacter->MoveIgnoreActorRemove(SourceCharacter);
		}
	}

	IgnoredEnemyActors.Reset();
}

void ULSGA_Bypass::PullTargetsToHologram(AActor* SourceActor, FVector HologramLocation, ULSBypassSkillDataAsset* BypassData)
{
	if (!SourceActor || !SourceActor->HasAuthority() || !BypassData)
	{
		if (BypassData && BypassData->bEnableSpoofingDebugLog)
		{
			UE_LOG(LogLS, Warning, TEXT("[GA_Bypass] Spoofing pull skipped. Source=%s HasAuthority=%d Data=%s"),
				*GetNameSafe(SourceActor),
				SourceActor && SourceActor->HasAuthority() ? 1 : 0,
				*GetNameSafe(BypassData));
		}
		return;
	}

	const FLSCharacterSkillRow* Row = ResolveBypassSkillRow(SourceActor, BypassData, TEXT("PullTargetsToHologram"));
	const bool bRowHasCCType = Row && Row->CC_Type != ELSCharacterSkillCrowdControlType::None;
	if (bRowHasCCType && Row->CC_Type != ELSCharacterSkillCrowdControlType::Pull)
	{
		return;
	}

	const float PullRadius = Row && Row->Range_X > 0.0f ? Row->Range_X : BypassData->PullRadius;
	if (PullRadius <= 0.0f)
	{
		return;
	}

	const float PullSpeed = Row && Row->CC_Value > 0.0f ? Row->CC_Value : BypassData->PullSpeed;
	const float PullDuration = Row && Row->Skill_Time > 0.0f ? Row->Skill_Time : BypassData->PullDuration;
	const ELSBreakPowerTier PullBreakPower = Row
		? static_cast<ELSBreakPowerTier>(FMath::Clamp(
			Row->Skill_Impact,
			static_cast<int32>(ELSBreakPowerTier::NormalAttack),
			static_cast<int32>(ELSBreakPowerTier::HardCrowdControl)))
		: BypassData->PullBreakPower;

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(SourceActor);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		SourceActor->GetWorld(),
		HologramLocation,
		PullRadius,
		ObjectTypes,
		ALSEnemyCharacter::StaticClass(),
		ActorsToIgnore,
		OverlappedActors);

	int32 PulledCount = 0;
	for (AActor* TargetActor : OverlappedActors)
	{
		ULSCharacterCombatComponent* TargetCombatComponent = TargetActor ? TargetActor->FindComponentByClass<ULSCharacterCombatComponent>() : nullptr;
		if (!TargetCombatComponent || !TargetCombatComponent->CanApplyCrowdControl(PullBreakPower))
		{
			continue;
		}

		FVector PullDirection = HologramLocation - TargetActor->GetActorLocation();
		PullDirection.Z = 0.0f;
		if (TargetCombatComponent->ApplyKnockback(PullDirection, PullSpeed, PullDuration, BypassData->PullUpSpeed))
		{
			++PulledCount;
			ScheduleSpoofingStun(TargetActor, BypassData, PullDuration);
		}
	}

	if (BypassData->bEnableSpoofingDebugLog)
	{
		UE_LOG(LogLS, Log, TEXT("[GA_Bypass] Spoofing Source=%s Hologram=%s Radius=%.2f RawTargets=%d Pulled=%d"),
			*GetNameSafe(SourceActor),
			*HologramLocation.ToCompactString(),
			PullRadius,
			OverlappedActors.Num(),
			PulledCount);
	}
}

void ULSGA_Bypass::ScheduleSpoofingStun(AActor* TargetActor, const ULSBypassSkillDataAsset* BypassData, float DelaySeconds)
{
	if (!TargetActor || !BypassData || !BypassData->StunEffectClass)
	{
		return;
	}

	UWorld* World = TargetActor->GetWorld();
	if (!World)
	{
		return;
	}

	TWeakObjectPtr<AActor> WeakTarget(TargetActor);
	TWeakObjectPtr<const ULSBypassSkillDataAsset> WeakBypassData(BypassData);
	FTimerDelegate TimerDelegate = FTimerDelegate::CreateLambda([WeakTarget, WeakBypassData]()
	{
		ULSGA_Bypass::ApplySpoofingStunIfConfigured(WeakTarget.Get(), WeakBypassData.Get());
	});

	FTimerHandle StunTimerHandle;
	World->GetTimerManager().SetTimer(StunTimerHandle, TimerDelegate, FMath::Max(0.01f, DelaySeconds), false);
}

void ULSGA_Bypass::ApplySpoofingStunIfConfigured(AActor* TargetActor, const ULSBypassSkillDataAsset* BypassData)
{
	ULSCharacterCombatComponent* TargetCombatComponent = TargetActor ? TargetActor->FindComponentByClass<ULSCharacterCombatComponent>() : nullptr;
	UAbilitySystemComponent* TargetASC = TargetCombatComponent ? TargetCombatComponent->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC || !BypassData || !BypassData->StunEffectClass)
	{
		if (BypassData && BypassData->bEnableSpoofingDebugLog)
		{
			UE_LOG(LogLS, Warning, TEXT("[GA_Bypass] Spoofing stun skipped. Target=%s ASC=%s StunGE=%s"),
				*GetNameSafe(TargetActor),
				*GetNameSafe(TargetASC),
				*GetNameSafe(BypassData ? BypassData->StunEffectClass.Get() : nullptr));
		}
		return;
	}

	FGameplayEffectContextHandle EffectContext = TargetASC->MakeEffectContext();
	EffectContext.AddSourceObject(BypassData);

	const FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(BypassData->StunEffectClass, 1.0f, EffectContext);
	if (!SpecHandle.IsValid())
	{
		if (BypassData->bEnableSpoofingDebugLog)
		{
			UE_LOG(LogLS, Warning, TEXT("[GA_Bypass] Spoofing stun spec failed. Target=%s StunGE=%s"),
				*GetNameSafe(TargetActor),
				*GetNameSafe(BypassData->StunEffectClass.Get()));
		}
		return;
	}

	if (BypassData->StunDuration > 0.0f)
	{
		SpecHandle.Data->SetDuration(BypassData->StunDuration, true);
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	if (BypassData->bEnableSpoofingDebugLog)
	{
		UE_LOG(LogLS, Log, TEXT("[GA_Bypass] Spoofing stun applied. Target=%s StunGE=%s Duration=%.2f HasTag=%d"),
			*GetNameSafe(TargetActor),
			*GetNameSafe(BypassData->StunEffectClass.Get()),
			BypassData->StunDuration,
			TargetASC->HasMatchingGameplayTag(LSGameplayTags::State_Stunned) ? 1 : 0);
	}
}

void ULSGA_Bypass::SetInvincibleTagActive(bool bActive)
{
	if (bInvincibleTagActive == bActive)
	{
		return;
	}

	if (AActor* SourceActor = GetAvatarActorFromActorInfo())
	{
		if (ULSCharacterCombatComponent* CombatComponent = SourceActor->FindComponentByClass<ULSCharacterCombatComponent>())
		{
			CombatComponent->SetCombatTagActive(LSGameplayTags::State_Invincible, bActive);
			bInvincibleTagActive = bActive;
		}
	}
}
