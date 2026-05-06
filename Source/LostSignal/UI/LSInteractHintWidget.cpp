#include "UI/LSInteractHintWidget.h"
#include "Components/TextBlock.h"

void ULSInteractHintWidget::UpdateHintInfo(const FText& ObjectName, const FText& KeyName)
{
	if (ObjectNameText) ObjectNameText->SetText(ObjectName);
	if (KeyText) KeyText->SetText(KeyName);
}
