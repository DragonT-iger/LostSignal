#include "GAS/Abilities/LSGA_Execution.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Characters/LSEnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Data/LSCharacterSkillRow.h"
#include "Engine/EngineTypes.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GAS/Effects/LSGE_PlayerBasicDamage.h"
#include "GAS/LSGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"
#include "Skills/LSExecutionSkillDataAsset.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "Skills/LSShortCircuitField.h"
#include "Skills/LSSkillDataAsset.h"
#include "TimerManager.h"

namespace
{
	ELSBreakPowerTier ToExecutionBreakPowerTier(int32 Value, ELSBreakPowerTier Fallback)
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
}

ULSGA_Execution::ULSGA_Execution()
{
	DamageEffectClass = ULSGE_PlayerBasicDamage::StaticClass();

	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(LSGameplayTags::Combat_Attacking);
	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_Attacking);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

bool ULSGA_Execution::ResolveMovementParams(const ULSSkillDataAsset* InSkillData, float& OutDistance, float& OutDuration) const
{
	const ULSExecutionSkillDataAsset* InExecutionData = Cast<ULSExecutionSkillDataAsset>(InSkillData);

	FLSCharacterSkillRow Row;
	const bool bHasRow = InSkillData && InSkillData->TryGetSkillRow(Row);
	OutDistance = bHasRow && Row.Range_X > 0.0f
		? Row.Range_X
		: InExecutionData ? InExecutionData->FallbackDashDistance : 650.0f;
	OutDuration = bHasRow && Row.Skill_Time > 0.0f
		? Row.Skill_Time
		: InExecutionData ? InExecutionData->FallbackDashDuration : 0.25f;

	return OutDistance > 0.0f && OutDuration > 0.0f;
}

void ULSGA_Execution::ActivateAbility(
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
		UE_LOG(LogLS, Warning, TEXT("%s Execution ability missing pending skill context."), *GetNameSafe(SourceActor));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SkillData = SkillContext.SkillData;
	ExecutionData = Cast<ULSExecutionSkillDataAsset>(SkillContext.SkillData);

	ULSCharacterCombatComponent* SourceCombatComponent = SourceActor->FindComponentByClass<ULSCharacterCombatComponent>();
	const TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = SkillContext.SkillData->DamageEffectClass
		? SkillContext.SkillData->DamageEffectClass
		: DamageEffectClass;
	if (!SourceCombatComponent || !ResolvedDamageEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FLSCharacterSkillRow Row;
	const bool bHasRow = SkillContext.SkillData->TryGetSkillRow(Row);
	float DashDuration = 0.0f;
	ResolveMovementParams(SkillContext.SkillData, CachedDashDistance, DashDuration);
	CachedSlashWidth = bHasRow && Row.Range_Y > 0.0f
		? Row.Range_Y
		: ExecutionData ? ExecutionData->FallbackSlashWidth : 220.0f;
	const float BaseAttackCoefficient = bHasRow && Row.Skill_Multiplier > 0.0f
		? Row.Skill_Multiplier
		: SkillContext.SkillData->AttackCoefficient > 0.0f ? SkillContext.SkillData->AttackCoefficient : FallbackAttackCoefficient;

	if (CachedDashDistance <= 0.0f || DashDuration <= 0.0f || CachedSlashWidth <= 0.0f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedDirection = SkillContext.TargetLocation - SourceActor->GetActorLocation();
	CachedDirection.Z = 0.0f;
	if (CachedDirection.IsNearlyZero())
	{
		CachedDirection = FRotator(0.0f, SkillContext.AimYaw, 0.0f).Vector();
	}

	CachedDirection = CachedDirection.GetSafeNormal2D();
	if (CachedDirection.IsNearlyZero())
	{
		CachedDirection = SourceActor->GetActorForwardVector().GetSafeNormal2D();
	}

	UCharacterMovementComponent* MovementComponent = SourceCharacter->GetCharacterMovement();
	if (CachedDirection.IsNearlyZero() || !MovementComponent)
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

	CachedStartLocation = SourceActor->GetActorLocation();
	CachedConsumedAccelerationStacks = ConsumeCombatAccelerationStacks(SourceActor);
	const float DataAssetAdditionalCoefficient = ExecutionData ? ExecutionData->AdditionalAttackCoefficientPerAccelerationStack : 0.25f;
	const float AdditionalCoefficient = bHasRow && Row.Skill_Count_Multiplier > 0.0f ? Row.Skill_Count_Multiplier : DataAssetAdditionalCoefficient;
	CachedAttackCoefficient = BaseAttackCoefficient + (AdditionalCoefficient * CachedConsumedAccelerationStacks);
	IgnoreEnemiesForDash(SourceCharacter);

	TSharedPtr<FRootMotionSource_ConstantForce> RootMotion = MakeShared<FRootMotionSource_ConstantForce>();
	RootMotion->InstanceName = FName("ExecutionDash");
	RootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	RootMotion->Priority = 9;
	RootMotion->Force = CachedDirection * (CachedDashDistance / DashDuration);
	RootMotion->Duration = DashDuration;
	RootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
	RootMotionSourceID = MovementComponent->ApplyRootMotionSource(RootMotion);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SheathHitTimerHandle, this, &ULSGA_Execution::PerformSheathHit, DashDuration, false);
	}
}

