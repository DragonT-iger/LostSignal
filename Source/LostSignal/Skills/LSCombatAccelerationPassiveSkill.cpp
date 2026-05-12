#include "Skills/LSCombatAccelerationPassiveSkill.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Data/LSCharacterSkillRow.h"
#include "GAS/LSGameplayTags.h"
#include "GameplayEffect.h"
#include "LostSignal.h"
#include "Skills/LSSkillDataAsset.h"

bool ULSCombatAccelerationPassiveSkill::HandleBasicAttackHit_Implementation(const FLSBasicAttackHitContext& Context) const
{
	if (!Context.SourceActor || Context.ComboIndex != RequiredComboIndex || Context.ValidHitCount <= 0)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Context.SourceActor);
	if (!ASC || !BuffEffectClass)
	{
		UE_LOG(LogLS, Warning, TEXT("%s CombatAcceleration passive skipped because ASC or BuffEffectClass is missing."), *GetNameSafe(Context.SourceActor));
		return false;
	}

	FLSCharacterSkillRow ResolvedSkillRow;
	const bool bHasSkillRow = TryGetSkillRow(ResolvedSkillRow);
	const float Duration = bHasSkillRow && ResolvedSkillRow.Skill_Effect_Duration > 0.0f ? ResolvedSkillRow.Skill_Effect_Duration : FallbackDuration;
	const float AttackSpeedBonus = bHasSkillRow && ResolvedSkillRow.Skill_Effect_Value > 0.0f ? ResolvedSkillRow.Skill_Effect_Value : FallbackAttackSpeedBonus;
	const float AttackPowerBonus = bHasSkillRow && ResolvedSkillRow.Skill_Effect_Value_2 > 0.0f ? ResolvedSkillRow.Skill_Effect_Value_2 : FallbackAttackPowerBonus;

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(Context.SkillData.Get());

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(BuffEffectClass, 1.0f, EffectContext);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	SpecHandle.Data->SetDuration(Duration, true);
	SpecHandle.Data->SetSetByCallerMagnitude(LSGameplayTags::Data_Buff_AttackSpeed, AttackSpeedBonus);
	SpecHandle.Data->SetSetByCallerMagnitude(LSGameplayTags::Data_Buff_AttackPower, AttackPowerBonus);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	UE_LOG(
		LogLS,
		Log,
		TEXT("%s applied CombatAcceleration passive. ComboIndex=%d Hits=%d Duration=%.2f AttackSpeed=%.3f AttackPower=%.3f"),
		*GetNameSafe(Context.SourceActor),
		Context.ComboIndex,
		Context.ValidHitCount,
		Duration,
		AttackSpeedBonus,
		AttackPowerBonus);

	return true;
}
