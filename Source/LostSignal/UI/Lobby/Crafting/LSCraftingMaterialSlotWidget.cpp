#include "UI/Lobby/Crafting/LSCraftingMaterialSlotWidget.h"

#include "Components/TextBlock.h"
#include "LostSignal.h"
#include "UI/Inventory/LSItemSlotWidget.h"

#define LOCTEXT_NAMESPACE "LSCrafting"

void ULSCraftingMaterialSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ItemSlot)
	{
		UE_LOG(LogLS, Warning, TEXT("[Crafting] ItemSlot is not bound on %s."), *GetNameSafe(this));
	}
	if (!AmountText)
	{
		UE_LOG(LogLS, Warning, TEXT("[Crafting] AmountText is not bound on %s."), *GetNameSafe(this));
	}
}

void ULSCraftingMaterialSlotWidget::SetMaterial(
	const FName ItemRowName,
	const int32 OwnedAmount,
	const int32 RequiredAmount) const
{
	if (ItemSlot)
	{
		ItemSlot->SetDisplayOnlySlotContext();
		ItemSlot->SetItem(ItemRowName, OwnedAmount, TArray<FLSChipResolvedStat>());
		ItemSlot->SetAmountTextVisible(false);
	}
	if (AmountText)
	{
		AmountText->SetText(FText::Format(
			LOCTEXT("MaterialAmountFormat", "{0}/{1}"),
			FText::AsNumber(OwnedAmount),
			FText::AsNumber(RequiredAmount)));
	}
}

#undef LOCTEXT_NAMESPACE
