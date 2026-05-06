#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSInteractHintWidget.generated.h"

class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSInteractHintWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void UpdateHintInfo(const FText& ObjectName, const FText& KeyName);

protected:
	// 블루프린트 위젯에 이름이 정확히 일치하는 TextBlock 필요
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UTextBlock> ObjectNameText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	TObjectPtr<UTextBlock> KeyText;
};
