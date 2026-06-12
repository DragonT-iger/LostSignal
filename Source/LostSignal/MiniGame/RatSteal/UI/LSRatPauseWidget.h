#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSRatPauseWidget.generated.h"

/**
 * 일시정지 메뉴 (41_UI_Menus). 계속/재시작/나가기.
 * 버튼 배치는 WBP에서, 동작은 아래 BlueprintCallable 호출.
 */
UCLASS()
class LOSTSIGNAL_API ULSRatPauseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 계속하기 — 일시정지 해제 */
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void ResumeGame();

	/** 재시작 — 미니게임 레벨 리로드 */
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void RestartGame();

	/** 나가기 — 본편 복귀 (31_Flow) */
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void QuitToMainWorld();

protected:
	virtual void NativeOnInitialized() override;
};
