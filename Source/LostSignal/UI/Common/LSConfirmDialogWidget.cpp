#include "UI/Common/LSConfirmDialogWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "LostSignal.h"

void ULSConfirmDialogWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.AddDynamic(this, &ULSConfirmDialogWidget::HandleConfirmClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("ConfirmButton is not bound on %s."), *GetNameSafe(this));
	}

	if (CancelButton)
	{
		CancelButton->OnClicked.AddDynamic(this, &ULSConfirmDialogWidget::HandleCancelClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("CancelButton is not bound on %s."), *GetNameSafe(this));
	}

	if (!MessageText)
	{
		UE_LOG(LogLS, Warning, TEXT("MessageText is not bound on %s."), *GetNameSafe(this));
	}
}

void ULSConfirmDialogWidget::NativeDestruct()
{
	if (ConfirmButton)
	{
		ConfirmButton->OnClicked.RemoveDynamic(this, &ULSConfirmDialogWidget::HandleConfirmClicked);
	}
	if (CancelButton)
	{
		CancelButton->OnClicked.RemoveDynamic(this, &ULSConfirmDialogWidget::HandleCancelClicked);
	}

	Super::NativeDestruct();
}

void ULSConfirmDialogWidget::SetMessage(const FText& InMessage) const
{
	if (!MessageText)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set message because MessageText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	MessageText->SetText(InMessage);
}

void ULSConfirmDialogWidget::HandleConfirmClicked()
{
	OnConfirmed.Broadcast();
	RemoveFromParent();
}

void ULSConfirmDialogWidget::HandleCancelClicked()
{
	OnCancelled.Broadcast();
	RemoveFromParent();
}
