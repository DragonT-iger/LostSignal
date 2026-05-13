#include "UI/LootDrop/LSLootDropWidget.h"

#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Core/LSPlayerControllerBase.h"
#include "Gameplay/LSLootBox.h"
#include "LostSignal.h"
#include "UI/Inventory/LSItemSlotWidget.h"

void ULSLootDropWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!LootSourceNameText)
	{
		UE_LOG(LogLS, Warning, TEXT("LootSourceNameText is not bound on %s."), *GetNameSafe(this));
	}

	if (!LootItemWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("LootItemWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	LootItemWrapBox->ClearChildren();
	LootItems.Empty();
}

void ULSLootDropWidget::SetLootSourceName(const FText InSourceName)
{
	if (!LootSourceNameText)
	{
		UE_LOG(LogLS, Warning, TEXT("LootSourceNameText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	LootSourceNameText->SetText(InSourceName);
}

void ULSLootDropWidget::SetLootItems(const TArray<FLSDropResult>& InItems)
{
	LootItems = InItems;
	HoveredLootSlotIndex = INDEX_NONE;
	RebuildLootSlots();
}

void ULSLootDropWidget::SetSourceLootBox(ALSLootBox* InSourceLootBox)
{
	SourceLootBox = InSourceLootBox;
}

void ULSLootDropWidget::RebuildLootSlots()
{
	if (!LootItemWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("LootItemWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	LootItemWrapBox->ClearChildren();

	if (!ItemSlotWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemSlotWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < LootItems.Num(); ++SlotIndex)
	{
		const FLSDropResult& Item = LootItems[SlotIndex];
		ULSItemSlotWidget* SlotWidget = CreateLootSlotWidget();
		if (!SlotWidget)
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create loot slot widget on %s."), *GetNameSafe(this));
			continue;
		}

		const bool bHasItem = !Item.ItemRowName.IsNone() && Item.Amount > 0;
		SlotWidget->SetLootSlotContext(this, SlotIndex, bHasItem);
		if (bHasItem)
		{
			SlotWidget->SetItem(Item.ItemRowName, Item.Amount);
		}
		else
		{
			SlotWidget->ClearItem();
		}

		LootItemWrapBox->AddChildToWrapBox(SlotWidget);
	}
}

void ULSLootDropWidget::ClearLootItems()
{
	LootItems.Empty();
	HoveredLootSlotIndex = INDEX_NONE;

	if (!LootItemWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("LootItemWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	LootItemWrapBox->ClearChildren();
}

void ULSLootDropWidget::ClearLootSlotAt(const int32 SlotIndex)
{
	if (!LootItemWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("LootItemWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (SlotIndex < 0 || SlotIndex >= LootItemWrapBox->GetChildrenCount())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot clear loot slot because index %d is invalid on %s."), SlotIndex, *GetNameSafe(this));
		return;
	}

	if (LootItems.IsValidIndex(SlotIndex))
	{
		SetLootSlotFromSessionItem(SlotIndex, FLSSessionItem());
		RebuildLootSlots();
		return;
	}

	ULSItemSlotWidget* SlotWidget = Cast<ULSItemSlotWidget>(LootItemWrapBox->GetChildAt(SlotIndex));
	if (!SlotWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot clear loot slot because child %d is not an inventory item slot on %s."), SlotIndex, *GetNameSafe(this));
		return;
	}

	SlotWidget->ClearItem();
}

bool ULSLootDropWidget::TransferLootSlotToInventory(const int32 SlotIndex)
{
	if (!LootItems.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot because index %d is invalid on %s."), SlotIndex, *GetNameSafe(this));
		return false;
	}

	const FLSDropResult LootItem = LootItems[SlotIndex];
	if (LootItem.ItemRowName.IsNone() || LootItem.Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot because item data is invalid. Index=%d Row=%s Amount=%d"),
			SlotIndex,
			*LootItem.ItemRowName.ToString(),
			LootItem.Amount);
		return false;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot because owning player controller is invalid on %s."), *GetNameSafe(this));
		return false;
	}

	FLSSessionItem RemainingLootItem;
	if (!PlayerController->TransferLootDropSlotToSession(SourceLootBox, SlotIndex, LootItem.ItemRowName, LootItem.Amount, RemainingLootItem))
	{
		return false;
	}

	SetLootSlotFromSessionItem(SlotIndex, RemainingLootItem);
	RebuildLootSlots();
	return true;
}

bool ULSLootDropWidget::TransferLootSlotToInventorySlot(const int32 SlotIndex, const ELSInventorySlotArea ToSlotArea, const int32 ToSlotIndex)
{
	if (!LootItems.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to inventory slot because index %d is invalid on %s."), SlotIndex, *GetNameSafe(this));
		return false;
	}

	const FLSDropResult LootItem = LootItems[SlotIndex];
	if (LootItem.ItemRowName.IsNone() || LootItem.Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to inventory slot because item data is invalid. Index=%d Row=%s Amount=%d"),
			SlotIndex,
			*LootItem.ItemRowName.ToString(),
			LootItem.Amount);
		return false;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to inventory slot because owning player controller is invalid on %s."), *GetNameSafe(this));
		return false;
	}

	FLSSessionItem RemainingLootItem;
	if (!PlayerController->TransferLootDropSlotToSessionSlot(SourceLootBox, SlotIndex, LootItem.ItemRowName, LootItem.Amount, ToSlotArea, ToSlotIndex, RemainingLootItem))
	{
		return false;
	}

	SetLootSlotFromSessionItem(SlotIndex, RemainingLootItem);
	RebuildLootSlots();
	return true;
}

bool ULSLootDropWidget::TransferHoveredLootSlotToInventory()
{
	if (!LootItems.IsValidIndex(HoveredLootSlotIndex))
	{
		return false;
	}

	const FLSDropResult& LootItem = LootItems[HoveredLootSlotIndex];
	if (LootItem.ItemRowName.IsNone() || LootItem.Amount <= 0)
	{
		return false;
	}

	return TransferLootSlotToInventory(HoveredLootSlotIndex);
}

void ULSLootDropWidget::NotifyLootSlotHovered(const int32 SlotIndex)
{
	HoveredLootSlotIndex = SlotIndex;
}

void ULSLootDropWidget::NotifyLootSlotUnhovered(const int32 SlotIndex)
{
	if (HoveredLootSlotIndex == SlotIndex)
	{
		HoveredLootSlotIndex = INDEX_NONE;
	}
}

void ULSLootDropWidget::SetLootSlotFromSessionItem(const int32 SlotIndex, const FLSSessionItem& SessionItem)
{
	if (!LootItems.IsValidIndex(SlotIndex))
	{
		return;
	}

	LootItems[SlotIndex].ItemRowName = SessionItem.ItemRowName;
	LootItems[SlotIndex].Amount = SessionItem.Amount;
	LootItems[SlotIndex].ItemText = FText::GetEmpty();
	if (HoveredLootSlotIndex == SlotIndex && (SessionItem.ItemRowName.IsNone() || SessionItem.Amount <= 0))
	{
		HoveredLootSlotIndex = INDEX_NONE;
	}
}

bool ULSLootDropWidget::HasLootItems() const
{
	for (const FLSDropResult& LootItem : LootItems)
	{
		if (!LootItem.ItemRowName.IsNone() && LootItem.Amount > 0)
		{
			return true;
		}
	}

	return false;
}

ULSItemSlotWidget* ULSLootDropWidget::CreateLootSlotWidget() const
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	UWorld* World = GetWorld();
	if (!OwningPlayer && !World)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot create loot slot because owner/world is missing on %s."), *GetNameSafe(this));
		return nullptr;
	}

	return OwningPlayer
		? CreateWidget<ULSItemSlotWidget>(OwningPlayer, ItemSlotWidgetClass)
		: CreateWidget<ULSItemSlotWidget>(World, ItemSlotWidgetClass);
}
