#include "Data/LSChipStats.h"

#include "Data/LSChipRow.h"
#include "Data/LSChipStatRow.h"
#include "Data/LSDropSettings.h"
#include "Engine/DataTable.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "LostSignal.h"
#include "Session/LSSessionSubsystem.h"

namespace
{
// 칩 스탯의 정규 순서. 표시/롤링 풀의 기준.
struct FChipStatEntry
{
	FName Key;
	const FLSStatRange FLSChipStatRow::* Member;
};

const TArray<FChipStatEntry>& GetChipStatSchema()
{
	static const TArray<FChipStatEntry> Schema = {
		{ TEXT("Chip_Attack"),              &FLSChipStatRow::Chip_Attack },
		{ TEXT("Chip_Attack_Speed"),        &FLSChipStatRow::Chip_Attack_Speed },
		{ TEXT("Chip_Skill_Haste"),         &FLSChipStatRow::Chip_Skill_Haste },
		{ TEXT("Chip_Critical_Rate"),       &FLSChipStatRow::Chip_Critical_Rate },
		{ TEXT("Chip_Critical_Damage"),     &FLSChipStatRow::Chip_Critical_Damage },
		{ TEXT("Chip_Defense_Penetration"), &FLSChipStatRow::Chip_Defense_Penetration },
		{ TEXT("Chip_Health"),              &FLSChipStatRow::Chip_Health },
		{ TEXT("Chip_Defense"),             &FLSChipStatRow::Chip_Defense },
		{ TEXT("Chip_Recovery"),            &FLSChipStatRow::Chip_Recovery },
		{ TEXT("Chip_Move_Speed"),          &FLSChipStatRow::Chip_Move_Speed },
	};
	return Schema;
}

void AddFallbackProtocolValue(const FName ChipRowName, const int32 Value, FLSChipProtocolTotals& Totals)
{
	if (Value <= 0)
	{
		return;
	}

	const FString RowNameString = ChipRowName.ToString();
	if (RowNameString.Contains(TEXT("_HP")) || RowNameString.Contains(TEXT("_SP")))
	{
		Totals.Survival += Value;
	}
	else if (RowNameString.Contains(TEXT("_Inventory")))
	{
		Totals.Carrying += Value;
	}
	else if (RowNameString.Contains(TEXT("_Minimap")) || RowNameString.Contains(TEXT("_Exitpoint")))
	{
		Totals.Navigation += Value;
	}
	else if (RowNameString.Contains(TEXT("_Battle")) || RowNameString.Contains(TEXT("_Skill")))
	{
		Totals.Battle += Value;
	}
}
}

