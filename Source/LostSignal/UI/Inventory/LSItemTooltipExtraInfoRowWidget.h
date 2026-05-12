#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "LSItemTooltipExtraInfoRowWidget.generated.h"

class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSItemTooltipExtraInfoRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void SetExtraInfo(const FText& ExtraInfoName, const FText& ExtraInfoValue);

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> ExtraInfoNameText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> ExtraInfoValueText;
};
