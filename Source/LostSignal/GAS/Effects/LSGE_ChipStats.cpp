#include "GAS/Effects/LSGE_ChipStats.h"

#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GAS/LSGameplayTags.h"

namespace
{
// 어트리뷰트에 SetByCaller(Additive) 모디파이어 하나를 추가한다.
void AddChipModifier(UGameplayEffect& Effect, const FGameplayAttribute& Attribute, const FGameplayTag& DataTag)
{
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = DataTag;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Effect.Modifiers.Add(Modifier);
}
}

ULSGE_ChipStats::ULSGE_ChipStats(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// 평탄 스탯: 칩 합산값을 그대로 가산.
	AddChipModifier(*this, ULSCharacterAttributeSet::GetAttackAttribute(), LSGameplayTags::Data_Chip_Attack);
	AddChipModifier(*this, ULSCombatAttributeSet::GetMaxHealthAttribute(), LSGameplayTags::Data_Chip_Health);
	AddChipModifier(*this, ULSCharacterAttributeSet::GetDefenceAttribute(), LSGameplayTags::Data_Chip_Defense);
	AddChipModifier(*this, ULSCharacterAttributeSet::GetRecoveryAttribute(), LSGameplayTags::Data_Chip_Recovery);

	// 배율/비율 스탯: 칩 합산값 ÷100을 가산 (환산은 ULSChipStatComponent에서 수행).
	AddChipModifier(*this, ULSCharacterAttributeSet::GetAttackSpeedAttribute(), LSGameplayTags::Data_Chip_AttackSpeed);
	AddChipModifier(*this, ULSCharacterAttributeSet::GetMoveSpeedAttribute(), LSGameplayTags::Data_Chip_MoveSpeed);
	AddChipModifier(*this, ULSCharacterAttributeSet::GetCritDamageAttribute(), LSGameplayTags::Data_Chip_CritDamage);
	AddChipModifier(*this, ULSCharacterAttributeSet::GetCritChanceAttribute(), LSGameplayTags::Data_Chip_CritRate);
	AddChipModifier(*this, ULSCharacterAttributeSet::GetCooldownReductionAttribute(), LSGameplayTags::Data_Chip_SkillHaste);
	AddChipModifier(*this, ULSCharacterAttributeSet::GetArmorPenetrationAttribute(), LSGameplayTags::Data_Chip_ArmorPenetration);
}
