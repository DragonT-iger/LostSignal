#include "GAS/Abilities/LSGA_Override.h"

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
	ELSBreakPowerTier ToOverrideAbilityBreakPowerTier(int32 Value, ELSBreakPowerTier Fallback)
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

ULSGA_Override::ULSGA_Override()
{
	DamageEffectClass = ULSGE_PlayerBasicDamage::StaticClass();

	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(LSGameplayTags::Combat_Attacking);
	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_Attacking);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void ULSGA_Override::ActivateAbility(
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
		UE_LOG(LogLS, Warning, TEXT("%s Override ability missing pending skill context."), *GetNameSafe(SourceActor));
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

	FLSCharacterSkillRow Row;
	const bool bHasRow = SkillContext.SkillData->TryGetSkillRow(Row);
	const float Radius = bHasRow && Row.Range_X > 0.0f ? Row.Range_X : FallbackRadius;
	const float DataAssetAttackCoefficient = SkillContext.SkillData->AttackCoefficient > 0.0f ? SkillContext.SkillData->AttackCoefficient : FallbackAttackCoefficient;
	const float ResolvedAttackCoefficient = bHasRow && Row.Skill_Multiplier > 0.0f ? Row.Skill_Multiplier : DataAssetAttackCoefficient;
	const ELSBreakPowerTier ResolvedBreakPower = bHasRow ? ToOverrideAbilityBreakPowerTier(Row.Skill_Impact, SkillContext.SkillData->BreakPower) : SkillContext.SkillData->BreakPower;
	const float ResolvedKnockbackDuration = bHasRow && Row.Skill_Time > 0.0f ? Row.Skill_Time : FallbackKnockbackDuration;
	if (Radius <= 0.0f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(SourceActor);
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	const FVector SourceLocation = SourceActor->GetActorLocation();
	UKismetSystemLibrary::SphereOverlapActors(
		SourceActor->GetWorld(),
		SourceLocation,
		Radius,
		ObjectTypes,
		nullptr,
		ActorsToIgnore,
		OverlappedActors);

	int32 ValidHitCount = 0;
	int32 KnockbackCount = 0;
	for (AActor* TargetActor : OverlappedActors)
	{
		if (!TargetActor)
		{
			continue;
		}

		if (!SourceCombatComponent->ApplyDamageEffectToTarget(
			TargetActor,
			ResolvedDamageEffectClass,
			1.0f,
			SkillContext.SkillData->FixedDamage,
			ResolvedAttackCoefficient,
			SkillContext.SkillData->bCanCrit,
			ResolvedBreakPower))
		{
			continue;
		}

		++ValidHitCount;

		ULSCharacterCombatComponent* TargetCombatComponent = TargetActor->FindComponentByClass<ULSCharacterCombatComponent>();
		if (TargetCombatComponent && !TargetCombatComponent->CanApplyCrowdControl(ResolvedBreakPower))
		{
			continue;
		}

		if (!TargetCombatComponent)
		{
			continue;
		}

		FVector KnockbackDirection = TargetActor->GetActorLocation() - SourceLocation;
		KnockbackDirection.Z = 0.0f;
		if (KnockbackDirection.IsNearlyZero())
		{
			KnockbackDirection = SourceActor->GetActorForwardVector();
		}

		KnockbackDirection = KnockbackDirection.GetSafeNormal2D();
		if (TargetCombatComponent->ApplyKnockback(KnockbackDirection, KnockbackSpeed, ResolvedKnockbackDuration, KnockbackUpSpeed))
		{
			++KnockbackCount;
		}
	}

	if (bEnableDebugLog)
	{
		UE_LOG(
			LogLS,
			Log,
			TEXT("[GA_Override] Source=%s Radius=%.2f RawTargets=%d ValidHits=%d Knockbacks=%d Coef=%.2f BreakPower=%d"),
			*GetNameSafe(SourceActor),
			Radius,
			OverlappedActors.Num(),
			ValidHitCount,
			KnockbackCount,
			ResolvedAttackCoefficient,
			static_cast<int32>(ResolvedBreakPower));
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
