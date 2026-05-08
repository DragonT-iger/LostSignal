#include "Session/LSSaveSubsystem.h"
#include "Session/LSSaveGame.h"
#include "Data/LSArmorRow.h"
#include "Data/LSChipRow.h"
#include "Data/LSDropSettings.h"
#include "Data/LSItemRow.h"
#include "Data/LSWeaponRow.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Policies\PrettyJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

const FString ULSSaveSubsystem::SlotName = TEXT("LostSignalSave");
const FString ULSSaveSubsystem::DebugFileName = TEXT("LostSignalSave_Debug.json");

namespace
{
int32 ResolveItemMaxStackForSave(const FName ItemRowName)
{
	if (ItemRowName.IsNone())
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot resolve max stack because ItemRowName is none."));
		return 1;
	}

	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (!Settings)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot resolve max stack because LS Drop Settings is missing."));
		return 1;
	}

	const FString RowNameString = ItemRowName.ToString();
	int32 MaxStack = 1;

	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		UDataTable* Table = Settings->ChipTable.LoadSynchronous();
		const FLSChipRow* Row = Table ? Table->FindRow<FLSChipRow>(ItemRowName, TEXT("ResolveItemMaxStackForSave")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[Save] Chip row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		UDataTable* Table = Settings->WeaponTable.LoadSynchronous();
		const FLSWeaponRow* Row = Table ? Table->FindRow<FLSWeaponRow>(ItemRowName, TEXT("ResolveItemMaxStackForSave")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[Save] Weapon row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		UDataTable* Table = Settings->ArmorTable.LoadSynchronous();
		const FLSArmorRow* Row = Table ? Table->FindRow<FLSArmorRow>(ItemRowName, TEXT("ResolveItemMaxStackForSave")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[Save] Armor row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else if (RowNameString.StartsWith(TEXT("Item_")))
	{
		UDataTable* Table = Settings->ItemTable.LoadSynchronous();
		const FLSItemRow* Row = Table ? Table->FindRow<FLSItemRow>(ItemRowName, TEXT("ResolveItemMaxStackForSave")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[Save] Item row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Unknown item row prefix for max stack: %s"), *ItemRowName.ToString());
	}

	if (MaxStack <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Invalid Item_Max for %s: %d. Falling back to 1."), *ItemRowName.ToString(), MaxStack);
		return 1;
	}

	return MaxStack;
}

void AddItemsToSlotArray(TArray<FLSSessionItem>& Slots, const FName ItemRowName, int32 Amount)
{
	if (ItemRowName.IsNone() || Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot add invalid stash item. Row=%s Amount=%d"), *ItemRowName.ToString(), Amount);
		return;
	}

	const int32 MaxStack = ResolveItemMaxStackForSave(ItemRowName);
	for (FLSSessionItem& Slot : Slots)
	{
		if (Amount <= 0)
		{
			return;
		}

		if (Slot.ItemRowName != ItemRowName || Slot.Amount >= MaxStack)
		{
			continue;
		}

		const int32 AddAmount = FMath::Min(Amount, MaxStack - Slot.Amount);
		Slot.Amount += AddAmount;
		Amount -= AddAmount;
	}

	while (Amount > 0)
	{
		FLSSessionItem NewSlot;
		NewSlot.ItemRowName = ItemRowName;
		NewSlot.Amount = FMath::Min(Amount, MaxStack);
		Slots.Add(NewSlot);
		Amount -= NewSlot.Amount;
	}
}

void NormalizeSlotArray(TArray<FLSSessionItem>& Slots)
{
	TArray<FLSSessionItem> OldSlots = MoveTemp(Slots);
	Slots.Reset();

	for (const FLSSessionItem& OldSlot : OldSlots)
	{
		AddItemsToSlotArray(Slots, OldSlot.ItemRowName, OldSlot.Amount);
	}
}

int32 FindRowOrder(UDataTable* Table, const FName RowName)
{
	if (!Table)
	{
		return MAX_int32 / 2;
	}

	const TArray<FName> RowNames = Table->GetRowNames();
	const int32 RowIndex = RowNames.IndexOfByKey(RowName);
	return RowIndex == INDEX_NONE ? MAX_int32 / 2 : RowIndex;
}

int32 ResolveItemSortKeyForSave(const FName ItemRowName)
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (!Settings || ItemRowName.IsNone())
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot resolve sort key. Row=%s"), *ItemRowName.ToString());
		return MAX_int32;
	}

	const FString RowNameString = ItemRowName.ToString();
	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		return FindRowOrder(Settings->ChipTable.LoadSynchronous(), ItemRowName);
	}

	if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		return 100000 + FindRowOrder(Settings->WeaponTable.LoadSynchronous(), ItemRowName);
	}

	if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		return 200000 + FindRowOrder(Settings->ArmorTable.LoadSynchronous(), ItemRowName);
	}

	if (RowNameString.StartsWith(TEXT("Item_")))
	{
		return 300000 + FindRowOrder(Settings->ItemTable.LoadSynchronous(), ItemRowName);
	}

	UE_LOG(LogLS, Warning, TEXT("[Save] Unknown item row prefix for sort key: %s"), *ItemRowName.ToString());
	return MAX_int32;
}

