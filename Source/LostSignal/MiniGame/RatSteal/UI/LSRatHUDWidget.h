#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MiniGame/RatSteal/LSRatTypes.h"
#include "LSRatHUDWidget.generated.h"

class UCanvasPanel;
class UImage;
class UProgressBar;
class UTextBlock;
class UTexture2D;

/**
 * 인게임 HUD (40_UI_HUD). 원작 MainScene UI 배치를 따른다:
 *  - 하단 Main_Panel: 좌측 프로필(엄마/아기) + 하트 3 + 포만 게이지, 우측 인벤토리 3슬롯
 *  - 우상단 Sign_Panel + "Score : N", 상단 중앙 타이머
 * WBP_RatStealHUD에서 같은 이름으로 배치하면 그쪽이 우선, 없으면 폴백 레이아웃을 스스로 구성.
 */
UCLASS()
class LOSTSIGNAL_API ULSRatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** WBP 미바인딩 시 코드로 원작 배치 구성 */
	void BuildFallbackLayout();

	UFUNCTION()
	void HandleScoreChanged(int32 TotalScore, int32 DeltaScore);

	UFUNCTION()
	void HandleHpChanged(int32 NewHp);

	UFUNCTION()
	void HandleFullnessChanged(float Fullness, float MaxFullness);

	UFUNCTION()
	void HandleInventoryChanged();

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "LS/RatSteal")
	TObjectPtr<UTextBlock> ScoreText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "LS/RatSteal")
	TObjectPtr<UTextBlock> TimerText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "LS/RatSteal")
	TObjectPtr<UProgressBar> FullnessBar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "LS/RatSteal")
	TObjectPtr<UImage> Heart1;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "LS/RatSteal")
	TObjectPtr<UImage> Heart2;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "LS/RatSteal")
	TObjectPtr<UImage> Heart3;

private:
	struct FSlotWidgets
	{
		TObjectPtr<UImage> Frame;
		TObjectPtr<UImage> Item;
		TObjectPtr<UTextBlock> Count;
	};

	void BuildBottomPanel(UCanvasPanel* Canvas);
	void BuildScoreSign(UCanvasPanel* Canvas);

	UTexture2D* GetItemTexture(ELSRatCropType Type) const;
	static UTexture2D* LoadHUDTexture(const TCHAR* AssetName);

	/** 폴백 인벤토리 슬롯 3개 (원작 Slot1~3) */
	FSlotWidgets SlotWidgets[3];

	UPROPERTY()
	TObjectPtr<UTexture2D> FrameTexture;

	UPROPERTY()
	TObjectPtr<UTexture2D> FrameSelectedTexture;
};
