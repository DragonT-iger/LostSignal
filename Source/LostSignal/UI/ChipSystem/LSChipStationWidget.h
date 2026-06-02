#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSChipStationWidget.generated.h"

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSChipStationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 칩 장착/설정 UI를 최신 데이터로 갱신한다.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LS/UI|Chip")
	void RefreshChipStation();
};
