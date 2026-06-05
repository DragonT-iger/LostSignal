#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "LSProtocolTooltipTextWidget.generated.h"

class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSProtocolTooltipTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI|Protocol")
	void SetProtocolTooltipText(const FText& InText);

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Protocol")
	TObjectPtr<UTextBlock> Text;
};