void ULSGA_Execution::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SheathHitTimerHandle);
	}

	if (ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		ClearIgnoredEnemiesForDash(SourceCharacter);

		if (UCharacterMovementComponent* MovementComponent = SourceCharacter->GetCharacterMovement())
		{
			MovementComponent->RemoveRootMotionSourceByID(RootMotionSourceID);
		}
	}

	RootMotionSourceID = 0;
	ExecutionData = nullptr;
	SkillData = nullptr;
	CachedConsumedAccelerationStacks = 0;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULSGA_Execution::PerformSheathHit()
{
	AActor* SourceActor = GetAvatarActorFromActorInfo();
	ULSCharacterCombatComponent* SourceCombatComponent = SourceActor ? SourceActor->FindComponentByClass<ULSCharacterCombatComponent>() : nullptr;
	if (!SourceActor || !SourceActor->HasAuthority() || !SkillData || !SourceCombatComponent)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	FLSCharacterSkillRow Row;
	const bool bHasRow = SkillData->TryGetSkillRow(Row);
	const ELSBreakPowerTier ResolvedBreakPower = bHasRow
		? ToExecutionBreakPowerTier(Row.Skill_Impact, SkillData->BreakPower)
		: SkillData->BreakPower;
	const TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = SkillData->DamageEffectClass
		? SkillData->DamageEffectClass
		: DamageEffectClass;

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(SourceActor);
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	const FVector AreaCenter = CachedStartLocation + (CachedDirection * (CachedDashDistance * 0.5f));
	const float QueryRadius = FMath::Sqrt(FMath::Square(CachedDashDistance * 0.5f) + FMath::Square(CachedSlashWidth * 0.5f));
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		AreaCenter,
		QueryRadius,
		ObjectTypes,
		nullptr,
		ActorsToIgnore,
		OverlappedActors);

	int32 ValidHitCount = 0;
	TSet<AActor*> UniqueTargets;
	for (AActor* TargetActor : OverlappedActors)
	{
		if (!TargetActor || UniqueTargets.Contains(TargetActor) || !IsPointInExecutionArea(TargetActor->GetActorLocation()))
		{
			continue;
		}

		if (SourceCombatComponent->ApplyDamageEffectToTarget(
			TargetActor,
			ResolvedDamageEffectClass,
			1.0f,
			SkillData->FixedDamage,
			CachedAttackCoefficient,
			SkillData->bCanCrit,
			ResolvedBreakPower))
		{
			UniqueTargets.Add(TargetActor);
			++ValidHitCount;
		}
	}

	int32 ExplodedFieldCount = 0;
	UWorld* World = GetWorld();
	if (World)
	{
		for (TActorIterator<ALSShortCircuitField> It(World); It; ++It)
		{
			ALSShortCircuitField* Field = *It;
			if (!Field || !IsPointInExecutionArea(Field->GetActorLocation()))
			{
				continue;
			}

			const float FieldExplosionCoefficient = ExecutionData && ExecutionData->FieldExplosionAttackCoefficient > 0.0f
				? ExecutionData->FieldExplosionAttackCoefficient
				: CachedAttackCoefficient;
			const float FieldExplosionFixedDamage = ExecutionData ? ExecutionData->FieldExplosionFixedDamage : SkillData->FixedDamage;
			const float FieldRadiusOverride = ExecutionData ? ExecutionData->FieldExplosionRadiusOverride : 0.0f;
			const bool bDestroyField = !ExecutionData || ExecutionData->bDestroyShortCircuitFieldOnExplosion;

			if (Field->ExplodeByExecution(SourceActor, SkillData, FieldExplosionFixedDamage, FieldExplosionCoefficient, FieldRadiusOverride, bDestroyField))
			{
				++ExplodedFieldCount;
			}
		}
	}

	if (ExecutionData && ExecutionData->bEnableDebugLog)
	{
		UE_LOG(
			LogLS,
			Log,
			TEXT("[GA_Execution] Source=%s ValidHits=%d ConsumedAcceleration=%d Coef=%.2f ExplodedFields=%d"),
			*GetNameSafe(SourceActor),
			ValidHitCount,
			CachedConsumedAccelerationStacks,
			CachedAttackCoefficient,
			ExplodedFieldCount);
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

int32 ULSGA_Execution::ConsumeCombatAccelerationStacks(AActor* SourceActor) const
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor);
	if (!ASC)
	{
		return 0;
	}

	FGameplayTagContainer BuffTags;
	BuffTags.AddTag(LSGameplayTags::Buff_CombatAcceleration);

	int32 StackCount = 0;
	for (const FActiveGameplayEffectHandle& Handle : ASC->GetActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAllOwningTags(BuffTags)))
	{
		StackCount += FMath::Max(0, ASC->GetCurrentStackCount(Handle));
	}

	if (StackCount > 0)
	{
		ASC->RemoveActiveEffectsWithGrantedTags(BuffTags);
	}

	return StackCount;
}

