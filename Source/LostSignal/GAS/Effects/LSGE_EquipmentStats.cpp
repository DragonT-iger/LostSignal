#include "GAS/Effects/LSGE_EquipmentStats.h"

#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GAS/LSGameplayTags.h"

namespace
{
// 어트리뷰트에 SetByCaller(Additive) 모디파이어 하나를 추가한다.
void AddEquipmentModifier(UGameplayEffect& Effect, const FGameplayAttribute& Attribute, const FGameplayTag& DataTag)
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

ULSGE_EquipmentStats::ULSGE_EquipmentStats(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// 평탄 스탯: 장비 합산값을 그대로 가산.
	AddEquipmentModifier(*this, ULSCharacterAttributeSet::GetAttackAttribute(), LSGameplayTags::Data_Equip_Attack);
	AddEquipmentModifier(*this, ULSCombatAttributeSet::GetMaxHealthAttribute(), LSGameplayTags::Data_Equip_Health);
	AddEquipmentModifier(*this, ULSCharacterAttributeSet::GetDefenceAttribute(), LSGameplayTags::Data_Equip_Defense);
	AddEquipmentModifier(*this, ULSCharacterAttributeSet::GetRecoveryAttribute(), LSGameplayTags::Data_Equip_Recovery);

	// 비율 스탯: 장비 합산값 ÷100을 가산 (환산은 ULSEquipmentStatComponent에서 수행).
	AddEquipmentModifier(*this, ULSCharacterAttributeSet::GetAttackSpeedAttribute(), LSGameplayTags::Data_Equip_AttackSpeed);
	AddEquipmentModifier(*this, ULSCharacterAttributeSet::GetCooldownReductionAttribute(), LSGameplayTags::Data_Equip_SkillHaste);
	AddEquipmentModifier(*this, ULSCharacterAttributeSet::GetCritDamageAttribute(), LSGameplayTags::Data_Equip_CritDamage);
	AddEquipmentModifier(*this, ULSCharacterAttributeSet::GetCritChanceAttribute(), LSGameplayTags::Data_Equip_CritRate);
	AddEquipmentModifier(*this, ULSCharacterAttributeSet::GetArmorPenetrationAttribute(), LSGameplayTags::Data_Equip_ArmorPenetration);
}
