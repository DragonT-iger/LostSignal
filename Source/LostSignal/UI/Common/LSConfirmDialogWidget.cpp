#include "UI/Common/LSConfirmDialogWidget.h"

#include "Components/Button.h"
#include "Components/RichTextBlock.h"
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

	// ESC를 받을 수 있도록, 그리고 뒤의 화면이 ESC를 가로채지 않도록 키보드 포커스를 가져온다.
	SetIsFocusable(true);
	SetKeyboardFocus();
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

FReply ULSConfirmDialogWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Tab)
	{
		// ESC/TAB은 취소(닫기)로 처리한다. TAB을 흘리면 Slate 포커스 이동으로 새서 다이얼로그가 키를 잃는다.
		HandleCancelClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULSConfirmDialogWidget::Cancel()
{
	HandleCancelClicked();
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
