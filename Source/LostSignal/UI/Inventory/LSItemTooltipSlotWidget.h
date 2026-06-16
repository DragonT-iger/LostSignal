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
	// UPROPERTY로 GC가 추적하게 한다. 없으면 슬롯 위젯이 풀링으로 오래 살아남는 동안
	// 툴팁 위젯이 GC되어 댕글링 포인터가 된다.
	UPROPERTY(Transient)
	TObjectPtr<ULSItemTooltipWidget> ItemTooltipWidget;
	FName CurrentTooltipItemRowName;
	int32 CurrentTooltipAmount = 0;
	TArray<FLSChipResolvedStat> CurrentTooltipChipStats;
	bool bHasTooltipItem = false;

	void RefreshItemTooltip();
};
