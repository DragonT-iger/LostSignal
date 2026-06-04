#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "LSChipStatWidget.generated.h"

class UTextBlock;
class UBorder;

// 칩 전투 스탯 한 줄 위젯 (WBP_ChipStat 의 부모 클래스).
//
// 표시 요소
//  - StatNameText  : 전투 스탯 이름 (예: 공격력)
//  - StatValueText : 전투 스탯 증가량
//  - SignalLossText: 신호 유실 증가량
//  - 게이지 바(1개): HorizontalBox 안에 [파랑][분홍][빈칸] Border 를 Slot Size = Fill 비중으로 표현.
//      파랑 비중 = 전투 스탯 증가량, 분홍 비중 = 신호 유실 증가량,
//      빈칸 비중 = (max - 전투 스탯 - 신호 유실).
//
// max 값은 현재 임의값(GaugeMax, 기본 50)이다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSChipStatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 한 줄 스탯 표시를 갱신한다.
	//  StatName   : 스탯 이름 (예: 공격력)
	//  StatValue  : 전투 스탯 증가량 (게이지 파랑 비중)
	//  SignalLoss : 신호 유실 증가량 (게이지 분홍 비중)
	UFUNCTION(BlueprintCallable, Category="LS/UI|Chip")
	void SetStat(const FText& StatName, int32 StatValue, int32 SignalLoss);

protected:
	// ---- 텍스트 ----
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> StatNameText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> StatValueText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> SignalLossText;

	// ---- 게이지 바 (HorizontalBox 안 [파랑][분홍][빈칸] Fill 비중) ----
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UBorder> GaugeStat;    // 파랑: 전투 스탯 증가량 비중

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UBorder> GaugeSignal;  // 분홍: 신호 유실 증가량 비중

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UBorder> GaugeEmpty;   // 빈칸: 남은 비중 (max - 둘 합)

	// 게이지 max (임의값). 파랑+분홍+빈칸 비중 합 기준.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/UI")
	int32 GaugeMax = 50;

private:
	// 자식 Border 의 HorizontalBoxSlot 을 Fill + 지정 비중으로 설정한다.
	static void SetGaugeFillWeight(UBorder* Segment, float Weight);
};
