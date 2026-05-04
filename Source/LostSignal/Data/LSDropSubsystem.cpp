#include "Data/LSDropSubsystem.h"
#include "Data/LSDropSettings.h"
#include "Data/LSRootingObjectRow.h"
#include "Data/LSDropTableRow.h"
#include "Data/LSGroupTableRow.h"
#include "Data/LSChipRow.h"
#include "Data/LSWeaponRow.h"
#include "Data/LSArmorRow.h"
#include "Data/LSItemRow.h"
#include "Engine/DataTable.h"

DEFINE_LOG_CATEGORY_STATIC(LogDrop, Log, All);

// "D_20000_1" → 20000, "G_30000_3" → 30000
static int32 ParseGroupIDFromRowName(const FName& RowName)
{
	TArray<FString> Parts;
	RowName.ToString().ParseIntoArray(Parts, TEXT("_"));
	if (Parts.Num() >= 2)
	{
		return FCString::Atoi(*Parts[1]);
	}

	UE_LOG(LogLS, Warning, TEXT("Invalid group reference: %s"), *RowName);

	return 0;
}

static int32 ParseGroupIDFromReference(const FString& Reference)
{
	TArray<FString> Parts;
	Reference.ParseIntoArray(Parts, TEXT("_"));
	if (Parts.Num() >= 2 && Parts[0].Equals(TEXT("G"), ESearchCase::IgnoreCase))
	{
		return FCString::Atoi(*Parts[1]);
	}

	UE_LOG(LogLS, Warning, TEXT("Invalid group reference: %s"), *Reference);

	return FCString::Atoi(*Reference);
}

void ULSDropSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadTables();
	CacheDropTable();
	CacheGroupTable();

	UE_LOG(LogDrop, Log, TEXT("DropSubsystem 초기화 완료 - DropTable %d그룹, GroupTable %d그룹"),
		DropTableMap.Num(), GroupTableMap.Num());
}

void ULSDropSubsystem::LoadTables()
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();

	RootingObjectTable = Settings->RootingObjectTable.LoadSynchronous();
	DropTableData = Settings->DropTable.LoadSynchronous();
	GroupTableData = Settings->GroupTable.LoadSynchronous();
	ChipTable = Settings->ChipTable.LoadSynchronous();
	WeaponTable = Settings->WeaponTable.LoadSynchronous();
	ArmorTable = Settings->ArmorTable.LoadSynchronous();
	ItemTable = Settings->ItemTable.LoadSynchronous();

	if (!RootingObjectTable) UE_LOG(LogDrop, Warning, TEXT("RootingObjectTable 미설정 - 프로젝트 설정 > LS Drop Settings 확인"));
	if (!DropTableData) UE_LOG(LogDrop, Warning, TEXT("DropTable 미설정 - 프로젝트 설정 > LS Drop Settings 확인"));
	if (!GroupTableData) UE_LOG(LogDrop, Warning, TEXT("GroupTable 미설정 - 프로젝트 설정 > LS Drop Settings 확인"));
}

void ULSDropSubsystem::CacheDropTable()
{
	if (!DropTableData) return;

	DropTableMap.Empty();
	const TMap<FName, uint8*>& RowMap = DropTableData->GetRowMap();
	for (const auto& Pair : RowMap)
	{
		const int32 GroupID = ParseGroupIDFromRowName(Pair.Key);
		const FLSDropTableRow* Row = reinterpret_cast<const FLSDropTableRow*>(Pair.Value);
		DropTableMap.FindOrAdd(GroupID).Add(Row);
	}
}

void ULSDropSubsystem::CacheGroupTable()
{
	if (!GroupTableData) return;

	GroupTableMap.Empty();
	const TMap<FName, uint8*>& RowMap = GroupTableData->GetRowMap();
	for (const auto& Pair : RowMap)
	{
		const int32 GroupID = ParseGroupIDFromRowName(Pair.Key);
		const FLSGroupTableRow* Row = reinterpret_cast<const FLSGroupTableRow*>(Pair.Value);
		GroupTableMap.FindOrAdd(GroupID).Add(Row);
	}
}

TArray<FLSDropResult> ULSDropSubsystem::RollDropTable(int32 DropTableID)
{
	TArray<FLSDropResult> Results;

	const TArray<const FLSDropTableRow*>* Entries = DropTableMap.Find(DropTableID);
	if (!Entries)
	{
		UE_LOG(LogDrop, Warning, TEXT("DropTable ID %d 없음"), DropTableID);
		return Results;
	}

	UE_LOG(LogDrop, Log, TEXT("  DropTable %d 처리 (%d개 항목)"), DropTableID, Entries->Num());

	for (const FLSDropTableRow* Entry : *Entries)
	{
		const float Roll = FMath::FRandRange(0.0f, 100.0f);
		if (Roll > Entry->Drop_Rate)
		{
			UE_LOG(LogDrop, Log, TEXT("    [실패] Rate=%.2f%% Roll=%.2f"), Entry->Drop_Rate, Roll);
			continue;
		}

		if (Entry->Item_Table_ID.IsEmpty())
		{
			UE_LOG(LogDrop, Warning, TEXT("    Empty Item_Table_ID in DropTable %d"), DropTableID);
			continue;
		}

		if (Entry->Item_Table_ID.StartsWith(TEXT("G_")) || Entry->Drop_Table_Type == 1)
		{
			const int32 GroupID = ParseGroupIDFromReference(Entry->Item_Table_ID);
			const FString ItemRowName = RollGroupTable(GroupID);
			if (!ItemRowName.IsEmpty())
			{
				FLSDropResult Result;
				Result.ItemRowName = ItemRowName;
				Result.Amount = Entry->Drop_Amount;
				Result.ItemName = FindItemName(ItemRowName);
				Results.Add(Result);
				UE_LOG(LogDrop, Log, TEXT("    [성공] 그룹 %d -> %s(%s) x%d"),
					GroupID, *ItemRowName, *Result.ItemName, Entry->Drop_Amount);
			}
		}
		else
		{
			FLSDropResult Result;
			Result.ItemRowName = Entry->Item_Table_ID;
			Result.Amount = Entry->Drop_Amount;
			Result.ItemName = FindItemName(Entry->Item_Table_ID);
			Results.Add(Result);
			UE_LOG(LogDrop, Log, TEXT("    [성공] 고정드랍 -> %s(%s) x%d"),
				*Entry->Item_Table_ID, *Result.ItemName, Entry->Drop_Amount);
		}
	}

	return Results;
}

