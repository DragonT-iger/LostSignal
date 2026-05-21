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
bool IsFilledSaveSlot(const FLSSessionItem& Item)
{
	return !Item.ItemRowName.IsNone() && Item.Amount > 0;
}

FLSSessionItem MakeEmptySaveSlot()
{
	return FLSSessionItem();
}

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

	for (FLSSessionItem& Slot : Slots)
	{
		if (Amount <= 0)
		{
			return;
		}

		if (IsFilledSaveSlot(Slot))
		{
			continue;
		}

		Slot.ItemRowName = ItemRowName;
		Slot.Amount = FMath::Min(Amount, MaxStack);
		Amount -= Slot.Amount;
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
		if (!IsFilledSaveSlot(OldSlot))
		{
			Slots.Add(MakeEmptySaveSlot());
			continue;
		}

		const int32 MaxStack = ResolveItemMaxStackForSave(OldSlot.ItemRowName);
		FLSSessionItem NormalizedSlot;
		NormalizedSlot.ItemRowName = OldSlot.ItemRowName;
		NormalizedSlot.Amount = FMath::Min(OldSlot.Amount, MaxStack);
		Slots.Add(NormalizedSlot);

		const int32 OverflowAmount = OldSlot.Amount - NormalizedSlot.Amount;
		if (OverflowAmount > 0)
		{
			AddItemsToSlotArray(Slots, OldSlot.ItemRowName, OverflowAmount);
		}
	}
}

void RemoveItemsFromSlotArray(TArray<FLSSessionItem>& Slots, const FName ItemRowName, int32 Amount)
{
	if (ItemRowName.IsNone() || Amount <= 0)
	{
		return;
	}

	for (FLSSessionItem& Slot : Slots)
	{
		if (Amount <= 0)
		{
			break;
		}

		if (Slot.ItemRowName != ItemRowName || Slot.Amount <= 0)
		{
			continue;
		}

		const int32 RemoveAmount = FMath::Min(Amount, Slot.Amount);
		Slot.Amount -= RemoveAmount;
		Amount -= RemoveAmount;
	}

	for (FLSSessionItem& Slot : Slots)
	{
		if (Slot.Amount <= 0)
		{
			Slot = MakeEmptySaveSlot();
		}
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
	const int32 OriginalSlotCount = Slots.Num();
	TMap<FName, int32> AmountByRowName;
	for (const FLSSessionItem& Slot : Slots)
	{
		if (!IsFilledSaveSlot(Slot))
		{
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

	while (Slots.Num() < OriginalSlotCount)
	{
		Slots.Add(MakeEmptySaveSlot());
	}
}

void EnsureSaveSlotIndex(TArray<FLSSessionItem>& Slots, const int32 SlotIndex)
{
	while (Slots.Num() <= SlotIndex)
	{
		Slots.Add(MakeEmptySaveSlot());
	}
}
}

void ULSSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Load();
}

void ULSSaveSubsystem::AddToInventory(const TArray<FLSSessionItem>& Items)
{
	if (!SaveData || Items.IsEmpty())
	{
		return;
	}

	TArray<FLSSessionItem>& Inv = GetMutableInventory();
	for (const FLSSessionItem& NewItem : Items)
	{
		AddItemsToSlotArray(Inv, NewItem.ItemRowName, NewItem.Amount);
	}

	UE_LOG(LogLS, Log, TEXT("[Save] Inventory updated: added %d entries, total slots %d"), Items.Num(), Inv.Num());
	Save();
}

void ULSSaveSubsystem::ReplaceInventory(const TArray<FLSSessionItem>& Items)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace inventory because SaveData is missing."));
		return;
	}

	SaveData->Inventory = Items;
	NormalizeSlotArray(SaveData->Inventory);

	UE_LOG(LogLS, Log, TEXT("[Save] Inventory replaced. Total slots: %d"), SaveData->Inventory.Num());
	Save();
}

