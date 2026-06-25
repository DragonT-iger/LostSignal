#include "UI/LSInteractHintWidget.h"
#include "Components/TextBlock.h"

void ULSInteractHintWidget::UpdateHintInfo(const FText& ObjectName, const FText& KeyName)
{
	// 오브젝트 이름은 숨기고 입력 키(F)만 표시한다.
	if (ObjectNameText) ObjectNameText->SetVisibility(ESlateVisibility::Collapsed);
	if (KeyText) KeyText->SetText(KeyName);
}
