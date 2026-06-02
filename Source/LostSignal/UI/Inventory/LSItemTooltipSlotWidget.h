#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Data/LSChipStats.h"
#include "LSItemTooltipSlotWidget.generated.h"

class ULSItemTooltipWidget;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSItemTooltipSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void SetTooltipItem(FName ItemRowName, int32 Amount, const TArray<FLSChipResolvedStat>& ChipStats);

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void ClearTooltipItem();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	TSubclassOf<ULSItemTooltipWidget> ItemTooltipWidgetClass;

	bool HasTooltipItem() const { return bHasTooltipItem && !CurrentTooltipItemRowName.IsNone(); }

private:
	TObjectPtr<ULSItemTooltipWidget> ItemTooltipWidget;
	FName CurrentTooltipItemRowName;
	int32 CurrentTooltipAmount = 0;
	TArray<FLSChipResolvedStat> CurrentTooltipChipStats;
	bool bHasTooltipItem = false;

	void RefreshItemTooltip();
};
