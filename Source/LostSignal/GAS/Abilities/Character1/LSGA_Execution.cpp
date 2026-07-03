#include "GAS/Abilities/Character1/LSGA_Execution.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Characters/LSCharacterBase.h"
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
}

bool ULSGA_Execution::ResolveMovementParams(const ULSSkillDataAsset* InSkillData, const FLSCharacterSkillRow* SkillRow, float& OutDistance, float& OutDuration) const
{
	const ULSExecutionSkillDataAsset* InExecutionData = Cast<ULSExecutionSkillDataAsset>(InSkillData);

	OutDistance = SkillRow && SkillRow->Range_X > 0.0f
		? SkillRow->Range_X
		: InExecutionData ? InExecutionData->FallbackDashDistance : 650.0f;
	// 대시 Duration은 DataAsset이 단일 출처. (DataTable Skill_Time은 시전시간 전용 — 다구간 스킬이라 몽타주 전체 스케일에도 쓰지 않는다)
	OutDuration = InExecutionData ? InExecutionData->FallbackDashDuration : 0.25f;

	return OutDistance > 0.0f && OutDuration > 0.0f;
}

bool ULSGA_Execution::PrepareSkillExecution()
{
	ACharacter* SourceCharacter = Cast<ACharacter>(GetSkillSourceActor());
	const FLSSkillActivationContext& SkillCtx = GetSkillContext();
	if (!SourceCharacter || !SkillCtx.SkillData)
	{
		return false;
	}

	SkillData = SkillCtx.SkillData;
	ExecutionData = Cast<ULSExecutionSkillDataAsset>(SkillCtx.SkillData);

	ULSCharacterCombatComponent* SourceCombatComponent = SourceCharacter->FindComponentByClass<ULSCharacterCombatComponent>();
	const TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = SkillCtx.SkillData->DamageEffectClass
		? SkillCtx.SkillData->DamageEffectClass
		: DamageEffectClass;
	if (!SourceCombatComponent || !ResolvedDamageEffectClass)
	{
		return false;
	}

	const FLSCharacterSkillRow* Row = SkillCtx.bHasSkillRow ? &SkillCtx.SkillRow : nullptr;
	ResolveMovementParams(SkillCtx.SkillData, Row, CachedDashDistance, CachedDashDuration);
	CachedSlashWidth = Row && Row->Range_Y > 0.0f
		? Row->Range_Y
		: ExecutionData ? ExecutionData->FallbackSlashWidth : 220.0f;
	if (CachedDashDistance <= 0.0f || CachedDashDuration <= 0.0f || CachedSlashWidth <= 0.0f)
	{
		return false;
	}

	CachedDirection = SkillCtx.TargetLocation - SourceCharacter->GetActorLocation();
	CachedDirection.Z = 0.0f;
	if (CachedDirection.IsNearlyZero())
	{
		CachedDirection = FRotator(0.0f, SkillCtx.AimYaw, 0.0f).Vector();
	}

	CachedDirection = CachedDirection.GetSafeNormal2D();
	if (CachedDirection.IsNearlyZero())
	{
		CachedDirection = SourceCharacter->GetActorForwardVector().GetSafeNormal2D();
	}

	CachedSkillRow = Row ? *Row : FLSCharacterSkillRow();
	bHasCachedSkillRow = Row != nullptr;
	return !CachedDirection.IsNearlyZero() && SourceCharacter->GetCharacterMovement() != nullptr;
}

void ULSGA_Execution::OnSkillStarted()
{
	ACharacter* SourceCharacter = Cast<ACharacter>(GetSkillSourceActor());
	UCharacterMovementComponent* MovementComponent = SourceCharacter ? SourceCharacter->GetCharacterMovement() : nullptr;
	if (!SourceCharacter || !MovementComponent)
	{
		return;
	}

	CachedStartLocation = SourceCharacter->GetActorLocation();
	CachedConsumedAccelerationStacks = ConsumeCombatAccelerationStacks(SourceCharacter);
	const float BaseAttackCoefficient = bHasCachedSkillRow && CachedSkillRow.Skill_Multiplier > 0.0f
		? CachedSkillRow.Skill_Multiplier
		: FallbackAttackCoefficient;
	const float DataAssetAdditionalCoefficient = ExecutionData ? ExecutionData->AdditionalAttackCoefficientPerAccelerationStack : 0.25f;
	const float AdditionalCoefficient = bHasCachedSkillRow && CachedSkillRow.Res_Multiplier > 0.0f
		? CachedSkillRow.Res_Multiplier
		: DataAssetAdditionalCoefficient;
	CachedAttackCoefficient = BaseAttackCoefficient + (AdditionalCoefficient * CachedConsumedAccelerationStacks);
	IgnoreEnemiesForDash(SourceCharacter);

	TSharedPtr<FRootMotionSource_ConstantForce> RootMotion = MakeShared<FRootMotionSource_ConstantForce>();
	RootMotion->InstanceName = FName("ExecutionDash");
	RootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	RootMotion->Priority = 9;
	RootMotion->Force = CachedDirection * (CachedDashDistance / CachedDashDuration);
	RootMotion->Duration = CachedDashDuration;
	RootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
	RootMotionSourceID = MovementComponent->ApplyRootMotionSource(RootMotion);

	// 대시 단계 종료 권위: 서버 타이머. 몽타주 끝이 아니다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(PhaseTimerHandle, this, &ULSGA_Execution::HandleDashFinished, CachedDashDuration, false);
	}
}

float ULSGA_Execution::GetSkillMontagePlayRate() const
{
	// Dash 섹션만 DashDuration에 자동 스케일. 섹션 미분할 몽타주면 1.0(오써링 속도 그대로).
	return ComputeMontagePlayRateForDuration(GetSkillMontage(), DashSectionName, CachedDashDuration);
}

