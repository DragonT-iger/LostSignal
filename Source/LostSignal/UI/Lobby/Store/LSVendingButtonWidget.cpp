#include "UI/Lobby/Store/LSVendingButtonWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "LostSignal.h"

void ULSVendingButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button)
	{
		Button->OnClicked.AddDynamic(this, &ULSVendingButtonWidget::HandleButtonClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] Button is not bound on %s."), *GetNameSafe(this));
	}
	if (!LabelText)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] LabelText is not bound on %s."), *GetNameSafe(this));
	}
}

void ULSVendingButtonWidget::NativeDestruct()
{
	if (Button)
	{
		Button->OnClicked.RemoveDynamic(this, &ULSVendingButtonWidget::HandleButtonClicked);
	}

	Super::NativeDestruct();
}

void ULSVendingButtonWidget::SetLabel(const FText& NewLabel) const
{
	if (LabelText)
	{
		LabelText->SetText(NewLabel);
	}
}

void ULSVendingButtonWidget::HandleButtonClicked()
{
	OnClicked.Broadcast(this);
}
