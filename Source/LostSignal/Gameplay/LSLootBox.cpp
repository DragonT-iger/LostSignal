#include "Gameplay/LSLootBox.h"
#include "Core/LSPlayerControllerBase.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
#include "Net/UnrealNetwork.h"

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
	if (!LootResults.IsValidIndex(FromLootSlotIndex) || !LSInventorySlotUtils::IsFilled(LootResults[FromLootSlotIndex]) || !LootResults.IsValidIndex(ToLootSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop loot slot because slot data is invalid. From=%d To=%d"), FromLootSlotIndex, ToLootSlotIndex);
		return false;
	}

	const bool bChanged = LSInventorySlotUtils::DropResultSlot(LootResults, FromLootSlotIndex, ToLootSlotIndex);
	if (bChanged)
	{
		NotifyLootResultsChanged();
		ForceNetUpdate();
	}
	return bChanged;
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

	if (!RaidInventory->TryAddSessionItem(LootResults[LootSlotIndex].ItemRowName, LootResults[LootSlotIndex].Amount, LootResults[LootSlotIndex].ChipStats, OutRemainingLootItem))
	{
		return false;
	}

	if (OutRemainingLootItem.ItemRowName.IsNone() || OutRemainingLootItem.Amount <= 0)
	{
		ClearLootSlot(LootSlotIndex);
	}
	else
	{
		LSInventorySlotUtils::SetDropResultFromSessionItem(LootResults[LootSlotIndex], OutRemainingLootItem);
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

	FLSSessionItem ExternalItem = LSInventorySlotUtils::ToSessionItem(LootResults[LootSlotIndex]);
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
		LSInventorySlotUtils::SetDropResultFromSessionItem(LootResults[LootSlotIndex], ExternalItem);
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

	FLSSessionItem CurrentLootItem = LSInventorySlotUtils::ToSessionItem(LootResults[LootSlotIndex]);

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

	LSInventorySlotUtils::SetDropResultFromSessionItem(LootResults[LootSlotIndex], OutLootItem);
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

	LSInventorySlotUtils::ClearDropResult(LootResults[LootSlotIndex]);
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
