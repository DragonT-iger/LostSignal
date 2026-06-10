#include "Session/LSSaveSubsystem.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSChipStats.h"
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

constexpr int32 SaveDefaultMaxInventorySlotCount = 10;
constexpr int32 ChipEquipmentSlotCount = 10;

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
		FLSSessionItem IgnoredRemainingItem;
		LSInventorySlotUtils::TryAddItemsToSlotArray(Inv, NewItem.ItemRowName, NewItem.Amount, GetMaxInventorySlotCount(), NewItem.ChipStats, IgnoredRemainingItem);
	}

	UE_LOG(LogLS, Log, TEXT("[Save] Inventory updated: added %d entries, total slots %d"), Items.Num(), Inv.Num());
	Save();
}

bool ULSSaveSubsystem::TryAddToInventory(const FName ItemRowName, const int32 Amount, const TArray<FLSChipResolvedStat>& ChipStats, FLSSessionItem& OutRemainingItem)
{
	OutRemainingItem = FLSSessionItem();
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot add inventory item because SaveData is missing."));
		return false;
	}

	const bool bChanged = LSInventorySlotUtils::TryAddItemsToSlotArray(GetMutableInventory(), ItemRowName, Amount, GetMaxInventorySlotCount(), ChipStats, OutRemainingItem);
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

const TArray<FLSSessionItem>& ULSSaveSubsystem::GetChipEquipmentSlots() const
{
	static TArray<FLSSessionItem> Empty;
	return SaveData ? SaveData->ChipEquipmentSlots : Empty;
}

int32 ULSSaveSubsystem::GetMaxInventorySlotCount() const
{
	return SaveDefaultMaxInventorySlotCount + GetCarryingProtocolSlotBonus(TEXT("Inventory"));
}

int32 ULSSaveSubsystem::GetMaxSafeStashSlotCount() const
{
	constexpr int32 SaveMaxSafeStashSlotCount = 4;
	return FMath::Clamp(GetCarryingProtocolSlotBonus(TEXT("Protected_Inventory")), 0, SaveMaxSafeStashSlotCount);
}

float ULSSaveSubsystem::GetChipSignalGaugePercent() const
{
	return SaveData ? FMath::Clamp(SaveData->ChipSignalGaugePercent, 0.0f, 1.0f) : 1.0f;
}

void ULSSaveSubsystem::SetChipSignalGaugePercent(const float Percent)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot set chip signal gauge because SaveData is missing."));
		return;
	}

	const float ClampedPercent = FMath::Clamp(Percent, 0.0f, 1.0f);
	if (FMath::IsNearlyEqual(SaveData->ChipSignalGaugePercent, ClampedPercent))
	{
		return;
	}

	SaveData->ChipSignalGaugePercent = ClampedPercent;
	Save();
}

bool ULSSaveSubsystem::EquipChipFromStoredSlot(const ELSInventorySlotArea SourceArea, const int32 SourceIndex, const int32 EquipmentIndex)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot equip chip because SaveData is missing."));
		return false;
	}

	if (EquipmentIndex < 0 || EquipmentIndex >= ChipEquipmentSlotCount)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot equip chip because equipment index is invalid. Index=%d"), EquipmentIndex);
		return false;
	}

	TArray<FLSSessionItem>* SourceSlots = GetMutableStoredSlots(SourceArea);
	if (!SourceSlots || !SourceSlots->IsValidIndex(SourceIndex) || !LSInventorySlotUtils::IsFilled((*SourceSlots)[SourceIndex]))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot equip chip because source slot is invalid. Area=%d Index=%d"),
			static_cast<int32>(SourceArea),
			SourceIndex);
		return false;
	}

	FLSSessionItem& SourceSlot = (*SourceSlots)[SourceIndex];
	if (!SourceSlot.ItemRowName.ToString().StartsWith(TEXT("Chip_")))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot equip non-chip item '%s'."), *SourceSlot.ItemRowName.ToString());
		return false;
	}

	EnsureChipEquipmentSlots();
	FLSSessionItem& EquipmentSlot = SaveData->ChipEquipmentSlots[EquipmentIndex];
	if (LSInventorySlotUtils::IsFilled(EquipmentSlot) && !EquipmentSlot.ItemRowName.ToString().StartsWith(TEXT("Chip_")))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot swap equipment slot %d because it contains non-chip item '%s'."),
			EquipmentIndex,
			*EquipmentSlot.ItemRowName.ToString());
		return false;
	}

	if (LSInventorySlotUtils::IsFilled(EquipmentSlot))
	{
		Swap(SourceSlot, EquipmentSlot);
	}
	else
	{
		EquipmentSlot = SourceSlot;
		SourceSlot = LSInventorySlotUtils::MakeEmptyItem();
	}

	Save();
	return true;
}