bool ULSGA_Execution::IsPointInExecutionArea(const FVector& Point) const
{
	const FVector ToPoint = Point - CachedStartLocation;
	const float ForwardDistance = FVector::DotProduct(ToPoint, CachedDirection);
	if (ForwardDistance < 0.0f || ForwardDistance > CachedDashDistance)
	{
		return false;
	}

	const FVector Right = FVector::CrossProduct(FVector::UpVector, CachedDirection).GetSafeNormal2D();
	const float LateralDistance = FMath::Abs(FVector::DotProduct(ToPoint, Right));
	return LateralDistance <= (CachedSlashWidth * 0.5f);
}

void ULSGA_Execution::IgnoreEnemiesForDash(ACharacter* SourceCharacter)
{
	if (!SourceCharacter || !SourceCharacter->HasAuthority())
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

	const FVector AreaCenter = CachedStartLocation + (CachedDirection * (CachedDashDistance * 0.5f));
	const float QueryRadius = FMath::Sqrt(FMath::Square(CachedDashDistance * 0.5f) + FMath::Square(CachedSlashWidth * 0.5f));
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		AreaCenter,
		QueryRadius,
		ObjectTypes,
		ALSEnemyCharacter::StaticClass(),
		ActorsToIgnore,
		OverlappedActors);

	for (AActor* EnemyActor : OverlappedActors)
	{
		if (!EnemyActor || !IsPointInExecutionArea(EnemyActor->GetActorLocation()))
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

void ULSGA_Execution::ClearIgnoredEnemiesForDash(ACharacter* SourceCharacter)
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