void ULSSaveSubsystem::ReplaceWarehouseItems(const TArray<FLSSessionItem>& Items)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace warehouse because SaveData is missing."));
		return;
	}

	SaveData->WarehouseItems = Items;
	NormalizeSlotArray(SaveData->WarehouseItems);

	UE_LOG(LogLS, Log, TEXT("[Save] Warehouse replaced. Total slots: %d"), SaveData->WarehouseItems.Num());
	Save();
}

void ULSSaveSubsystem::ReplaceSafeStash(const TArray<FLSSessionItem>& Items)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace safe stash because SaveData is missing."));
		return;
	}

	SaveData->SafeStash = Items;
	NormalizeSlotArray(SaveData->SafeStash);

	UE_LOG(LogLS, Log, TEXT("[Save] Safe stash replaced. Total slots: %d"), SaveData->SafeStash.Num());
	Save();
}

const TArray<FLSSessionItem>& ULSSaveSubsystem::GetInventory() const
{
	static TArray<FLSSessionItem> Empty;
	return SaveData ? SaveData->Inventory : Empty;
}

const TArray<FLSSessionItem>& ULSSaveSubsystem::GetWarehouseItems() const
{
	static TArray<FLSSessionItem> Empty;
	return SaveData ? SaveData->WarehouseItems : Empty;
}

const TArray<FLSSessionItem>& ULSSaveSubsystem::GetSafeStash() const
{
	static TArray<FLSSessionItem> Empty;
	return SaveData ? SaveData->SafeStash : Empty;
}

void ULSSaveSubsystem::SortInventory()
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot sort inventory because SaveData is missing."));
		return;
	}

	SortAndCompactSlotArray(SaveData->Inventory);
	UE_LOG(LogLS, Log, TEXT("[Save] Inventory sorted and compacted. Total slots: %d"), SaveData->Inventory.Num());
	Save();
}

void ULSSaveSubsystem::SortWarehouse()
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot sort warehouse because SaveData is missing."));
		return;
	}

	SortAndCompactSlotArray(SaveData->WarehouseItems);
	UE_LOG(LogLS, Log, TEXT("[Save] Warehouse sorted and compacted. Total slots: %d"), SaveData->WarehouseItems.Num());
	Save();
}

bool ULSSaveSubsystem::DropStoredSlot(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop stored slot because SaveData is missing."));
		return false;
	}

	TArray<FLSSessionItem>* FromSlots = GetMutableStoredSlots(FromArea);
	TArray<FLSSessionItem>* ToSlots = GetMutableStoredSlots(ToArea);
	if (!FromSlots || !ToSlots || !FromSlots->IsValidIndex(FromIndex) || !IsFilledSaveSlot((*FromSlots)[FromIndex]) || ToIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop stored slot. FromArea=%d From=%d ToArea=%d To=%d"),
			static_cast<int32>(FromArea), FromIndex, static_cast<int32>(ToArea), ToIndex);
		return false;
	}

	if (FromSlots == ToSlots && FromIndex == ToIndex)
	{
		return true;
	}

	EnsureSaveSlotIndex(*ToSlots, ToIndex);

	FLSSessionItem& FromSlot = (*FromSlots)[FromIndex];
	FLSSessionItem& ToSlot = (*ToSlots)[ToIndex];

	if (!IsFilledSaveSlot(ToSlot))
	{
		ToSlot = FromSlot;
		FromSlot = MakeEmptySaveSlot();
		Save();
		return true;
	}

	if (FromSlot.ItemRowName == ToSlot.ItemRowName)
	{
		const int32 MaxStack = ResolveItemMaxStackForSave(FromSlot.ItemRowName);
		const int32 AddAmount = FMath::Min(FromSlot.Amount, MaxStack - ToSlot.Amount);
		if (AddAmount <= 0)
		{
			return false;
		}

		ToSlot.Amount += AddAmount;
		FromSlot.Amount -= AddAmount;
		if (FromSlot.Amount <= 0)
		{
			FromSlot = MakeEmptySaveSlot();
		}
		Save();
		return true;
	}

	Swap(FromSlot, ToSlot);
	Save();
	return true;
}