bool ULSSaveSubsystem::DropChipEquipmentSlot(const int32 FromEquipmentIndex, const int32 ToEquipmentIndex)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop chip equipment slot because SaveData is missing."));
		return false;
	}

	if (FromEquipmentIndex < 0 || FromEquipmentIndex >= ChipEquipmentSlotCount || ToEquipmentIndex < 0 || ToEquipmentIndex >= ChipEquipmentSlotCount)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop chip equipment slot because index is invalid. From=%d To=%d"),
			FromEquipmentIndex,
			ToEquipmentIndex);
		return false;
	}

	EnsureChipEquipmentSlots();
	const FLSSessionItem& FromSlot = SaveData->ChipEquipmentSlots[FromEquipmentIndex];
	if (!LSInventorySlotUtils::IsFilled(FromSlot))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop chip equipment slot because source is empty. From=%d To=%d"),
			FromEquipmentIndex,
			ToEquipmentIndex);
		return false;
	}

	if (!FromSlot.ItemRowName.ToString().StartsWith(TEXT("Chip_")))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop non-chip equipment item '%s'."), *FromSlot.ItemRowName.ToString());
		return false;
	}

	const FLSSessionItem& ToSlot = SaveData->ChipEquipmentSlots[ToEquipmentIndex];
	if (LSInventorySlotUtils::IsFilled(ToSlot) && !ToSlot.ItemRowName.ToString().StartsWith(TEXT("Chip_")))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop to equipment slot %d because it contains non-chip item '%s'."),
			ToEquipmentIndex,
			*ToSlot.ItemRowName.ToString());
		return false;
	}

	const bool bDropped = LSInventorySlotUtils::DropSlot(
		SaveData->ChipEquipmentSlots,
		FromEquipmentIndex,
		SaveData->ChipEquipmentSlots,
		ToEquipmentIndex,
		ChipEquipmentSlotCount);
	if (bDropped)
	{
		Save();
	}
	return bDropped;
}

bool ULSSaveSubsystem::UnequipChipToWarehouse(const int32 EquipmentIndex)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot unequip chip because SaveData is missing."));
		return false;
	}

	if (EquipmentIndex < 0 || EquipmentIndex >= ChipEquipmentSlotCount)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot unequip chip because equipment index is invalid. Index=%d"), EquipmentIndex);
		return false;
	}

	EnsureChipEquipmentSlots();
	FLSSessionItem& EquipmentSlot = SaveData->ChipEquipmentSlots[EquipmentIndex];
	if (!LSInventorySlotUtils::IsFilled(EquipmentSlot))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot unequip chip because equipment slot is empty. Index=%d"), EquipmentIndex);
		return false;
	}

	if (!EquipmentSlot.ItemRowName.ToString().StartsWith(TEXT("Chip_")))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot unequip non-chip item '%s'."), *EquipmentSlot.ItemRowName.ToString());
		return false;
	}

	FLSSessionItem RemainingItem;
	const bool bMoved = LSInventorySlotUtils::TryAddItemsToSlotArray(
		SaveData->WarehouseItems,
		EquipmentSlot.ItemRowName,
		EquipmentSlot.Amount,
		MAX_int32,
		EquipmentSlot.ChipStats,
		RemainingItem);
	if (!bMoved || LSInventorySlotUtils::IsFilled(RemainingItem))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot unequip chip because warehouse add failed. Index=%d Row=%s"),
			EquipmentIndex,
			*EquipmentSlot.ItemRowName.ToString());
		return false;
	}

	EquipmentSlot = LSInventorySlotUtils::MakeEmptyItem();
	Save();
	return true;
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

	if (FromArea == ELSInventorySlotArea::Safe && FromIndex >= GetMaxSafeStashSlotCount())
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop stored slot because source safe slot is locked. Index=%d"), FromIndex);
		return false;
	}

	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory
		? GetMaxInventorySlotCount()
		: (ToArea == ELSInventorySlotArea::Safe ? GetMaxSafeStashSlotCount() : INDEX_NONE);
	const bool bChanged = LSInventorySlotUtils::DropSlot(*FromSlots, FromIndex, *ToSlots, ToIndex, ToMaxSlotCount);
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

	if (FromArea == ELSInventorySlotArea::Safe && FromIndex >= GetMaxSafeStashSlotCount())
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot transfer stored slot because source safe slot is locked. Index=%d"), FromIndex);
		return false;
	}

	FLSSessionItem& FromSlot = (*FromSlots)[FromIndex];
	FLSSessionItem RemainingItem;
	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory
		? GetMaxInventorySlotCount()
		: (ToArea == ELSInventorySlotArea::Safe ? GetMaxSafeStashSlotCount() : MAX_int32);
	if (!LSInventorySlotUtils::TryAddItemsToSlotArray(*ToSlots, FromSlot.ItemRowName, FromSlot.Amount, ToMaxSlotCount, FromSlot.ChipStats, RemainingItem))
	{
		return false;
	}

	FromSlot = RemainingItem;
	Save();
	return true;
}

