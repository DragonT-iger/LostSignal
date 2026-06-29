#include "UI/Title/LSTitleMenuButtonWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "LostSignal.h"

void ULSTitleMenuButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!Button)
	{
		UE_LOG(LogLS, Warning, TEXT("Button is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		Button->OnClicked.AddDynamic(this, &ULSTitleMenuButtonWidget::HandleButtonClicked);
	}

	if (!Text)
	{
		UE_LOG(LogLS, Warning, TEXT("Text is not bound on %s."), *GetNameSafe(this));
	}
}

void ULSTitleMenuButtonWidget::NativeDestruct()
{
	if (Button)
	{
		Button->OnClicked.RemoveDynamic(this, &ULSTitleMenuButtonWidget::HandleButtonClicked);
	}

	Super::NativeDestruct();
}

void ULSTitleMenuButtonWidget::SetLabelText(const FText& NewText) const
{
	if (!Text)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set label text because Text is not bound on %s."), *GetNameSafe(this));
		return;
	}

	Text->SetText(NewText);
}

void ULSTitleMenuButtonWidget::SetButtonEnabled(const bool bEnabled)
{
	if (!Button)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set enabled because Button is not bound on %s."), *GetNameSafe(this));
		return;
	}

	Button->SetIsEnabled(bEnabled);
}

void ULSTitleMenuButtonWidget::HandleButtonClicked()
{
	OnClicked.Broadcast();
}
