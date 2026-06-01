#include "Data/LSDropSubsystem.h"
#include "LostSignal.h"
#include "Data/LSDropSettings.h"
#include "Data/LSRootingObjectRow.h"
#include "Data/LSDropTableRow.h"
#include "Data/LSGroupTableRow.h"
#include "Data/LSChipRow.h"
#include "Data/LSChipStats.h"
#include "Data/LSWeaponRow.h"
#include "Data/LSArmorRow.h"
#include "Data/LSItemRow.h"
#include "Engine/DataTable.h"

// "Drop_Chip_Chest_1" → "Drop_Chip_Chest"
// "Group_Chip_Supply_2" → "Group_Chip_Supply"
// 마지막 "_숫자" 부분만 제거. 숫자가 아니면 원본 반환.
static FName ExtractRowNamePrefix(const FName& RowName)
{
	FString Str = RowName.ToString();
	int32 LastUnderscore;
	if (!Str.FindLastChar(TEXT('_'), LastUnderscore))
	{
		return RowName;
	}

	const FString Suffix = Str.Mid(LastUnderscore + 1);
	for (TCHAR Ch : Suffix)
	{
		if (!FChar::IsDigit(Ch))
		{
			return RowName;
		}
	}

	return FName(*Str.Left(LastUnderscore));
}

void ULSDropSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadTables();
	CacheDropTable();
	CacheGroupTable();

#if WITH_EDITOR
	ValidateGroupReferences();
#endif

	UE_LOG(LogLS, Log, TEXT("DropSubsystem 초기화 완료 - DropTable %d그룹, GroupTable %d그룹"),
		DropTableMap.Num(), GroupTableMap.Num());
}

void ULSDropSubsystem::LoadTables()
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();

	RootingObjectTable = Settings->RootingObjectTable.LoadSynchronous();
	DropTableData      = Settings->DropTable.LoadSynchronous();
	GroupTableData     = Settings->GroupTable.LoadSynchronous();
	ChipTable          = Settings->ChipTable.LoadSynchronous();
	WeaponTable        = Settings->WeaponTable.LoadSynchronous();
	ArmorTable         = Settings->ArmorTable.LoadSynchronous();
	ItemTable          = Settings->ItemTable.LoadSynchronous();

	if (!RootingObjectTable) UE_LOG(LogLS, Warning, TEXT("RootingObjectTable 미설정 - 프로젝트 설정 > LS Drop Settings 확인"));
	if (!DropTableData)      UE_LOG(LogLS, Warning, TEXT("DropTable 미설정 - 프로젝트 설정 > LS Drop Settings 확인"));
	if (!GroupTableData)     UE_LOG(LogLS, Warning, TEXT("GroupTable 미설정 - 프로젝트 설정 > LS Drop Settings 확인"));
}

void ULSDropSubsystem::CacheDropTable()
{
	if (!DropTableData) return;

	DropTableMap.Empty();
	for (const auto& Pair : DropTableData->GetRowMap())
	{
		const FName Prefix = ExtractRowNamePrefix(Pair.Key);
		const FLSDropTableRow* Row = reinterpret_cast<const FLSDropTableRow*>(Pair.Value);
		DropTableMap.FindOrAdd(Prefix).Add(Row);
	}
}

void ULSDropSubsystem::CacheGroupTable()
{
	if (!GroupTableData) return;

	GroupTableMap.Empty();
	for (const auto& Pair : GroupTableData->GetRowMap())
	{
		const FName Prefix = ExtractRowNamePrefix(Pair.Key);
		const FLSGroupTableRow* Row = reinterpret_cast<const FLSGroupTableRow*>(Pair.Value);
		GroupTableMap.FindOrAdd(Prefix).Add(Row);
	}
}

