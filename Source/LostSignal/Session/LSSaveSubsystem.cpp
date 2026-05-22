#include "Session/LSSaveSubsystem.h"
#include "Session/LSSaveGame.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Policies\PrettyJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"

const FString ULSSaveSubsystem::SlotName = TEXT("LostSignalSave");
const FString ULSSaveSubsystem::DebugFileName = TEXT("LostSignalSave_Debug.json");

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
		LSInventorySlotUtils::AddItemsToSlotArray(Inv, NewItem.ItemRowName, NewItem.Amount);
	}

	UE_LOG(LogLS, Log, TEXT("[Save] Inventory updated: added %d entries, total slots %d"), Items.Num(), Inv.Num());
	Save();
}

bool ULSSaveSubsystem::TryAddToInventory(const FName ItemRowName, const int32 Amount, FLSSessionItem& OutRemainingItem)
{
	OutRemainingItem = FLSSessionItem();
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot add inventory item because SaveData is missing."));
		return false;
	}

	const bool bChanged = LSInventorySlotUtils::TryAddItemsToSlotArray(GetMutableInventory(), ItemRowName, Amount, MAX_int32, OutRemainingItem);
	if (bChanged)
	{
		Save();
	}
	return bChanged;
}

void ULSSaveSubsystem::ReplaceInventory(const TArray<FLSSessionItem>& Items)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace inventory because SaveData is missing."));
		return;
	}

	SaveData->Inventory = Items;
	LSInventorySlotUtils::NormalizeSlotArray(SaveData->Inventory);

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
	LSInventorySlotUtils::NormalizeSlotArray(SaveData->WarehouseItems);

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
	LSInventorySlotUtils::NormalizeSlotArray(SaveData->SafeStash);

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

	LSInventorySlotUtils::SortAndCompactSlotArray(SaveData->Inventory);
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

	LSInventorySlotUtils::SortAndCompactSlotArray(SaveData->WarehouseItems);
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
	if (!FromSlots || !ToSlots || !FromSlots->IsValidIndex(FromIndex) || !LSInventorySlotUtils::IsFilled((*FromSlots)[FromIndex]) || ToIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop stored slot. FromArea=%d From=%d ToArea=%d To=%d"),
			static_cast<int32>(FromArea), FromIndex, static_cast<int32>(ToArea), ToIndex);
		return false;
	}

	if (FromSlots == ToSlots && FromIndex == ToIndex)
	{
		return true;
	}

	const bool bChanged = LSInventorySlotUtils::DropSlot(*FromSlots, FromIndex, *ToSlots, ToIndex);
	if (bChanged)
	{
		Save();
	}
	return bChanged;
}

bool ULSSaveSubsystem::TransferStoredSlotToArea(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot transfer stored slot because SaveData is missing."));
		return false;
	}

	TArray<FLSSessionItem>* FromSlots = GetMutableStoredSlots(FromArea);
	TArray<FLSSessionItem>* ToSlots = GetMutableStoredSlots(ToArea);
	if (!FromSlots || !ToSlots || !FromSlots->IsValidIndex(FromIndex) || !LSInventorySlotUtils::IsFilled((*FromSlots)[FromIndex]))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot transfer stored slot. FromArea=%d From=%d ToArea=%d"),
			static_cast<int32>(FromArea), FromIndex, static_cast<int32>(ToArea));
		return false;
	}

	if (FromSlots == ToSlots)
	{
		return false;
	}

	FLSSessionItem& FromSlot = (*FromSlots)[FromIndex];
	FLSSessionItem RemainingItem;
	if (!LSInventorySlotUtils::TryAddItemsToSlotArray(*ToSlots, FromSlot.ItemRowName, FromSlot.Amount, MAX_int32, RemainingItem))
	{
		return false;
	}

	FromSlot = RemainingItem;
	Save();
	return true;
}

bool ULSSaveSubsystem::GetStoredSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, FLSSessionItem& OutItem) const
{
	OutItem = FLSSessionItem();
	const TArray<FLSSessionItem>* Slots = GetStoredSlots(SlotArea);
	if (!Slots || !Slots->IsValidIndex(SlotIndex) || !LSInventorySlotUtils::IsFilled((*Slots)[SlotIndex]))
	{
		return false;
	}

	OutItem = (*Slots)[SlotIndex];
	return true;
}

bool ULSSaveSubsystem::ClearStoredSlot(const ELSInventorySlotArea SlotArea, const int32 SlotIndex)
{
	TArray<FLSSessionItem>* Slots = GetMutableStoredSlots(SlotArea);
	if (!Slots || !Slots->IsValidIndex(SlotIndex) || !LSInventorySlotUtils::IsFilled((*Slots)[SlotIndex]))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot clear stored slot. Area=%d Index=%d"), static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	(*Slots)[SlotIndex] = LSInventorySlotUtils::MakeEmptyItem();
	Save();
	return true;
}

