#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MiniGame/RatSteal/LSRatTypes.h"
#include "LSRatResultWidget.generated.h"

class UTextBlock;

/**
 * 결과(엔딩) 화면 (41_UI_Menus).
 * 종료 사유별 결과 + 총점/종류별 수집 수/★등급. 복귀 버튼은 WBP에서 ReturnToMainWorld 호출.
 */
UCLASS()
class LOSTSIGNAL_API ULSRatResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void SetResult(const FLSRatResult& InResult);

	/** 복귀 버튼에서 호출 — 서브시스템 경유로 본편 복귀 (31_Flow) */
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void ReturnToMainWorld();

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	const FLSRatResult& GetResult() const { return Result; }

	/** WBP에서 사유별 엔딩 일러스트/연출 분기용 */
	UFUNCTION(BlueprintImplementableEvent, Category = "LS/RatSteal")
	void OnResultSet(const FLSRatResult& InResult);

protected:
	virtual void NativeOnInitialized() override;

	/** WBP 미바인딩 시 코드로 기본 레이아웃 구성 */
	void BuildFallbackLayout();

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "LS/RatSteal")
	TObjectPtr<UTextBlock> ReasonText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "LS/RatSteal")
	TObjectPtr<UTextBlock> ScoreText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "LS/RatSteal")
	TObjectPtr<UTextBlock> GradeText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "LS/RatSteal")
	TObjectPtr<UTextBlock> CountsText;

private:
	FLSRatResult Result;
};
