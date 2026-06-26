#include "GAS/Abilities/Character1/LSGA_Overclock.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Data/LSCharacterSkillRow.h"
#include "Engine/EngineTypes.h"
#include "GAS/Effects/LSGE_PlayerBasicDamage.h"
#include "GAS/LSGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "Skills/LSSkillDataAsset.h"

namespace
{
	ELSBreakPowerTier ToOverclockAbilityBreakPowerTier(int32 Value, ELSBreakPowerTier Fallback)
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

ULSGA_Overclock::ULSGA_Overclock()
{
	DamageEffectClass = ULSGE_PlayerBasicDamage::StaticClass();

	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Stunned);
	ActivationBlockedTags.AddTag(LSGameplayTags::Combat_SkillCasting); // 스킬끼리만 차단 (기본공격은 통과)
	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_Attacking);      // 공통 "진행 중" 의미 유지
	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_SkillCasting);   // 시전 중 표식
	CancelAbilitiesWithTag.AddTag(LSGameplayTags::Ability_PlayerBasicAttack); // 기본공격 모션 캔슬 후 발동

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void ULSGA_Overclock::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* SourceActor = GetAvatarActorFromActorInfo();
	ULSPlayerSkillComponent* SkillComponent = SourceActor ? SourceActor->FindComponentByClass<ULSPlayerSkillComponent>() : nullptr;
	if (!SourceActor || !SourceActor->HasAuthority() || !SkillComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FLSSkillActivationContext SkillContext;
	if (!SkillComponent->ConsumePendingAbilityContext(GetClass(), SkillContext) || !SkillContext.SkillData)
	{
		UE_LOG(LogLS, Warning, TEXT("%s Overclock ability missing pending skill context."), *GetNameSafe(SourceActor));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = SkillContext.SkillData->DamageEffectClass ? SkillContext.SkillData->DamageEffectClass : DamageEffectClass;
	ULSCharacterCombatComponent* SourceCombatComponent = SourceActor->FindComponentByClass<ULSCharacterCombatComponent>();
	if (!SourceCombatComponent || !ResolvedDamageEffectClass)
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

	const FLSCharacterSkillRow* Row = SkillContext.bHasSkillRow ? &SkillContext.SkillRow : nullptr;
	const float Range = Row && Row->Range_X > 0.0f ? Row->Range_X : FallbackRange;
	const float ConeDegrees = Row && Row->Range_Y > 0.0f ? Row->Range_Y : FallbackConeDegrees;
	const float BaseAttackCoefficient = Row && Row->Skill_Multiplier > 0.0f ? Row->Skill_Multiplier : FallbackAttackCoefficient;
	const float AdditionalCoefficientPerStack = Row && Row->Res_Multiplier > 0.0f ? Row->Res_Multiplier : FallbackAdditionalAttackCoefficientPerStack;
	const ELSBreakPowerTier ResolvedBreakPower = Row ? ToOverclockAbilityBreakPowerTier(Row->Skill_Impact, BreakPower) : BreakPower;

	const FVector SourceLocation = SourceActor->GetActorLocation();
	FVector AimDirection = SkillContext.TargetLocation - SourceLocation;
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

	if (AimDirection.IsNearlyZero() || Range <= 0.0f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(SourceActor);
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		SourceActor->GetWorld(),
		SourceLocation,
		Range,
		ObjectTypes,
		nullptr,
		ActorsToIgnore,
		OverlappedActors);

	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(ConeDegrees * 0.5f));
	TArray<AActor*> ConeTargets;
	for (AActor* TargetActor : OverlappedActors)
	{
		if (!TargetActor || ConeTargets.Contains(TargetActor))
		{
			continue;
		}

		FVector ToTarget = TargetActor->GetActorLocation() - SourceLocation;
		ToTarget.Z = 0.0f;
		if (ToTarget.IsNearlyZero() || FVector::DotProduct(AimDirection, ToTarget.GetSafeNormal()) >= CosHalfAngle)
		{
			ConeTargets.Add(TargetActor);
		}
	}

	if (ConeTargets.Num() == 0)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	const int32 ConsumedStacks = ConsumeCombatAccelerationStacks(SourceActor);
	const float FinalAttackCoefficient = BaseAttackCoefficient + (AdditionalCoefficientPerStack * ConsumedStacks);

	int32 ValidHitCount = 0;
	for (AActor* TargetActor : ConeTargets)
	{
		if (SourceCombatComponent->ApplyDamageEffectToTarget(
			TargetActor,
			ResolvedDamageEffectClass,
			1.0f,
			0.0f,
			FinalAttackCoefficient,
			bCanCrit,
			ResolvedBreakPower))
		{
			++ValidHitCount;

			// 스킬 row에 정의된 상태이상을 명중 대상/자신에게 적용한다(서버 권위).
			if (Row)
			{
				SourceCombatComponent->ApplyStatusEffectFromRow(Row->Status_ID, Row->Effect_Target, Row->Skill_Effect_Duration, TargetActor);
				SourceCombatComponent->ApplyStatusEffectFromRow(Row->Status_ID_2, Row->Effect_Target_2, Row->Skill_Effect_Duration_2, TargetActor);
			}
		}
	}

	if (bEnableDebugLog)
	{
		UE_LOG(
			LogLS,
			Log,
			TEXT("[GA_Overclock] Source=%s Targets=%d ValidHits=%d ConsumedStacks=%d Coef=%.2f"),
			*GetNameSafe(SourceActor),
			ConeTargets.Num(),
			ValidHitCount,
			ConsumedStacks,
			FinalAttackCoefficient);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

int32 ULSGA_Overclock::ConsumeCombatAccelerationStacks(AActor* SourceActor) const
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