bool ULSSaveSubsystem::TransferAllInventoryToWarehouse(const int32 WarehouseMaxSlotCount, bool& bOutStoppedBecauseFull)
{
	bOutStoppedBecauseFull = false;
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot store all inventory items because SaveData is missing."));
		return false;
	}

	if (WarehouseMaxSlotCount <= 0)
	{
		bOutStoppedBecauseFull = true;
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot store all inventory items because warehouse has no available slots."));
		return false;
	}

	bool bChanged = false;
	for (int32 InventoryIndex = 0; InventoryIndex < SaveData->Inventory.Num(); ++InventoryIndex)
	{
		FLSSessionItem& InventorySlot = SaveData->Inventory[InventoryIndex];
		if (!LSInventorySlotUtils::IsFilled(InventorySlot))
		{
			continue;
		}

		FLSSessionItem RemainingItem;
		const bool bAddedAny = LSInventorySlotUtils::TryAddItemsToSlotArray(
			SaveData->WarehouseItems,
			InventorySlot.ItemRowName,
			InventorySlot.Amount,
			WarehouseMaxSlotCount,
			InventorySlot.ChipStats,
			RemainingItem);

		if (!bAddedAny)
		{
			bOutStoppedBecauseFull = true;
			UE_LOG(LogLS, Warning, TEXT("[Save] Warehouse is full while storing all inventory items. InventoryIndex=%d Row=%s Amount=%d"),
				InventoryIndex,
				*InventorySlot.ItemRowName.ToString(),
				InventorySlot.Amount);
			break;
		}

		bChanged = true;
		InventorySlot = RemainingItem;
		if (LSInventorySlotUtils::IsFilled(RemainingItem))
		{
			bOutStoppedBecauseFull = true;
			UE_LOG(LogLS, Warning, TEXT("[Save] Warehouse became full while storing all inventory items. InventoryIndex=%d Row=%s RemainingAmount=%d"),
				InventoryIndex,
				*RemainingItem.ItemRowName.ToString(),
				RemainingItem.Amount);
			break;
		}
	}

	if (bChanged)
	{
		Save();
	}

	return bChanged;
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

	const int32 MaxSlotCount = SlotArea == ELSInventorySlotArea::Inventory
		? GetMaxInventorySlotCount()
		: (SlotArea == ELSInventorySlotArea::Safe ? GetMaxSafeStashSlotCount() : INDEX_NONE);
	if (MaxSlotCount != INDEX_NONE && SlotIndex >= MaxSlotCount)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace stored slot because index exceeds max. Area=%d Index=%d Max=%d"),
			static_cast<int32>(SlotArea), SlotIndex, MaxSlotCount);
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
			EnsureChipEquipmentSlots();
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
	EnsureChipEquipmentSlots();
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

void ULSSaveSubsystem::EnsureChipEquipmentSlots()
{
	if (!SaveData)
	{
		return;
	}

	while (SaveData->ChipEquipmentSlots.Num() < ChipEquipmentSlotCount)
	{
		SaveData->ChipEquipmentSlots.Add(LSInventorySlotUtils::MakeEmptyItem());
	}

	if (SaveData->ChipEquipmentSlots.Num() > ChipEquipmentSlotCount)
	{
		SaveData->ChipEquipmentSlots.SetNum(ChipEquipmentSlotCount);
	}
}

