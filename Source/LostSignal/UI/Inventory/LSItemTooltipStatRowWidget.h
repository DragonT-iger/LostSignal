#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "LSItemTooltipStatRowWidget.generated.h"

class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSItemTooltipStatRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void SetStat(const FText& StatName, const FText& StatValue);

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> StatNameText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> StatValueText;
};
