#include "UI/Lobby/Crafting/LSCraftingTabWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "LostSignal.h"

void ULSCraftingTabWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button)
	{
		Button->OnClicked.AddDynamic(this, &ULSCraftingTabWidget::HandleButtonClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Crafting] Button is not bound on %s."), *GetNameSafe(this));
	}
	if (!LabelText)
	{
		UE_LOG(LogLS, Warning, TEXT("[Crafting] LabelText is not bound on %s."), *GetNameSafe(this));
	}
}

void ULSCraftingTabWidget::NativeDestruct()
{
	if (Button)
	{
		Button->OnClicked.RemoveDynamic(this, &ULSCraftingTabWidget::HandleButtonClicked);
	}
	Super::NativeDestruct();
}

void ULSCraftingTabWidget::SetLabel(const FText& NewLabel) const
{
	if (LabelText)
	{
		LabelText->SetText(NewLabel);
	}
}

void ULSCraftingTabWidget::SetSelected(const bool bSelected) const
{
	if (Button)
	{
		Button->SetIsEnabled(!bSelected);
	}
}

void ULSCraftingTabWidget::HandleButtonClicked()
{
	OnClicked.Broadcast(this);
}