int32 ULSSaveSubsystem::GetCarryingProtocolSlotBonus(const FName EnableName) const
{
	if (!SaveData || EnableName.IsNone())
	{
		return 0;
	}

	UGameInstance* GameInstance = GetGameInstance();
	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	if (!GameDataSubsystem)
	{
		return 0;
	}

	const int32 InactiveSlotCount = LSChipStats::ResolveInactiveSignalSlotCount(GetChipSignalGaugePercent());
	const TArray<FLSSessionItem> ActiveEquipmentItems = LSChipStats::BuildSignalActiveEquipmentItems(SaveData->ChipEquipmentSlots, InactiveSlotCount);
	const int32 CurrentCarrying = LSChipStats::AggregateChipProtocolTotals(ActiveEquipmentItems, this).Carrying;
	const int32 PreviousCarrying = LSChipStats::AggregateChipProtocolTotals(SaveData->ChipEquipmentSlots, this).Carrying;
	return GameDataSubsystem->GetVisibleProtocolEnableValueSum(
		ELSProtocolType::Carrying,
		EnableName,
		CurrentCarrying,
		PreviousCarrying,
		TEXT("SaveCarryingSlotBonus"));
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
	RootObject->SetNumberField(TEXT("chipSignalGaugePercent"), GetChipSignalGaugePercent());

	auto AddSessionItemFields = [](const TSharedRef<FJsonObject>& ItemObject, const FLSSessionItem& Item, const int32 SlotIndex)
	{
		ItemObject->SetNumberField(TEXT("slotIndex"), SlotIndex);
		ItemObject->SetStringField(TEXT("itemRowName"), Item.ItemRowName.ToString());
		ItemObject->SetNumberField(TEXT("amount"), Item.Amount);

		if (!Item.ChipStats.IsEmpty())
		{
			TArray<TSharedPtr<FJsonValue>> ChipStatsArray;
			ChipStatsArray.Reserve(Item.ChipStats.Num());
			for (const FLSChipResolvedStat& Stat : Item.ChipStats)
			{
				TSharedRef<FJsonObject> StatObject = MakeShared<FJsonObject>();
				StatObject->SetStringField(TEXT("statKey"), Stat.StatKey.ToString());
				StatObject->SetNumberField(TEXT("value"), Stat.Value);
				ChipStatsArray.Add(MakeShared<FJsonValueObject>(StatObject));
			}
			ItemObject->SetArrayField(TEXT("chipStats"), ChipStatsArray);
		}
	};

	auto AddSlotArrayField = [&RootObject, &AddSessionItemFields](const TCHAR* FieldName, const TArray<FLSSessionItem>& Items)
	{
		TArray<TSharedPtr<FJsonValue>> JsonArray;
		JsonArray.Reserve(Items.Num());
		for (const FLSSessionItem& Item : Items)
		{
			TSharedRef<FJsonObject> ItemObject = MakeShared<FJsonObject>();
			AddSessionItemFields(ItemObject, Item, JsonArray.Num());
			JsonArray.Add(MakeShared<FJsonValueObject>(ItemObject));
		}

		RootObject->SetArrayField(FieldName, JsonArray);
	};

	AddSlotArrayField(TEXT("inventory"), SaveData->Inventory);
	AddSlotArrayField(TEXT("warehouseItems"), SaveData->WarehouseItems);
	AddSlotArrayField(TEXT("safeStash"), SaveData->SafeStash);
	AddSlotArrayField(TEXT("chipEquipmentSlots"), SaveData->ChipEquipmentSlots);

	TArray<TSharedPtr<FJsonValue>> ActiveRaidLoadoutArray;
	ActiveRaidLoadoutArray.Reserve(SaveData->ActiveRaidLoadout.Num());
	for (const FLSSessionItem& Item : SaveData->ActiveRaidLoadout)
	{
		TSharedRef<FJsonObject> ItemObject = MakeShared<FJsonObject>();
		AddSessionItemFields(ItemObject, Item, ActiveRaidLoadoutArray.Num());
		ActiveRaidLoadoutArray.Add(MakeShared<FJsonValueObject>(ItemObject));
	}

	RootObject->SetArrayField(TEXT("activeRaidLoadout"), ActiveRaidLoadoutArray);

	TArray<TSharedPtr<FJsonValue>> ActiveRaidConsumedItemsArray;
	ActiveRaidConsumedItemsArray.Reserve(SaveData->ActiveRaidConsumedItems.Num());
	for (const FLSSessionItem& Item : SaveData->ActiveRaidConsumedItems)
	{
		TSharedRef<FJsonObject> ItemObject = MakeShared<FJsonObject>();
		AddSessionItemFields(ItemObject, Item, ActiveRaidConsumedItemsArray.Num());
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
