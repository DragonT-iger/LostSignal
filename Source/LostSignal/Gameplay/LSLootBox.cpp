#include "Gameplay/LSLootBox.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSArmorRow.h"
#include "Data/LSChipRow.h"
#include "Data/LSDropSettings.h"
#include "Data/LSItemRow.h"
#include "Data/LSWeaponRow.h"
#include "Engine/DataTable.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
#include "Net/UnrealNetwork.h"

namespace
{
bool IsFilledLootSlot(const FLSDropResult& LootItem)
{
	return !LootItem.ItemRowName.IsNone() && LootItem.Amount > 0;
}

int32 ResolveItemMaxStackForLootBox(const FName ItemRowName)
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (!Settings || ItemRowName.IsNone())
	{
		UE_LOG(LogLS, Warning, TEXT("[LootBox] Cannot resolve max stack. Row=%s"), *ItemRowName.ToString());
		return 1;
	}

	const FString RowNameString = ItemRowName.ToString();
	int32 MaxStack = 1;

	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		UDataTable* Table = Settings->ChipTable.LoadSynchronous();
		const FLSChipRow* Row = Table ? Table->FindRow<FLSChipRow>(ItemRowName, TEXT("ResolveItemMaxStackForLootBox")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[LootBox] Chip row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		UDataTable* Table = Settings->WeaponTable.LoadSynchronous();
		const FLSWeaponRow* Row = Table ? Table->FindRow<FLSWeaponRow>(ItemRowName, TEXT("ResolveItemMaxStackForLootBox")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[LootBox] Weapon row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		UDataTable* Table = Settings->ArmorTable.LoadSynchronous();
		const FLSArmorRow* Row = Table ? Table->FindRow<FLSArmorRow>(ItemRowName, TEXT("ResolveItemMaxStackForLootBox")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[LootBox] Armor row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else if (RowNameString.StartsWith(TEXT("Item_")))
	{
		UDataTable* Table = Settings->ItemTable.LoadSynchronous();
		const FLSItemRow* Row = Table ? Table->FindRow<FLSItemRow>(ItemRowName, TEXT("ResolveItemMaxStackForLootBox")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[LootBox] Item row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[LootBox] Unknown item row prefix for max stack: %s"), *ItemRowName.ToString());
	}

	if (MaxStack <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[LootBox] Invalid Item_Max for %s: %d. Falling back to 1."), *ItemRowName.ToString(), MaxStack);
		return 1;
	}

	return MaxStack;
}
}

void ALSLootBox::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALSLootBox, bIsOpened);
	DOREPLIFETIME(ALSLootBox, LootResults);
}

bool ALSLootBox::CanInteract_Implementation(APawn* Interactor)
{
	return true;
}

void ALSLootBox::Interact_Implementation(APawn* Interactor)
{
	if (!HasAuthority()) return;

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	if (!bIsOpened)
	{
		ULSDropSubsystem* DropSubsystem = GI->GetSubsystem<ULSDropSubsystem>();
		if (!DropSubsystem)
		{
			UE_LOG(LogLS, Warning, TEXT("ALSLootBox: DropSubsystem 없음"));
			return;
		}

		if (RootingObjectRowName.IsNone())
		{
			UE_LOG(LogLS, Warning, TEXT("ALSLootBox: RootingObjectRowName 미설정"));
			return;
		}

		LootResults = DropSubsystem->OpenRootingObject(RootingObjectRowName);
		bIsOpened = true;
		RefreshWidgetVisibility();
		OnLootResultReceived(LootResults);
		NotifyLootResultsChanged();
		ForceNetUpdate();
	}

	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(Interactor ? Interactor->GetController() : nullptr))
	{
		const FText InteractObjectText = GetInteractText_Implementation();
		PlayerController->ShowLootDropWidget(
			InteractObjectText.IsEmpty() ? FText::FromName(RootingObjectRowName) : InteractObjectText,
			LootResults,
			this);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("ALSLootBox: Cannot show loot drop widget because interactor controller is invalid on %s."), *GetNameSafe(this));
	}
}

void ALSLootBox::OnRep_IsOpened()
{
	RefreshWidgetVisibility();
}

void ALSLootBox::OnRep_LootResults()
{
	NotifyLootResultsChanged();
}