#if WITH_EDITOR
void ULSDropSubsystem::ValidateGroupReferences()
{
	for (const auto& DropGroup : DropTableMap)
	{
		for (const FLSDropTableRow* Entry : DropGroup.Value)
		{
			if (Entry->Group_Table_Name.IsNone()) continue;

			if (!GroupTableMap.Contains(Entry->Group_Table_Name))
			{
				UE_LOG(LogLS, Warning, TEXT("[데이터검증] DropTable '%s' → GroupTable '%s' 없음"),
					*DropGroup.Key.ToString(), *Entry->Group_Table_Name.ToString());
			}
		}
	}
}
#endif

TArray<FLSDropResult> ULSDropSubsystem::RollDropTable(FName DropTableName)
{
	TArray<FLSDropResult> Results;

	const TArray<const FLSDropTableRow*>* Entries = DropTableMap.Find(DropTableName);
	if (!Entries)
	{
		UE_LOG(LogLS, Warning, TEXT("DropTable '%s' 없음"), *DropTableName.ToString());
		return Results;
	}

	UE_LOG(LogLS, Log, TEXT("  DropTable '%s' 처리 (%d개 항목)"), *DropTableName.ToString(), Entries->Num());

	for (const FLSDropTableRow* Entry : *Entries)
	{
		const float Roll = FMath::FRandRange(0.0f, 100.0f);
		if (Roll > Entry->Drop_Rate)
		{
			UE_LOG(LogLS, Log, TEXT("    [실패] Rate=%.2f%% Roll=%.2f"), Entry->Drop_Rate, Roll);
			continue;
		}

		if (Entry->Group_Table_Name.IsNone())
		{
			UE_LOG(LogLS, Warning, TEXT("    Group_Table_Name 미설정 (DropTable '%s')"), *DropTableName.ToString());
			continue;
		}

		const FName ItemRowName = RollGroupTable(Entry->Group_Table_Name);
		if (ItemRowName.IsNone())
		{
			UE_LOG(LogLS, Warning, TEXT("    [경고] 그룹 '%s' 롤 결과 없음"), *Entry->Group_Table_Name.ToString());
			continue;
		}

		FLSDropResult Result;
		Result.ItemRowName = ItemRowName;
		Result.Amount      = Entry->Drop_Amount;
		Result.ItemText    = FindItemText(ItemRowName);
		// 칩은 획득 시점에 인스턴스 스탯 시드를 부여한다. (등급+시드로 전투 스탯 결정론적 산출)
		if (ItemRowName.ToString().StartsWith(TEXT("Chip_")))
		{
			Result.StatSeed = LSChipStats::RollNewChipSeed();
		}
		Results.Add(Result);

		UE_LOG(LogLS, Log, TEXT("    [성공] 그룹 '%s' -> %s(%s) x%d"),
			*Entry->Group_Table_Name.ToString(), *ItemRowName.ToString(),
			*Result.ItemText.ToString(), Entry->Drop_Amount);
	}

	return Results;
}

FName ULSDropSubsystem::RollGroupTable(FName GroupTableName)
{
	const TArray<const FLSGroupTableRow*>* Entries = GroupTableMap.Find(GroupTableName);
	if (!Entries || Entries->IsEmpty())
	{
		UE_LOG(LogLS, Warning, TEXT("GroupTable '%s' 없음"), *GroupTableName.ToString());
		return NAME_None;
	}

	int32 TotalWeight = 0;
	for (const FLSGroupTableRow* Row : *Entries)
	{
		TotalWeight += Row->Group_Weight;
	}

	if (TotalWeight <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("GroupTable '%s' 총 가중치 0"), *GroupTableName.ToString());
		return NAME_None;
	}

	int32 Roll = FMath::RandRange(1, TotalWeight);
	int32 Accumulated = 0;

	UE_LOG(LogLS, Log, TEXT("      [가중치 롤] 그룹='%s' 총가중치=%d 롤=%d"),
		*GroupTableName.ToString(), TotalWeight, Roll);

	for (const FLSGroupTableRow* Row : *Entries)
	{
		const int32 PrevAccumulated = Accumulated;
		Accumulated += Row->Group_Weight;
		const bool bSelected = Roll <= Accumulated;
		UE_LOG(LogLS, Log, TEXT("        %s '%s' 가중치=%d 구간=[%d~%d] %s"),
			bSelected ? TEXT("▶") : TEXT(" "),
			*Row->Item_Name.ToString(),
			Row->Group_Weight,
			PrevAccumulated + 1, Accumulated,
			bSelected ? TEXT("← 선택") : TEXT(""));
		if (bSelected)
		{
			return Row->Item_Name;
		}
	}

	return (*Entries).Last()->Item_Name;
}