void ULSSaveSubsystem::BeginRaidSave(const TArray<FLSSessionItem>& Loadout)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot begin raid save because SaveData is missing."));
		return;
	}

	SaveData->bRaidSaveActive = true;
	SaveData->ActiveRaidLoadout = Loadout;
	SaveData->ActiveRaidConsumedItems.Reset();
	Save();
}

void ULSSaveSubsystem::UpdateRaidConsumedItems(const TArray<FLSSessionItem>& ConsumedItems)
{
	if (!SaveData || !SaveData->bRaidSaveActive)
	{
		return;
	}

	SaveData->ActiveRaidConsumedItems = ConsumedItems;
	Save();
}

void ULSSaveSubsystem::ClearRaidSave()
{
	if (!SaveData)
	{
		return;
	}

	SaveData->bRaidSaveActive = false;
	SaveData->ActiveRaidLoadout.Reset();
	SaveData->ActiveRaidConsumedItems.Reset();
	Save();
}

void ULSSaveSubsystem::Load()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		SaveData = Cast<ULSSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		if (SaveData)
		{
			MigrateInventory();
			NormalizeSlotArray(SaveData->Inventory);
			NormalizeSlotArray(SaveData->WarehouseItems);
			NormalizeSlotArray(SaveData->SafeStash);
			ResolveInterruptedRaid();
			Save();
		}
		UE_LOG(LogLS, Log, TEXT("[Save] Loaded inventory slots: %d, warehouse slots: %d, safe slots: %d"),
			SaveData ? SaveData->Inventory.Num() : 0,
			SaveData ? SaveData->WarehouseItems.Num() : 0,
			SaveData ? SaveData->SafeStash.Num() : 0);
		return;
	}

	SaveData = Cast<ULSSaveGame>(UGameplayStatics::CreateSaveGameObject(ULSSaveGame::StaticClass()));
	UE_LOG(LogLS, Log, TEXT("[Save] Created new save object"));
}

void ULSSaveSubsystem::MigrateInventory()
{
	if (!SaveData || SaveData->bInventoryMigrated)
	{
		return;
	}

	if (SaveData->Inventory.IsEmpty())
	{
		// Player1Inventory 있으면 우선, 없으면 Stash에서 마이그레이션
		if (!SaveData->Player1Inventory.IsEmpty())
		{
			NormalizeSlotArray(SaveData->Player1Inventory);
			SaveData->Inventory = SaveData->Player1Inventory;
		}
		else if (!SaveData->Stash.IsEmpty())
		{
			NormalizeSlotArray(SaveData->Stash);
			SaveData->Inventory = SaveData->Stash;
		}
	}

	SaveData->bInventoryMigrated = true;
	UE_LOG(LogLS, Log, TEXT("[Save] Migrated legacy data into Inventory."));
}

