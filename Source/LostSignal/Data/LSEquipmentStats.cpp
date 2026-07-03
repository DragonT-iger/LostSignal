#include "Data/LSEquipmentStats.h"

#include "Data/LSArmorRow.h"
#include "Data/LSDropSettings.h"
#include "Data/LSWeaponRow.h"
#include "Engine/DataTable.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "LostSignal.h"
#include "Session/LSSessionSubsystem.h"

namespace LSEquipmentStats
{
FLSEquipmentStatTotals ComputeEquipmentStatTotals(const TArray<FLSSessionItem>& EquipmentSlots)
{
	FLSEquipmentStatTotals Totals;

	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (!Settings)
	{
		UE_LOG(LogLS, Warning, TEXT("[EquipStat] LSDropSettings가 없어 장비 스탯을 합산할 수 없습니다."));
		return Totals;
	}

	UDataTable* WeaponTable = Settings->WeaponTable.LoadSynchronous();
	UDataTable* ArmorTable = Settings->ArmorTable.LoadSynchronous();

	for (const FLSSessionItem& Slot : EquipmentSlots)
	{
		if (!LSInventorySlotUtils::IsFilled(Slot))
		{
			continue;
		}

		const FString RowNameString = Slot.ItemRowName.ToString();
		if (RowNameString.StartsWith(TEXT("Weapon_")))
		{
			const FLSWeaponRow* Row = WeaponTable ? WeaponTable->FindRow<FLSWeaponRow>(Slot.ItemRowName, TEXT("ComputeEquipmentStatTotals")) : nullptr;
			if (!Row)
			{
				UE_LOG(LogLS, Warning, TEXT("[EquipStat] 무기 Row '%s'를 찾지 못해 스탯 합산에서 제외합니다."), *RowNameString);
				continue;
			}

			Totals.Attack += Row->Item_Attack;
			Totals.AttackSpeed += Row->Item_Attack_Speed;
			Totals.SkillHaste += Row->Item_Skill_Haste;
			Totals.CritRate += Row->Item_Critical_Rate;
			Totals.CritDamage += Row->Item_Critical_Damage;
			Totals.DefensePenetration += Row->Item_Defense_Penetration;
		}
		else if (RowNameString.StartsWith(TEXT("Armor_")))
		{
			const FLSArmorRow* Row = ArmorTable ? ArmorTable->FindRow<FLSArmorRow>(Slot.ItemRowName, TEXT("ComputeEquipmentStatTotals")) : nullptr;
			if (!Row)
			{
				UE_LOG(LogLS, Warning, TEXT("[EquipStat] 방어구 Row '%s'를 찾지 못해 스탯 합산에서 제외합니다."), *RowNameString);
				continue;
			}

			Totals.Health += Row->Item_Health;
			Totals.Defense += Row->Item_Defense;
			Totals.Recovery += Row->Item_Recovery;
		}
	}

	return Totals;
}
}