bool ULSSaveSubsystem::ReplaceStoredSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, const FLSSessionItem& NewItem, FLSSessionItem& OutPreviousItem)
{
	OutPreviousItem = FLSSessionItem();
	if (SlotIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace stored slot because index is invalid. Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	TArray<FLSSessionItem>* Slots = GetMutableStoredSlots(SlotArea);
	if (!Slots)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace stored slot because area is invalid. Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	LSInventorySlotUtils::EnsureSlotIndex(*Slots, SlotIndex);
	OutPreviousItem = (*Slots)[SlotIndex];
	(*Slots)[SlotIndex] = NewItem;
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

FString ULSSaveSubsystem::GetResolvedSlotName() const
{
#if WITH_EDITOR
	const UGameInstance* GameInstance = GetGameInstance();
	const UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	const UPackage* WorldPackage = World ? World->GetPackage() : nullptr;
	const int32 PIEInstanceId = WorldPackage ? WorldPackage->GetPIEInstanceID() : INDEX_NONE;
	if (PIEInstanceId != INDEX_NONE)
	{
		return FString::Printf(TEXT("%s_PIE_%d"), *SlotName, PIEInstanceId);
	}
#endif

	return SlotName;
}

FString ULSSaveSubsystem::GetResolvedDebugFileName() const
{
	const FString ResolvedSlotName = GetResolvedSlotName();
	if (ResolvedSlotName == SlotName)
	{
		return DebugFileName;
	}

	return FString::Printf(TEXT("%s_Debug.json"), *ResolvedSlotName);
}

void ULSSaveSubsystem::Load()
{
	const FString ResolvedSlotName = GetResolvedSlotName();
	const FString LoadSlotName = UGameplayStatics::DoesSaveGameExist(ResolvedSlotName, 0) ? ResolvedSlotName : SlotName;

	if (UGameplayStatics::DoesSaveGameExist(LoadSlotName, 0))
	{
		SaveData = Cast<ULSSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadSlotName, 0));
		if (SaveData)
		{
			MigrateInventory();
			LSInventorySlotUtils::NormalizeSlotArray(SaveData->Inventory);
			LSInventorySlotUtils::NormalizeSlotArray(SaveData->WarehouseItems);
			LSInventorySlotUtils::NormalizeSlotArray(SaveData->SafeStash);
			ResolveInterruptedRaid();
			Save();
		}
		UE_LOG(LogLS, Log, TEXT("[Save] Loaded slot %s into %s. Inventory slots: %d, warehouse slots: %d, safe slots: %d"),
			*LoadSlotName,
			*ResolvedSlotName,
			SaveData ? SaveData->Inventory.Num() : 0,
			SaveData ? SaveData->WarehouseItems.Num() : 0,
			SaveData ? SaveData->SafeStash.Num() : 0);
		return;
	}

	SaveData = Cast<ULSSaveGame>(UGameplayStatics::CreateSaveGameObject(ULSSaveGame::StaticClass()));
	UE_LOG(LogLS, Log, TEXT("[Save] Created new save object for slot %s"), *ResolvedSlotName);
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
		LSInventorySlotUtils::NormalizeSlotArray(SaveData->Player1Inventory);
			SaveData->Inventory = SaveData->Player1Inventory;
		}
		else if (!SaveData->Stash.IsEmpty())
		{
		LSInventorySlotUtils::NormalizeSlotArray(SaveData->Stash);
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
	LSInventorySlotUtils::NormalizeSlotArray(RecoveredStash);

	for (const FLSSessionItem& ConsumedItem : SaveData->ActiveRaidConsumedItems)
	{
		LSInventorySlotUtils::RemoveItemsFromSlotArray(RecoveredStash, ConsumedItem.ItemRowName, ConsumedItem.Amount);
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

	const FString ResolvedSlotName = GetResolvedSlotName();
	UGameplayStatics::SaveGameToSlot(SaveData, ResolvedSlotName, 0);
#if !UE_BUILD_SHIPPING
	SaveDebugJson();
#endif
	UE_LOG(LogLS, Log, TEXT("[Save] Save completed: %s"), *ResolvedSlotName);
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

const TArray<FLSSessionItem>* ULSSaveSubsystem::GetStoredSlots(const ELSInventorySlotArea SlotArea) const
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
	RootObject->SetStringField(TEXT("slotName"), GetResolvedSlotName());
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

	const FString DebugFilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), GetResolvedDebugFileName());
	if (!FFileHelper::SaveStringToFile(OutputString, *DebugFilePath))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Debug JSON write failed: %s"), *DebugFilePath);
		return;
	}

	UE_LOG(LogLS, Log, TEXT("[Save] Debug JSON written: %s"), *DebugFilePath);
}
