#include "UI/Storage/LSStorageButtonWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "LostSignal.h"

void ULSStorageButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!Button)
	{
		UE_LOG(LogLS, Warning, TEXT("Button is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		Button->OnClicked.AddDynamic(this, &ULSStorageButtonWidget::HandleButtonClicked);
	}

	if (!Text)
	{
		UE_LOG(LogLS, Warning, TEXT("Text is not bound on %s."), *GetNameSafe(this));
	}
}

void ULSStorageButtonWidget::NativeDestruct()
{
	if (Button)
	{
		Button->OnClicked.RemoveDynamic(this, &ULSStorageButtonWidget::HandleButtonClicked);
	}

	Super::NativeDestruct();
}

void ULSStorageButtonWidget::SetLabelText(const FText& NewText) const
{
	if (!Text)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set label text because Text is not bound on %s."), *GetNameSafe(this));
		return;
	}

	Text->SetText(NewText);
}

void ULSStorageButtonWidget::HandleButtonClicked()
{
	OnClicked.Broadcast();
}
