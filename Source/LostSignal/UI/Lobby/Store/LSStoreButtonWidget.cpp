#include "UI/Lobby/Store/LSStoreButtonWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "LostSignal.h"

void ULSStoreButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button)
	{
		Button->OnClicked.AddDynamic(this, &ULSStoreButtonWidget::HandleButtonClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("Button is not bound on %s."), *GetNameSafe(this));
	}

	if (!LabelText)
	{
		UE_LOG(LogLS, Warning, TEXT("LabelText is not bound on %s."), *GetNameSafe(this));
	}
	if (!QuestIcon)
	{
		UE_LOG(LogLS, Warning, TEXT("QuestIcon is not bound on %s."), *GetNameSafe(this));
	}
}

void ULSStoreButtonWidget::NativeDestruct()
{
	if (Button)
	{
		Button->OnClicked.RemoveDynamic(this, &ULSStoreButtonWidget::HandleButtonClicked);
	}

	Super::NativeDestruct();
}

void ULSStoreButtonWidget::SetLabel(const FText& NewLabel) const
{
	if (LabelText)
	{
		LabelText->SetText(NewLabel);
	}
}

void ULSStoreButtonWidget::SetQuestIconVisible(const bool bVisible) const
{
	if (QuestIcon)
	{
		QuestIcon->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void ULSStoreButtonWidget::HandleButtonClicked()
{
	OnClicked.Broadcast(this);
}