void SortAndCompactSlotArray(TArray<FLSSessionItem>& Slots)
{
	TMap<FName, int32> AmountByRowName;
	for (const FLSSessionItem& Slot : Slots)
	{
		if (Slot.ItemRowName.IsNone() || Slot.Amount <= 0)
		{
			UE_LOG(LogLS, Warning, TEXT("[Save] Skipping invalid slot while sorting. Row=%s Amount=%d"), *Slot.ItemRowName.ToString(), Slot.Amount);
			continue;
		}

		AmountByRowName.FindOrAdd(Slot.ItemRowName) += Slot.Amount;
	}

	TArray<FLSSessionItem> MergedItems;
	MergedItems.Reserve(AmountByRowName.Num());
	for (const TPair<FName, int32>& Pair : AmountByRowName)
	{
		MergedItems.Add({ Pair.Key, Pair.Value });
	}

	MergedItems.Sort([](const FLSSessionItem& Left, const FLSSessionItem& Right)
	{
		const int32 LeftSortKey = ResolveItemSortKeyForSave(Left.ItemRowName);
		const int32 RightSortKey = ResolveItemSortKeyForSave(Right.ItemRowName);
		if (LeftSortKey != RightSortKey)
		{
			return LeftSortKey < RightSortKey;
		}

		return Left.ItemRowName.LexicalLess(Right.ItemRowName);
	});

	Slots.Reset();
	for (const FLSSessionItem& MergedItem : MergedItems)
	{
		AddItemsToSlotArray(Slots, MergedItem.ItemRowName, MergedItem.Amount);
	}
}
}

void ULSSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Load();
}

void ULSSaveSubsystem::AddToStash(const TArray<FLSSessionItem>& Items)
{
	if (!SaveData || Items.IsEmpty())
	{
		return;
	}

	for (const FLSSessionItem& NewItem : Items)
	{
		AddItemsToSlotArray(SaveData->Stash, NewItem.ItemRowName, NewItem.Amount);
	}

	UE_LOG(LogLS, Log, TEXT("[Save] Stash updated: added %d entries, total slots %d"),
		Items.Num(), SaveData->Stash.Num());
	Save();
}

void ULSSaveSubsystem::ReplaceStash(const TArray<FLSSessionItem>& Items)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace stash because SaveData is missing."));
		return;
	}

	SaveData->Stash.Reset();
	for (const FLSSessionItem& Item : Items)
	{
		AddItemsToSlotArray(SaveData->Stash, Item.ItemRowName, Item.Amount);
	}

	UE_LOG(LogLS, Log, TEXT("[Save] Stash replaced. Total slots: %d"), SaveData->Stash.Num());
	Save();
}

const TArray<FLSSessionItem>& ULSSaveSubsystem::GetStash() const
{
	static TArray<FLSSessionItem> Empty;
	return SaveData ? SaveData->Stash : Empty;
}

void ULSSaveSubsystem::SortStash()
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot sort stash because SaveData is missing."));
		return;
	}

	SortAndCompactSlotArray(SaveData->Stash);
	UE_LOG(LogLS, Log, TEXT("[Save] Stash sorted and compacted. Total slots: %d"), SaveData->Stash.Num());
	Save();
}

void ULSSaveSubsystem::Load()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		SaveData = Cast<ULSSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		if (SaveData)
		{
			NormalizeSlotArray(SaveData->Stash);
			Save();
		}
		UE_LOG(LogLS, Log, TEXT("[Save] Loaded stash slots: %d"),
			SaveData ? SaveData->Stash.Num() : 0);
		return;
	}

	SaveData = Cast<ULSSaveGame>(UGameplayStatics::CreateSaveGameObject(ULSSaveGame::StaticClass()));
	UE_LOG(LogLS, Log, TEXT("[Save] Created new save object"));
}

void ULSSaveSubsystem::Save()
{
	if (!SaveData)
	{
		return;
	}

	UGameplayStatics::SaveGameToSlot(SaveData, SlotName, 0);
#if !UE_BUILD_SHIPPING
	SaveDebugJson();
#endif
	UE_LOG(LogLS, Log, TEXT("[Save] Save completed"));
}

void ULSSaveSubsystem::SaveDebugJson() const
{
	if (!SaveData)
	{
		return;
	}

	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(TEXT("slotName"), SlotName);

	TArray<TSharedPtr<FJsonValue>> StashArray;
	StashArray.Reserve(SaveData->Stash.Num());

	for (const FLSSessionItem& Item : SaveData->Stash)
	{
		TSharedRef<FJsonObject> ItemObject = MakeShared<FJsonObject>();
		ItemObject->SetNumberField(TEXT("slotIndex"), StashArray.Num());
		ItemObject->SetStringField(TEXT("itemRowName"), Item.ItemRowName.ToString());
		ItemObject->SetNumberField(TEXT("amount"), Item.Amount);
		StashArray.Add(MakeShared<FJsonValueObject>(ItemObject));
	}

	RootObject->SetArrayField(TEXT("stash"), StashArray);

	FString OutputString;
	const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutputString);

	if (!FJsonSerializer::Serialize(RootObject, Writer))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Debug JSON serialize failed"));
		return;
	}

	const FString DebugFilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), DebugFileName);
	if (!FFileHelper::SaveStringToFile(OutputString, *DebugFilePath))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Debug JSON write failed: %s"), *DebugFilePath);
		return;
	}

	UE_LOG(LogLS, Log, TEXT("[Save] Debug JSON written: %s"), *DebugFilePath);
}
