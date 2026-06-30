#include "UI/Lobby/LSQuestInfoWidget.h"

#include "Components/TextBlock.h"
#include "LostSignal.h"

void ULSQuestInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!QuestNameText)
	{
		UE_LOG(LogLS, Warning, TEXT("QuestNameText is not bound on %s."), *GetNameSafe(this));
	}
	if (!QuestObjectiveText)
	{
		UE_LOG(LogLS, Warning, TEXT("QuestObjectiveText is not bound on %s."), *GetNameSafe(this));
	}
}

void ULSQuestInfoWidget::SetQuestInfo(const FText& NewName, const FText& NewObjective) const
{
	SetQuestName(NewName);
	SetQuestObjective(NewObjective);
}

void ULSQuestInfoWidget::SetQuestName(const FText& NewName) const
{
	if (!QuestNameText)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set quest name because QuestNameText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	QuestNameText->SetText(NewName);
}

void ULSQuestInfoWidget::SetQuestObjective(const FText& NewObjective) const
{
	if (!QuestObjectiveText)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set quest objective because QuestObjectiveText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	QuestObjectiveText->SetText(NewObjective);
}
