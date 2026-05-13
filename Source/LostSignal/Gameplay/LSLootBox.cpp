#include "Gameplay/LSLootBox.h"
#include "Core/LSPlayerControllerBase.h"
#include "LostSignal.h"
#include "Net/UnrealNetwork.h"

void ALSLootBox::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALSLootBox, bIsOpened);
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

bool ALSLootBox::TransferLootSlotToSession(const int32 LootSlotIndex, ULSSessionSubsystem* SessionSubsystem, FLSSessionItem& OutRemainingLootItem)
{
	OutRemainingLootItem = FLSSessionItem();
	if (!LootResults.IsValidIndex(LootSlotIndex) || LootResults[LootSlotIndex].ItemRowName.IsNone() || LootResults[LootSlotIndex].Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot because slot is invalid. Index=%d"), LootSlotIndex);
		return false;
	}

	if (!SessionSubsystem || !SessionSubsystem->IsRaidActive())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot because raid session is not active."));
		return false;
	}

	if (!SessionSubsystem->TryAddSessionItem(LootResults[LootSlotIndex].ItemRowName, LootResults[LootSlotIndex].Amount, OutRemainingLootItem))
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
	return true;
}

bool ALSLootBox::TransferLootSlotToSessionSlot(const int32 LootSlotIndex, ULSSessionSubsystem* SessionSubsystem, const ELSInventorySlotArea ToSlotArea, const int32 ToSlotIndex, FLSSessionItem& OutRemainingLootItem)
{
	OutRemainingLootItem = FLSSessionItem();
	if (!LootResults.IsValidIndex(LootSlotIndex) || LootResults[LootSlotIndex].ItemRowName.IsNone() || LootResults[LootSlotIndex].Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to inventory slot because slot is invalid. Index=%d"), LootSlotIndex);
		return false;
	}

	if (!SessionSubsystem || !SessionSubsystem->IsRaidActive())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to inventory slot because raid session is not active."));
		return false;
	}

	FLSSessionItem ExternalItem;
	ExternalItem.ItemRowName = LootResults[LootSlotIndex].ItemRowName;
	ExternalItem.Amount = LootResults[LootSlotIndex].Amount;
	if (!SessionSubsystem->DropExternalItemToSessionSlot(ExternalItem, ToSlotArea, ToSlotIndex))
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

	return true;
}

bool ALSLootBox::TransferSessionSlotToLootSlot(const int32 LootSlotIndex, ULSSessionSubsystem* SessionSubsystem, const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex, FLSSessionItem& OutLootItem)
{
	OutLootItem = FLSSessionItem();
	if (!LootResults.IsValidIndex(LootSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer session slot to loot slot because loot index is invalid. Index=%d"), LootSlotIndex);
		return false;
	}

	if (!SessionSubsystem || !SessionSubsystem->IsRaidActive())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer session slot to loot slot because raid session is not active."));
		return false;
	}

	FLSSessionItem CurrentLootItem;
	CurrentLootItem.ItemRowName = LootResults[LootSlotIndex].ItemRowName;
	CurrentLootItem.Amount = LootResults[LootSlotIndex].Amount;

	FLSSessionItem SourceItem;
	if (!SessionSubsystem->GetSessionSlotItem(FromSlotArea, FromSlotIndex, SourceItem))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer session slot to loot slot because source slot is empty. Area=%d Index=%d"),
			static_cast<int32>(FromSlotArea), FromSlotIndex);
		return false;
	}

	if (!SessionSubsystem->ReplaceSessionSlotItem(FromSlotArea, FromSlotIndex, CurrentLootItem, OutLootItem))
	{
		return false;
	}

	LootResults[LootSlotIndex].ItemRowName = OutLootItem.ItemRowName;
	LootResults[LootSlotIndex].Amount = OutLootItem.Amount;
	LootResults[LootSlotIndex].ItemText = FText::GetEmpty();
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
