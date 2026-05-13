#include "UI/LootDrop/LSLootDropWidget.h"

#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
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

	for (const FLSDropResult& Item : InItems)
	{
		ULSItemSlotWidget* SlotWidget = CreateLootSlotWidget();
		if (!SlotWidget)
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create loot slot widget on %s."), *GetNameSafe(this));
			continue;
		}

		if (!Item.ItemRowName.IsNone() && Item.Amount > 0)
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

	ULSItemSlotWidget* SlotWidget = Cast<ULSItemSlotWidget>(LootItemWrapBox->GetChildAt(SlotIndex));
	if (!SlotWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot clear loot slot because child %d is not an inventory item slot on %s."), SlotIndex, *GetNameSafe(this));
		return;
	}

	SlotWidget->ClearItem();
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