void ULSSaveSubsystem::ResolveInterruptedRaid()
{
	if (!SaveData || !SaveData->bRaidSaveActive)
	{
		return;
	}

	TArray<FLSSessionItem> RecoveredStash = SaveData->ActiveRaidLoadout;
	NormalizeSlotArray(RecoveredStash);

	for (const FLSSessionItem& ConsumedItem : SaveData->ActiveRaidConsumedItems)
	{
		RemoveItemsFromSlotArray(RecoveredStash, ConsumedItem.ItemRowName, ConsumedItem.Amount);
	}

	SaveData->Inventory = RecoveredStash;
	SaveData->bRaidSaveActive = false;
	SaveData->ActiveRaidLoadout.Reset();
	SaveData->ActiveRaidConsumedItems.Reset();

	UE_LOG(LogLS, Log, TEXT("[Save] Interrupted raid resolved. Recovered inventory slots: %d"), SaveData->Inventory.Num());
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

TArray<FLSSessionItem>& ULSSaveSubsystem::GetMutableInventory()
{
	return SaveData->Inventory;
}

TArray<FLSSessionItem>* ULSSaveSubsystem::GetMutableStoredSlots(const ELSInventorySlotArea SlotArea)
{
	if (!SaveData)
	{
		return nullptr;
	}

	switch (SlotArea)
	{
	case ELSInventorySlotArea::Inventory:
		return &SaveData->Inventory;
	case ELSInventorySlotArea::Safe:
		return &SaveData->SafeStash;
	case ELSInventorySlotArea::Warehouse:
		return &SaveData->WarehouseItems;
	default:
		return nullptr;
	}
}

void ULSSaveSubsystem::SaveDebugJson() const
{
	if (!SaveData)
	{
		return;
	}

	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(TEXT("slotName"), SlotName);
	RootObject->SetBoolField(TEXT("raidSaveActive"), SaveData->bRaidSaveActive);

	auto AddSlotArrayField = [&RootObject](const TCHAR* FieldName, const TArray<FLSSessionItem>& Items)
	{
		TArray<TSharedPtr<FJsonValue>> JsonArray;
		JsonArray.Reserve(Items.Num());
		for (const FLSSessionItem& Item : Items)
		{
			TSharedRef<FJsonObject> ItemObject = MakeShared<FJsonObject>();
			ItemObject->SetNumberField(TEXT("slotIndex"), JsonArray.Num());
			ItemObject->SetStringField(TEXT("itemRowName"), Item.ItemRowName.ToString());
			ItemObject->SetNumberField(TEXT("amount"), Item.Amount);
			JsonArray.Add(MakeShared<FJsonValueObject>(ItemObject));
		}

		RootObject->SetArrayField(FieldName, JsonArray);
	};

	AddSlotArrayField(TEXT("inventory"), SaveData->Inventory);
	AddSlotArrayField(TEXT("warehouseItems"), SaveData->WarehouseItems);
	AddSlotArrayField(TEXT("safeStash"), SaveData->SafeStash);

	TArray<TSharedPtr<FJsonValue>> ActiveRaidLoadoutArray;
	ActiveRaidLoadoutArray.Reserve(SaveData->ActiveRaidLoadout.Num());
	for (const FLSSessionItem& Item : SaveData->ActiveRaidLoadout)
	{
		TSharedRef<FJsonObject> ItemObject = MakeShared<FJsonObject>();
		ItemObject->SetNumberField(TEXT("slotIndex"), ActiveRaidLoadoutArray.Num());
		ItemObject->SetStringField(TEXT("itemRowName"), Item.ItemRowName.ToString());
		ItemObject->SetNumberField(TEXT("amount"), Item.Amount);
		ActiveRaidLoadoutArray.Add(MakeShared<FJsonValueObject>(ItemObject));
	}

	RootObject->SetArrayField(TEXT("activeRaidLoadout"), ActiveRaidLoadoutArray);

	TArray<TSharedPtr<FJsonValue>> ActiveRaidConsumedItemsArray;
	ActiveRaidConsumedItemsArray.Reserve(SaveData->ActiveRaidConsumedItems.Num());
	for (const FLSSessionItem& Item : SaveData->ActiveRaidConsumedItems)
	{
		TSharedRef<FJsonObject> ItemObject = MakeShared<FJsonObject>();
		ItemObject->SetNumberField(TEXT("slotIndex"), ActiveRaidConsumedItemsArray.Num());
		ItemObject->SetStringField(TEXT("itemRowName"), Item.ItemRowName.ToString());
		ItemObject->SetNumberField(TEXT("amount"), Item.Amount);
		ActiveRaidConsumedItemsArray.Add(MakeShared<FJsonValueObject>(ItemObject));
	}

	RootObject->SetArrayField(TEXT("activeRaidConsumedItems"), ActiveRaidConsumedItemsArray);

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