TArray<FLSDropResult> ULSDropSubsystem::OpenRootingObject(const FName& RootingObjectRowName)
{
	TArray<FLSDropResult> Results;

	if (!RootingObjectTable)
	{
		UE_LOG(LogLS, Warning, TEXT("RootingObjectTable 없음"));
		return Results;
	}

	const FLSRootingObjectRow* Row = RootingObjectTable->FindRow<FLSRootingObjectRow>(
		RootingObjectRowName, TEXT("OpenRootingObject"));
	if (!Row)
	{
		UE_LOG(LogLS, Warning, TEXT("RootingObject '%s' 없음"), *RootingObjectRowName.ToString());
		return Results;
	}

	UE_LOG(LogLS, Log, TEXT("루팅 오브젝트 [%s] '%s' 열림"),
		*RootingObjectRowName.ToString(), *Row->Loot_Object_Text.ToString());

	if (!Row->Drop_Table_Name.IsNone())
	{
		Results = RollDropTable(Row->Drop_Table_Name);
	}
	else
	{
		UE_LOG(LogLS, Log, TEXT("  Drop_Table_Name 미설정"));
	}

	UE_LOG(LogLS, Log, TEXT("  총 %d개 아이템 드랍"), Results.Num());
	return Results;
}

void ULSDropSubsystem::TestDrop(const FName& RootingObjectRowName)
{
	UE_LOG(LogLS, Log, TEXT("========== 드랍 테스트: %s =========="), *RootingObjectRowName.ToString());
	TArray<FLSDropResult> Results = OpenRootingObject(RootingObjectRowName);

	UE_LOG(LogLS, Log, TEXT("---------- 결과 ----------"));
	for (const FLSDropResult& R : Results)
	{
		UE_LOG(LogLS, Log, TEXT("  [%s] %s x%d"), *R.ItemRowName.ToString(), *R.ItemText.ToString(), R.Amount);
	}
	UE_LOG(LogLS, Log, TEXT("==========  끝  =========="));
}

FText ULSDropSubsystem::FindItemText(const FName& ItemRowName) const
{
	if (ItemRowName.IsNone()) return FText::FromString(TEXT("(없음)"));

	const FString NameStr = ItemRowName.ToString();

	if (NameStr.StartsWith(TEXT("Chip_")) && ChipTable)
	{
		if (const FLSChipRow* Row = ChipTable->FindRow<FLSChipRow>(ItemRowName, TEXT("")))
			return Row->Item_Text;
	}
	else if (NameStr.StartsWith(TEXT("Weapon_")) && WeaponTable)
	{
		if (const FLSWeaponRow* Row = WeaponTable->FindRow<FLSWeaponRow>(ItemRowName, TEXT("")))
			return Row->Item_Text;
	}
	else if (NameStr.StartsWith(TEXT("Armor_")) && ArmorTable)
	{
		if (const FLSArmorRow* Row = ArmorTable->FindRow<FLSArmorRow>(ItemRowName, TEXT("")))
			return Row->Item_Text;
	}
	else if (NameStr.StartsWith(TEXT("Item_")) && ItemTable)
	{
		if (const FLSItemRow* Row = ItemTable->FindRow<FLSItemRow>(ItemRowName, TEXT("")))
			return Row->Item_Text;
	}

	return FText::FromString(FString::Printf(TEXT("Unknown_%s"), *NameStr));
}
