#pragma once

#include "CoreMinimal.h"

struct FLSSessionItem;

// 장착 무기/방어구에서 합산된 전투 스탯. DataTable(DT_Weapon/DT_Armor) Row의 원본 값을 그대로 합산한 결과다.
// 비율/평탄 환산(÷100 등)은 GAS 적용 단계(ULSEquipmentStatComponent)에서 처리한다.
struct LOSTSIGNAL_API FLSEquipmentStatTotals
{
	// 무기 스탯
	float Attack = 0.0f;
	float AttackSpeed = 0.0f;
	float SkillHaste = 0.0f;
	float CritRate = 0.0f;
	float CritDamage = 0.0f;
	float DefensePenetration = 0.0f;

	// 방어구 스탯
	float Health = 0.0f;
	float Defense = 0.0f;
	float Recovery = 0.0f;
};

namespace LSEquipmentStats
{
	// 장비 5칸(무기/프로세서/코어/구동계/프레임)을 순회하며 DataTable Row의 전투 스탯을 합산한다.
	// 무기는 Weapon_ 접두어, 방어구는 Armor_ 접두어로 판정해 각 테이블 Row를 조회한다.
	LOSTSIGNAL_API FLSEquipmentStatTotals ComputeEquipmentStatTotals(const TArray<FLSSessionItem>& EquipmentSlots);
}