namespace LSChipStats
{
TArray<FLSChipResolvedStat> RollChipStats(const FName ChipRowName)
{
	TArray<FLSChipResolvedStat> Out;
	if (ChipRowName.IsNone())
	{
		return Out;
	}

	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	UDataTable* ChipTable = Settings ? Settings->ChipTable.LoadSynchronous() : nullptr;
	UDataTable* ChipStatTable = Settings ? Settings->ChipStatTable.LoadSynchronous() : nullptr;
	if (!ChipTable || !ChipStatTable)
	{
		UE_LOG(LogLS, Warning, TEXT("[ChipStats] ChipTable/ChipStatTable이 설정되지 않아 스탯을 산출할 수 없습니다."));
		return Out;
	}

	const FLSChipRow* ChipRow = ChipTable->FindRow<FLSChipRow>(ChipRowName, TEXT("RollChipStats"));
	if (!ChipRow)
	{
		UE_LOG(LogLS, Warning, TEXT("[ChipStats] 칩 행 '%s' 없음."), *ChipRowName.ToString());
		return Out;
	}

	const FString Grade = LSInventorySlotUtils::ResolveItemGradeFromRowName(ChipRowName);
	if (Grade.IsEmpty())
	{
		return Out;
	}

	const FLSChipStatRow* StatRow = ChipStatTable->FindRow<FLSChipStatRow>(FName(*Grade), TEXT("RollChipStats"));
	if (!StatRow)
	{
		UE_LOG(LogLS, Warning, TEXT("[ChipStats] 등급 '%s' 의 ChipStat 행이 없음."), *Grade);
		return Out;
	}

	const TArray<FChipStatEntry>& Schema = GetChipStatSchema();
	const int32 PoolSize = Schema.Num();
	const int32 Count = FMath::Clamp(ChipRow->Item_Chip_Status_Count, 0, PoolSize);
	if (Count <= 0)
	{
		return Out;
	}

	TArray<int32> Indices;
	Indices.Reserve(PoolSize);
	for (int32 i = 0; i < PoolSize; ++i)
	{
		Indices.Add(i);
	}
	for (int32 i = PoolSize - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Indices.Swap(i, j);
	}

	// 선택된 N개를 정규 순서(인덱스 오름차순)로 정렬해 표시 일관성 유지.
	TArray<int32> Chosen(Indices.GetData(), Count);
	Chosen.Sort();

	for (const int32 Index : Chosen)
	{
		const FChipStatEntry& Entry = Schema[Index];
		const FLSStatRange& Range = StatRow->*(Entry.Member);
		const int32 Lo = FMath::Min(Range.Min, Range.Max);
		const int32 Hi = FMath::Max(Range.Min, Range.Max);

		FLSChipResolvedStat Resolved;
		Resolved.StatKey = Entry.Key;
		Resolved.Value = FMath::RandRange(Lo, Hi);
		Out.Add(Resolved);
	}

	return Out;
}

int32 ResolveInactiveSignalSlotCount(const float SignalGaugePercent)
{
	const float ClampedPercent = FMath::Clamp(SignalGaugePercent, 0.0f, 1.0f);
	if (ClampedPercent > 0.9f)
	{
		return 0;
	}

	return FMath::Clamp(FMath::FloorToInt((0.9f - ClampedPercent) / 0.1f) + 1, 0, 10);
}

TArray<FLSSessionItem> BuildSignalActiveEquipmentItems(const TArray<FLSSessionItem>& Items, const int32 InactiveSlotCount)
{
	TArray<FLSSessionItem> ActiveItems;
	ActiveItems.Reserve(Items.Num());
	for (int32 SlotIndex = InactiveSlotCount; SlotIndex < Items.Num(); ++SlotIndex)
	{
		ActiveItems.Add(Items[SlotIndex]);
	}
	return ActiveItems;
}

TMap<FName, int32> AggregateChipStatTotals(const TArray<FLSSessionItem>& Items)
{
	TMap<FName, int32> Totals;

	for (const FLSSessionItem& Item : Items)
	{
		if (!LSInventorySlotUtils::IsFilled(Item))
		{
			continue;
		}

		for (const FLSChipResolvedStat& Stat : Item.ChipStats)
		{
			if (Stat.StatKey.IsNone())
			{
				continue;
			}

			int32& Total = Totals.FindOrAdd(Stat.StatKey);
			Total += Stat.Value;
		}
	}

	return Totals;
}

TMap<FName, int32> ComputeEffectiveChipStatTotals(const TArray<FLSSessionItem>& Items, const int32 InactiveSlotCount)
{
	const TMap<FName, int32> AllTotals = AggregateChipStatTotals(Items);

	// 신호 유실은 프로토콜만 깎는다. 비활성 슬롯(앞에서부터 InactiveSlotCount개) 칩 스탯의 절반은
	// 전체 합산 위에 보너스로 가산한다 (UI의 SignalLossText 표시값과 동일 규칙).
	TArray<FLSSessionItem> InactiveItems;
	InactiveItems.Reserve(FMath::Min(Items.Num(), InactiveSlotCount));
	for (int32 SlotIndex = 0; SlotIndex < Items.Num() && SlotIndex < InactiveSlotCount; ++SlotIndex)
	{
		InactiveItems.Add(Items[SlotIndex]);
	}
	const TMap<FName, int32> InactiveTotals = AggregateChipStatTotals(InactiveItems);

	TMap<FName, int32> Effective;
	Effective.Reserve(AllTotals.Num());
	for (const TPair<FName, int32>& Pair : AllTotals)
	{
		const int32* InactivePtr = InactiveTotals.Find(Pair.Key);
		const int32 HalfBonus = InactivePtr ? FMath::RoundToInt(static_cast<float>(*InactivePtr) * 0.5f) : 0;
		Effective.Add(Pair.Key, Pair.Value + HalfBonus);
	}

	return Effective;
}

FLSChipProtocolTotals AggregateChipProtocolTotals(const TArray<FLSSessionItem>& Items, const UObject* LogContext)
{
	FLSChipProtocolTotals Totals;

	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	UDataTable* ChipTable = Settings ? Settings->ChipTable.LoadSynchronous() : nullptr;
	if (!ChipTable)
	{
		UE_LOG(LogLS, Warning, TEXT("[ChipStats] Cannot aggregate chip protocols because ChipTable is not set on %s."), *GetNameSafe(LogContext));
		return Totals;
	}

	for (const FLSSessionItem& Item : Items)
	{
		if (!LSInventorySlotUtils::IsFilled(Item))
		{
			continue;
		}

		const FLSChipRow* ChipRow = ChipTable->FindRow<FLSChipRow>(Item.ItemRowName, TEXT("AggregateChipProtocolTotals"));
		if (!ChipRow)
		{
			UE_LOG(LogLS, Warning, TEXT("[ChipStats] Cannot aggregate chip protocol because chip '%s' is missing on %s."),
				*Item.ItemRowName.ToString(), *GetNameSafe(LogContext));
			continue;
		}

		const int32 ExplicitProtocolTotal =
			ChipRow->Chip_Protocol_Survival +
			ChipRow->Chip_Protocol_Carrying +
			ChipRow->Chip_Protocol_Battle +
			ChipRow->Chip_Protocol_Navigation;
		if (ExplicitProtocolTotal > 0)
		{
			Totals.Survival += ChipRow->Chip_Protocol_Survival;
			Totals.Carrying += ChipRow->Chip_Protocol_Carrying;
			Totals.Battle += ChipRow->Chip_Protocol_Battle;
			Totals.Navigation += ChipRow->Chip_Protocol_Navigation;
			continue;
		}

		AddFallbackProtocolValue(Item.ItemRowName, ChipRow->Item_Chip_Status_Count, Totals);
	}

	return Totals;
}

FText GetChipStatLabel(const FName StatKey)
{
	static const TMap<FName, FText> Labels = {
		{ TEXT("Chip_Attack"),              NSLOCTEXT("LSChipStats", "Attack", "공격력") },
		{ TEXT("Chip_Attack_Speed"),        NSLOCTEXT("LSChipStats", "AttackSpeed", "공격 속도") },
		{ TEXT("Chip_Skill_Haste"),         NSLOCTEXT("LSChipStats", "SkillHaste", "스킬 가속") },
		{ TEXT("Chip_Critical_Rate"),       NSLOCTEXT("LSChipStats", "CritRate", "치명타 확률") },
		{ TEXT("Chip_Critical_Damage"),     NSLOCTEXT("LSChipStats", "CritDamage", "치명타 피해") },
		{ TEXT("Chip_Defense_Penetration"), NSLOCTEXT("LSChipStats", "DefPen", "방어 관통") },
		{ TEXT("Chip_Health"),              NSLOCTEXT("LSChipStats", "Health", "체력") },
		{ TEXT("Chip_Defense"),             NSLOCTEXT("LSChipStats", "Defense", "방어력") },
		{ TEXT("Chip_Recovery"),            NSLOCTEXT("LSChipStats", "Recovery", "회복") },
		{ TEXT("Chip_Move_Speed"),          NSLOCTEXT("LSChipStats", "MoveSpeed", "이동 속도") },
	};

	if (const FText* Found = Labels.Find(StatKey))
	{
		return *Found;
	}
	return FText::FromName(StatKey);
}
}