FString ULSDropSubsystem::RollGroupTable(int32 GroupID)
{
	const TArray<const FLSGroupTableRow*>* Entries = GroupTableMap.Find(GroupID);
	if (!Entries || Entries->Num() == 0)
	{
		UE_LOG(LogDrop, Warning, TEXT("GroupTable ID %d 없음"), GroupID);
		return FString();
	}

	int32 TotalWeight = 0;
	for (const FLSGroupTableRow* Row : *Entries)
	{
		TotalWeight += Row->Group_Weight;
	}

	int32 Roll = FMath::RandRange(1, TotalWeight);
	int32 Accumulated = 0;

	for (const FLSGroupTableRow* Row : *Entries)
	{
		Accumulated += Row->Group_Weight;
		if (Roll <= Accumulated)
		{
			return Row->Group_Item_ID;
		}
	}

	return (*Entries).Last()->Group_Item_ID;
}

TArray<FLSDropResult> ULSDropSubsystem::OpenRootingObject(const FString& RootingObjectRowName)
{
	TArray<FLSDropResult> Results;

	if (!RootingObjectTable)
	{
		UE_LOG(LogDrop, Warning, TEXT("RootingObjectTable 없음"));
		return Results;
	}

	const FLSRootingObjectRow* Row = RootingObjectTable->FindRow<FLSRootingObjectRow>(
		FName(*RootingObjectRowName), TEXT("OpenRootingObject"));
	if (!Row)
	{
		UE_LOG(LogDrop, Warning, TEXT("RootingObject '%s' 없음"), *RootingObjectRowName);
		return Results;
	}

	UE_LOG(LogDrop, Log, TEXT("루팅 오브젝트 [%s] %s (타입=%d) 열림"),
		*RootingObjectRowName, *Row->Rooting_Object_Name, Row->Rooting_Object_Type);

	if (Row->Drop_Table_ID > 0)
	{
		Results = RollDropTable(Row->Drop_Table_ID);
	}
	else
	{
		UE_LOG(LogDrop, Log, TEXT("  Drop_Table_ID 미설정"));
	}

	UE_LOG(LogDrop, Log, TEXT("  총 %d개 아이템 드랍"), Results.Num());
	return Results;
}

void ULSDropSubsystem::TestDrop(const FString& RootingObjectRowName)
{
	UE_LOG(LogDrop, Log, TEXT("========== 드랍 테스트: %s =========="), *RootingObjectRowName);
	TArray<FLSDropResult> Results = OpenRootingObject(RootingObjectRowName);

	UE_LOG(LogDrop, Log, TEXT("---------- 결과 ----------"));
	for (const FLSDropResult& R : Results)
	{
		UE_LOG(LogDrop, Log, TEXT("  [%s] %s x%d"), *R.ItemRowName, *R.ItemName, R.Amount);
	}
	UE_LOG(LogDrop, Log, TEXT("==========  끝  =========="));
}

FString ULSDropSubsystem::FindItemName(const FString& ItemRowName) const
{
	if (ItemRowName.IsEmpty()) return TEXT("(없음)");

	const FName RowFName(*ItemRowName);

	if (ItemRowName.StartsWith(TEXT("C_")) && ChipTable)
	{
		if (const FLSChipRow* Row = ChipTable->FindRow<FLSChipRow>(RowFName, TEXT("")))
			return Row->Item_Name;
	}
	else if (ItemRowName.StartsWith(TEXT("W_")) && WeaponTable)
	{
		if (const FLSWeaponRow* Row = WeaponTable->FindRow<FLSWeaponRow>(RowFName, TEXT("")))
			return Row->Item_Name;
	}
	else if (ItemRowName.StartsWith(TEXT("A_")) && ArmorTable)
	{
		if (const FLSArmorRow* Row = ArmorTable->FindRow<FLSArmorRow>(RowFName, TEXT("")))
			return Row->Item_Name;
	}
	else if (ItemRowName.StartsWith(TEXT("I_")) && ItemTable)
	{
		if (const FLSItemRow* Row = ItemTable->FindRow<FLSItemRow>(RowFName, TEXT("")))
			return Row->Item_Name;
	}

	return FString::Printf(TEXT("Unknown_%s"), *ItemRowName);
}
