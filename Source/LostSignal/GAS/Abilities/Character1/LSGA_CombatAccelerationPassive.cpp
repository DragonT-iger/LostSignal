#include "GAS/Abilities/Character1/LSGA_CombatAccelerationPassive.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Data/LSCharacterPassiveSkillRow.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSStatusEffectRow.h"
#include "Engine/GameInstance.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"
#include "Skills/LSCombatAccelerationSkillDataAsset.h"

namespace
{
	bool IsCombatAccelerationTriggerEvent(ELSCharacterPassiveTriggerEvent TriggerEvent)
	{
		return TriggerEvent == ELSCharacterPassiveTriggerEvent::OnComboEnd
			|| TriggerEvent == ELSCharacterPassiveTriggerEvent::OnStrike;
	}

	float NormalizeStatusModifierValue(ELSStatusEffectMathType MathType, float Value)
	{
		if (MathType == ELSStatusEffectMathType::Percent && FMath::Abs(Value) > 1.0f)
		{
			return Value * 0.01f;
		}

		return Value;
	}

	void AccumulateCombatAccelerationModifier(const FName TargetStat, ELSStatusEffectMathType MathType, float ModValue, float& OutAttackSpeedBonus, float& OutAttackPowerBonus)
	{
		const FString TargetStatString = TargetStat.ToString();
		const float NormalizedValue = NormalizeStatusModifierValue(MathType, ModValue);
		if (TargetStatString.Equals(TEXT("Char_Atkspeed"), ESearchCase::IgnoreCase))
		{
			OutAttackSpeedBonus += NormalizedValue;
		}
		else if (TargetStatString.Equals(TEXT("Char_Attack"), ESearchCase::IgnoreCase))
		{
			OutAttackPowerBonus += NormalizedValue;
		}
	}

	void ResolveCombatAccelerationBonuses(const FLSStatusEffectRow& StatusRow, float& OutAttackSpeedBonus, float& OutAttackPowerBonus)
	{
		OutAttackSpeedBonus = 0.0f;
		OutAttackPowerBonus = 0.0f;

		for (const FLSStatusEffectStatModifier& Modifier : StatusRow.Stat_Modifiers)
		{
			AccumulateCombatAccelerationModifier(Modifier.Target_Stat, Modifier.Math_Type, Modifier.Mod_Value, OutAttackSpeedBonus, OutAttackPowerBonus);
		}

		AccumulateCombatAccelerationModifier(StatusRow.Target_Stat, StatusRow.Math_Type, StatusRow.Mod_Value, OutAttackSpeedBonus, OutAttackPowerBonus);
		AccumulateCombatAccelerationModifier(StatusRow.Target_Stat_2, StatusRow.Math_Type_2, StatusRow.Mod_Value_2, OutAttackSpeedBonus, OutAttackPowerBonus);
	}

	const ULSGameDataSubsystem* ResolveGameDataSubsystem(const AActor* SourceActor)
	{
		const UWorld* World = SourceActor ? SourceActor->GetWorld() : nullptr;
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	}
}

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

	const int32 ComboAttackID = TriggerEventData ? FMath::RoundToInt(TriggerEventData->EventMagnitude) : INDEX_NONE;
	if (!SourceActor || !SourceActor->HasAuthority() || !ASC || !SkillData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const ULSGameDataSubsystem* GameDataSubsystem = ResolveGameDataSubsystem(SourceActor);
	const FLSCharacterPassiveSkillRow* PassiveRow = GameDataSubsystem
		? GameDataSubsystem->FindPassiveSkillRowByID(SkillData->GetPassiveSkillID(), TEXT("CombatAcceleration.PassiveRow"))
		: nullptr;
	if (!PassiveRow)
	{
		UE_LOG(LogLS, Warning, TEXT("%s CombatAcceleration passive skipped because PassiveSkill_ID=%d row is missing."),
			*GetNameSafe(SourceActor),
			SkillData->GetPassiveSkillID());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!IsCombatAccelerationTriggerEvent(PassiveRow->Trigger_Event) ||
		(PassiveRow->Trigger_Target_ID > 0 && PassiveRow->Trigger_Target_ID != ComboAttackID))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (!SkillData->BuffEffectClass)
	{
		UE_LOG(LogLS, Warning, TEXT("%s CombatAcceleration passive skipped because BuffEffectClass is missing."), *GetNameSafe(SourceActor));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FLSStatusEffectRow* StatusRow = PassiveRow->Status_ID > 0 && GameDataSubsystem
		? GameDataSubsystem->FindStatusEffectRowByID(PassiveRow->Status_ID, TEXT("CombatAcceleration.StatusRow"))
		: nullptr;
	if (!StatusRow)
	{
		UE_LOG(LogLS, Warning, TEXT("%s CombatAcceleration passive skipped because Status_ID=%d row is missing."),
			*GetNameSafe(SourceActor),
			PassiveRow->Status_ID);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float Duration = PassiveRow->Effect_Duration;
	float AttackSpeedBonus = 0.0f;
	float AttackPowerBonus = 0.0f;
	ResolveCombatAccelerationBonuses(*StatusRow, AttackSpeedBonus, AttackPowerBonus);

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
		TEXT("%s applied CombatAcceleration passive by GAS. PassiveID=%d ComboID=%d StatusID=%d Duration=%.2f AttackSpeed=%.3f AttackPower=%.3f"),
		*GetNameSafe(SourceActor),
		SkillData->GetPassiveSkillID(),
		ComboAttackID,
		PassiveRow->Status_ID,
		Duration,
		AttackSpeedBonus,
		AttackPowerBonus);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
