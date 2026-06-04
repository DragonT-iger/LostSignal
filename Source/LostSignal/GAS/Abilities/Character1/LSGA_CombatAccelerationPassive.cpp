#include "GAS/Abilities/Character1/LSGA_CombatAccelerationPassive.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"
#include "Skills/LSCombatAccelerationSkillDataAsset.h"

ULSGA_CombatAccelerationPassive::ULSGA_CombatAccelerationPassive()
{
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = LSGameplayTags::Event_Combat_BasicAttackHit;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dead);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void ULSGA_CombatAccelerationPassive::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* SourceActor = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const ULSCombatAccelerationSkillDataAsset* SkillData = TriggerEventData
		? Cast<ULSCombatAccelerationSkillDataAsset>(const_cast<UObject*>(TriggerEventData->OptionalObject.Get()))
		: nullptr;

	const int32 ComboIndex = TriggerEventData ? FMath::RoundToInt(TriggerEventData->EventMagnitude) : INDEX_NONE;
	if (!SourceActor || !SourceActor->HasAuthority() || !ASC || !SkillData || ComboIndex != SkillData->RequiredComboIndex)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!SkillData->BuffEffectClass)
	{
		UE_LOG(LogLS, Warning, TEXT("%s CombatAcceleration passive skipped because BuffEffectClass is missing."), *GetNameSafe(SourceActor));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float Duration = SkillData->FallbackDuration;
	const float AttackSpeedBonus = SkillData->FallbackAttackSpeedBonus;
	const float AttackPowerBonus = SkillData->FallbackAttackPowerBonus;

	if (Duration <= 0.0f || (AttackSpeedBonus <= 0.0f && AttackPowerBonus <= 0.0f))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(SkillData);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(SkillData->BuffEffectClass, 1.0f, EffectContext);
	if (!SpecHandle.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SpecHandle.Data->SetDuration(Duration, true);
	SpecHandle.Data->SetSetByCallerMagnitude(LSGameplayTags::Data_Buff_AttackSpeed, AttackSpeedBonus);
	SpecHandle.Data->SetSetByCallerMagnitude(LSGameplayTags::Data_Buff_AttackPower, AttackPowerBonus);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	UE_LOG(
		LogLS,
		Log,
		TEXT("%s applied CombatAcceleration passive by GAS. ComboIndex=%d Duration=%.2f AttackSpeed=%.3f AttackPower=%.3f"),
		*GetNameSafe(SourceActor),
		ComboIndex,
		Duration,
		AttackSpeedBonus,
		AttackPowerBonus);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
