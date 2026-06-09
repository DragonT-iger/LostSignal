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

	UFUNCTION(BlueprintCallable, Category="LS/UI|Protocol")
	void SetProtocolTooltipStateText(const FText& InText, bool bUnlocked, bool bProtected);

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Protocol")
	TObjectPtr<UTextBlock> Text;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	FSlateColor UnlockedColor = FSlateColor(FLinearColor::White);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	FSlateColor ProtectedColor = FSlateColor(FLinearColor(0.45f, 0.85f, 1.0f, 1.0f));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	FSlateColor LockedColor = FSlateColor(FLinearColor(0.45f, 0.45f, 0.45f, 1.0f));
};
