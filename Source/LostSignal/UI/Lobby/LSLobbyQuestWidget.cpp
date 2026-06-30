#include "UI/Lobby/LSLobbyQuestWidget.h"

#include "Components/Button.h"
#include "LostSignal.h"
#include "UI/Lobby/LSQuestInfoWidget.h"

void ULSLobbyQuestWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!MainQuest)
	{
		UE_LOG(LogLS, Warning, TEXT("MainQuest is not bound on %s."), *GetNameSafe(this));
	}
	if (!SubQuest1)
	{
		UE_LOG(LogLS, Warning, TEXT("SubQuest1 is not bound on %s."), *GetNameSafe(this));
	}
	if (!SubQuest2)
	{
		UE_LOG(LogLS, Warning, TEXT("SubQuest2 is not bound on %s."), *GetNameSafe(this));
	}
	if (!SubQuest3)
	{
		UE_LOG(LogLS, Warning, TEXT("SubQuest3 is not bound on %s."), *GetNameSafe(this));
	}

	if (MainToggleButton)
	{
		MainToggleButton->OnClicked.AddDynamic(this, &ULSLobbyQuestWidget::HandleMainToggleClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("MainToggleButton is not bound on %s."), *GetNameSafe(this));
	}
	if (SubToggleButton)
	{
		SubToggleButton->OnClicked.AddDynamic(this, &ULSLobbyQuestWidget::HandleSubToggleClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("SubToggleButton is not bound on %s."), *GetNameSafe(this));
	}

	// 초기 표시 상태(펼침)를 위젯과 아이콘에 반영한다.
	RefreshMainQuestState();
	RefreshSubQuestState();
}

void ULSLobbyQuestWidget::NativeDestruct()
{
	if (MainToggleButton)
	{
		MainToggleButton->OnClicked.RemoveDynamic(this, &ULSLobbyQuestWidget::HandleMainToggleClicked);
	}
	if (SubToggleButton)
	{
		SubToggleButton->OnClicked.RemoveDynamic(this, &ULSLobbyQuestWidget::HandleSubToggleClicked);
	}

	Super::NativeDestruct();
}

void ULSLobbyQuestWidget::SetMainQuest(const FText& NewName, const FText& NewObjective) const
{
	if (!MainQuest)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set main quest because MainQuest is not bound on %s."), *GetNameSafe(this));
		return;
	}

	MainQuest->SetQuestInfo(NewName, NewObjective);
}

void ULSLobbyQuestWidget::SetSubQuest(const int32 SubIndex, const FText& NewName, const FText& NewObjective) const
{
	ULSQuestInfoWidget* SubQuest = GetSubQuest(SubIndex);
	if (!SubQuest)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set sub quest %d because it is out of range or not bound on %s."), SubIndex, *GetNameSafe(this));
		return;
	}

	SubQuest->SetQuestInfo(NewName, NewObjective);
}

void ULSLobbyQuestWidget::HandleMainToggleClicked()
{
	bMainQuestOpen = !bMainQuestOpen;
	RefreshMainQuestState();
}

void ULSLobbyQuestWidget::HandleSubToggleClicked()
{
	bSubQuestOpen = !bSubQuestOpen;
	RefreshSubQuestState();
}

void ULSLobbyQuestWidget::RefreshMainQuestState() const
{
	if (MainQuest)
	{
		MainQuest->SetVisibility(bMainQuestOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	ApplyToggleImage(MainToggleButton, bMainQuestOpen);
}

void ULSLobbyQuestWidget::RefreshSubQuestState() const
{
	const ESlateVisibility SubVisibility = bSubQuestOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	for (int32 SubIndex = 0; SubIndex < SubQuestCount; ++SubIndex)
	{
		if (ULSQuestInfoWidget* SubQuest = GetSubQuest(SubIndex))
		{
			SubQuest->SetVisibility(SubVisibility);
		}
	}

	ApplyToggleImage(SubToggleButton, bSubQuestOpen);
}

void ULSLobbyQuestWidget::ApplyToggleImage(UButton* ToggleButton, const bool bOpen) const
{
	if (!ToggleButton)
	{
		return;
	}

	// 펼쳐져 있으면 "닫기" 아이콘, 접혀 있으면 "열기" 아이콘을 버튼 스타일에 적용한다.
	const FSlateBrush& IconBrush = bOpen ? CloseImageBrush : OpenImageBrush;
	FButtonStyle NewStyle = ToggleButton->GetStyle();
	NewStyle.SetNormal(IconBrush);
	NewStyle.SetHovered(IconBrush);
	NewStyle.SetPressed(IconBrush);
	ToggleButton->SetStyle(NewStyle);
}

ULSQuestInfoWidget* ULSLobbyQuestWidget::GetSubQuest(const int32 SubIndex) const
{
	switch (SubIndex)
	{
	case 0:
		return SubQuest1;
	case 1:
		return SubQuest2;
	case 2:
		return SubQuest3;
	default:
		return nullptr;
	}
}
