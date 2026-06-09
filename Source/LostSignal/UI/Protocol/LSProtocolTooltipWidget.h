#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Data/LSProtocolTypes.h"
#include "LSProtocolTooltipWidget.generated.h"

class UImage;
class UPanelWidget;
class ULSProtocolTooltipTextWidget;
class UTextBlock;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSProtocolTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI|Protocol")
	void SetProtocolTooltip(ELSProtocolType ProtocolType, UTexture2D* IconTexture);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Protocol")
	void SetProtocolTooltipLevels(ELSProtocolType ProtocolType, UTexture2D* IconTexture, int32 CurrentLevel, int32 PreviousLevel);

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Protocol")
	TObjectPtr<UImage> ProtocolImage;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Protocol")
	TObjectPtr<UTextBlock> ProtocolTypeText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Protocol")
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Protocol")
	TObjectPtr<UPanelWidget> SynergyBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	TSubclassOf<ULSProtocolTooltipTextWidget> TooltipTextWidgetClass;

private:
	void AddSynergyText(const FText& SynergyText, bool bUnlocked, bool bProtected);
};
