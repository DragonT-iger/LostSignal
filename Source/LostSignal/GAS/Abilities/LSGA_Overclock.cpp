#include "GAS/Abilities/LSGA_Overclock.h"

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
	ActivationBlockedTags.AddTag(LSGameplayTags::Combat_Attacking);
	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_Attacking);

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

	ULSCharacterCombatComponent* SourceCombatComponent = SourceActor->FindComponentByClass<ULSCharacterCombatComponent>();
	if (!SourceCombatComponent || !DamageEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FLSCharacterSkillRow Row;
	const bool bHasRow = SkillContext.SkillData->TryGetSkillRow(Row);
	const float Range = bHasRow && Row.Range_X > 0.0f ? Row.Range_X : FallbackRange;
	const float ConeDegrees = bHasRow && Row.Range_Y > 0.0f ? Row.Range_Y : FallbackConeDegrees;
	const float BaseAttackCoefficient = bHasRow && Row.Skill_Multiplier > 0.0f ? Row.Skill_Multiplier : FallbackAttackCoefficient;
	const float AdditionalCoefficientPerStack = bHasRow && Row.Skill_Count_Multiplier > 0.0f ? Row.Skill_Count_Multiplier : FallbackAdditionalAttackCoefficientPerStack;
	const ELSBreakPowerTier ResolvedBreakPower = bHasRow ? ToOverclockAbilityBreakPowerTier(Row.Skill_Impact, BreakPower) : BreakPower;

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
			DamageEffectClass,
			1.0f,
			FixedDamage,
			FinalAttackCoefficient,
			bCanCrit,
			ResolvedBreakPower))
		{
			++ValidHitCount;
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