void ULSGA_Execution::OnSkillMontagePlaying()
{
	// 몽타주를 처음부터 연속 재생하되 Dash→Slash 링크를 명시적으로 세팅한다.
	// 이렇게 하면 대시 끝에서 재생을 끊지 않고 자연스럽게 발도로 흐르고, 경계에선 playRate만 바꾸면 된다(팝 없음).
	UAnimMontage* Montage = GetSkillMontage();
	ALSCharacterBase* Character = Cast<ALSCharacterBase>(GetSkillSourceActor());
	if (!Character || !Montage)
	{
		return;
	}

	if (Montage->GetSectionIndex(DashSectionName) != INDEX_NONE && Montage->GetSectionIndex(SlashSectionName) != INDEX_NONE)
	{
		Character->MulticastSetLSMontageNextSection(Montage, DashSectionName, SlashSectionName);
	}
}

void ULSGA_Execution::HandleDashFinished()
{
	if (!IsActive())
	{
		return;
	}

	UAnimMontage* Montage = GetSkillMontage();
	ALSCharacterBase* Character = Cast<ALSCharacterBase>(GetSkillSourceActor());
	const int32 SlashSectionIndex = Montage ? Montage->GetSectionIndex(SlashSectionName) : INDEX_NONE;
	const float SlashSectionLength = SlashSectionIndex != INDEX_NONE ? Montage->GetSectionLength(SlashSectionIndex) : 0.0f;
	if (!Character || !Montage || SlashSectionLength <= 0.0f)
	{
		// 몽타주/발도 섹션이 없으면 기존 즉발과 동일하게 대시 끝에 타격 후 종료한다.
		FinishExecution();
		return;
	}

	// 재생을 끊지 않고 playRate만 1.0으로 복구한다. 몽타주는 Dash→Slash 링크를 따라 이미 발도로 흐르는 중(팝 없음).
	Character->MulticastSetLSMontagePlayRate(Montage, 1.0f);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(PhaseTimerHandle, this, &ULSGA_Execution::FinishExecution, SlashSectionLength, false);
	}
}

void ULSGA_Execution::FinishExecution()
{
	if (!IsActive())
	{
		return;
	}

	// 노티파이 누락/몽타주 없음 폴백: 종료 전에 타격 1회를 보장한다(중복은 베이스 가드가 차단).
	TriggerSkillEffectOnce();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
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
		World->GetTimerManager().ClearTimer(PhaseTimerHandle);
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
	CachedDashDuration = 0.0f;
	CachedConsumedAccelerationStacks = 0;
	CachedSkillRow = FLSCharacterSkillRow();
	bHasCachedSkillRow = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULSGA_Execution::ExecuteSkillEffect()
{
	AActor* SourceActor = GetSkillSourceActor();
	ULSCharacterCombatComponent* SourceCombatComponent = SourceActor ? SourceActor->FindComponentByClass<ULSCharacterCombatComponent>() : nullptr;
	if (!SourceActor || !SourceActor->HasAuthority() || !SkillData || !SourceCombatComponent)
	{
		return;
	}

	const ELSBreakPowerTier ResolvedBreakPower = bHasCachedSkillRow
		? ToExecutionBreakPowerTier(CachedSkillRow.Skill_Impact, FallbackBreakPower)
		: FallbackBreakPower;
	const TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = SkillData->DamageEffectClass
		? SkillData->DamageEffectClass
		: DamageEffectClass;

	const int32 ValidHitCount = ApplySheathDamage(SourceActor, SourceCombatComponent, ResolvedDamageEffectClass, ResolvedBreakPower);
	const int32 ExplodedFieldCount = ExplodeShortCircuitFields(SourceActor, ResolvedBreakPower);

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
}

int32 ULSGA_Execution::ApplySheathDamage(AActor* SourceActor, ULSCharacterCombatComponent* SourceCombatComponent, TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass, ELSBreakPowerTier ResolvedBreakPower) const
{
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
			0.0f,
			CachedAttackCoefficient,
			bCanCrit,
			ResolvedBreakPower))
		{
			UniqueTargets.Add(TargetActor);
			++ValidHitCount;

			// 스킬 row에 정의된 상태이상을 명중 대상/자신에게 적용한다(서버 권위).
			if (bHasCachedSkillRow)
			{
				SourceCombatComponent->ApplyStatusEffectFromRow(CachedSkillRow.Status_ID, CachedSkillRow.Effect_Target, CachedSkillRow.Skill_Effect_Duration, TargetActor);
				SourceCombatComponent->ApplyStatusEffectFromRow(CachedSkillRow.Status_ID_2, CachedSkillRow.Effect_Target_2, CachedSkillRow.Skill_Effect_Duration_2, TargetActor);
			}
		}
	}

	return ValidHitCount;
}

int32 ULSGA_Execution::ExplodeShortCircuitFields(AActor* SourceActor, ELSBreakPowerTier ResolvedBreakPower) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	int32 ExplodedFieldCount = 0;
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
		const float FieldRadiusOverride = ExecutionData ? ExecutionData->FieldExplosionRadiusOverride : 0.0f;
		const bool bDestroyField = !ExecutionData || ExecutionData->bDestroyShortCircuitFieldOnExplosion;

		if (Field->ExplodeByExecution(SourceActor, SkillData, FieldExplosionCoefficient, bCanCrit, ResolvedBreakPower, FieldRadiusOverride, bDestroyField))
		{
			++ExplodedFieldCount;
		}
	}

	return ExplodedFieldCount;
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