bool ALSLootBox::DropLootSlot(const int32 FromLootSlotIndex, const int32 ToLootSlotIndex)
{
	if (!LootResults.IsValidIndex(FromLootSlotIndex) || !IsFilledLootSlot(LootResults[FromLootSlotIndex]) || !LootResults.IsValidIndex(ToLootSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop loot slot because slot data is invalid. From=%d To=%d"), FromLootSlotIndex, ToLootSlotIndex);
		return false;
	}

	if (FromLootSlotIndex == ToLootSlotIndex)
	{
		return true;
	}

	FLSDropResult& FromSlot = LootResults[FromLootSlotIndex];
	FLSDropResult& ToSlot = LootResults[ToLootSlotIndex];
	if (!IsFilledLootSlot(ToSlot))
	{
		ToSlot = FromSlot;
		ClearLootSlot(FromLootSlotIndex);
		NotifyLootResultsChanged();
		ForceNetUpdate();
		return true;
	}

	if (FromSlot.ItemRowName == ToSlot.ItemRowName)
	{
		const int32 MaxStack = ResolveItemMaxStackForLootBox(FromSlot.ItemRowName);
		const int32 AddAmount = FMath::Min(FromSlot.Amount, MaxStack - ToSlot.Amount);
		if (AddAmount <= 0)
		{
			return false;
		}

		ToSlot.Amount += AddAmount;
		FromSlot.Amount -= AddAmount;
		ToSlot.ItemText = FText::GetEmpty();
		if (FromSlot.Amount <= 0)
		{
			ClearLootSlot(FromLootSlotIndex);
		}
		else
		{
			FromSlot.ItemText = FText::GetEmpty();
		}

		NotifyLootResultsChanged();
		ForceNetUpdate();
		return true;
	}

	Swap(FromSlot, ToSlot);
	NotifyLootResultsChanged();
	ForceNetUpdate();
	return true;
}

bool ALSLootBox::TransferLootSlotToSession(const int32 LootSlotIndex, ULSRaidInventoryComponent* RaidInventory, FLSSessionItem& OutRemainingLootItem)
{
	OutRemainingLootItem = FLSSessionItem();
	if (!LootResults.IsValidIndex(LootSlotIndex) || LootResults[LootSlotIndex].ItemRowName.IsNone() || LootResults[LootSlotIndex].Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot because slot is invalid. Index=%d"), LootSlotIndex);
		return false;
	}

	if (!RaidInventory || !RaidInventory->IsRaidActive())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot because raid session is not active."));
		return false;
	}

	if (!RaidInventory->TryAddSessionItem(LootResults[LootSlotIndex].ItemRowName, LootResults[LootSlotIndex].Amount, OutRemainingLootItem))
	{
		return false;
	}

	if (OutRemainingLootItem.ItemRowName.IsNone() || OutRemainingLootItem.Amount <= 0)
	{
		ClearLootSlot(LootSlotIndex);
	}
	else
	{
		LootResults[LootSlotIndex].ItemRowName = OutRemainingLootItem.ItemRowName;
		LootResults[LootSlotIndex].Amount = OutRemainingLootItem.Amount;
		LootResults[LootSlotIndex].ItemText = FText::GetEmpty();
	}
	NotifyLootResultsChanged();
	ForceNetUpdate();
	return true;
}

bool ALSLootBox::TransferLootSlotToSessionSlot(const int32 LootSlotIndex, ULSRaidInventoryComponent* RaidInventory, const ELSInventorySlotArea ToSlotArea, const int32 ToSlotIndex, FLSSessionItem& OutRemainingLootItem)
{
	OutRemainingLootItem = FLSSessionItem();
	if (!LootResults.IsValidIndex(LootSlotIndex) || LootResults[LootSlotIndex].ItemRowName.IsNone() || LootResults[LootSlotIndex].Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to inventory slot because slot is invalid. Index=%d"), LootSlotIndex);
		return false;
	}

	if (!RaidInventory || !RaidInventory->IsRaidActive())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to inventory slot because raid session is not active."));
		return false;
	}

	FLSSessionItem ExternalItem;
	ExternalItem.ItemRowName = LootResults[LootSlotIndex].ItemRowName;
	ExternalItem.Amount = LootResults[LootSlotIndex].Amount;
	if (!RaidInventory->DropExternalItemToSessionSlot(ExternalItem, ToSlotArea, ToSlotIndex))
	{
		return false;
	}

	OutRemainingLootItem = ExternalItem;
	if (ExternalItem.ItemRowName.IsNone() || ExternalItem.Amount <= 0)
	{
		ClearLootSlot(LootSlotIndex);
	}
	else
	{
		LootResults[LootSlotIndex].ItemRowName = ExternalItem.ItemRowName;
		LootResults[LootSlotIndex].Amount = ExternalItem.Amount;
		LootResults[LootSlotIndex].ItemText = FText::GetEmpty();
	}

	NotifyLootResultsChanged();
	ForceNetUpdate();
	return true;
}

bool ALSLootBox::TransferSessionSlotToLootSlot(const int32 LootSlotIndex, ULSRaidInventoryComponent* RaidInventory, const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex, FLSSessionItem& OutLootItem)
{
	OutLootItem = FLSSessionItem();
	if (!LootResults.IsValidIndex(LootSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer session slot to loot slot because loot index is invalid. Index=%d"), LootSlotIndex);
		return false;
	}

	if (!RaidInventory || !RaidInventory->IsRaidActive())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer session slot to loot slot because raid session is not active."));
		return false;
	}

	FLSSessionItem CurrentLootItem;
	CurrentLootItem.ItemRowName = LootResults[LootSlotIndex].ItemRowName;
	CurrentLootItem.Amount = LootResults[LootSlotIndex].Amount;

	FLSSessionItem SourceItem;
	if (!RaidInventory->GetSessionSlotItem(FromSlotArea, FromSlotIndex, SourceItem))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer session slot to loot slot because source slot is empty. Area=%d Index=%d"),
			static_cast<int32>(FromSlotArea), FromSlotIndex);
		return false;
	}

	if (!RaidInventory->ReplaceSessionSlotItem(FromSlotArea, FromSlotIndex, CurrentLootItem, OutLootItem))
	{
		return false;
	}

	LootResults[LootSlotIndex].ItemRowName = OutLootItem.ItemRowName;
	LootResults[LootSlotIndex].Amount = OutLootItem.Amount;
	LootResults[LootSlotIndex].ItemText = FText::GetEmpty();
	NotifyLootResultsChanged();
	ForceNetUpdate();
	return true;
}

void ALSLootBox::ClearLootSlot(const int32 LootSlotIndex)
{
	if (!LootResults.IsValidIndex(LootSlotIndex))
	{
		return;
	}

	LootResults[LootSlotIndex].ItemRowName = NAME_None;
	LootResults[LootSlotIndex].Amount = 0;
	LootResults[LootSlotIndex].ItemText = FText::GetEmpty();
}

void ALSLootBox::NotifyLootResultsChanged()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(It->Get()))
		{
			PlayerController->RefreshLootDropWidgetForSource(this, LootResults);
		}
	}
}
