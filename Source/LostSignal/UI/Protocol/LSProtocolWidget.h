#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Data/LSProtocolTypes.h"
#include "LSProtocolWidget.generated.h"

class UImage;
class ULSProtocolStageWidget;
class ULSProtocolTooltipWidget;
class UTexture2D;
class UBorder;

// 프로토콜 한 줄 위젯 (WBP_Protocol 등 한 칸의 부모 클래스).
//
// 표시 요소
//  - ProtocolNameImage : 프로토콜 이름 이미지. 배치한 쪽(WBP_ChipStation 등)에서
//      인스턴스별로 ProtocolNameTexture 를 지정해 4종 프로토콜 이미지를 구분한다.
//  - ProtocolStage_1~7 : 단계 칸 7개(WBP_ProtocolStage). 순번 숫자와 해금 여부 색을
//      각 칸의 ULSProtocolStageWidget 이 표시한다. 데이터 단계 수와 무관하게 7칸 고정이며,
//      순번이 현재 프로토콜 레벨 이하인 칸만 해금 색으로 그린다.
//
// 해금 항목별 상세(이름·보호 여부)는 호버 툴팁(ULSProtocolTooltipWidget)이 DT_Protocol 기준으로 표시한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSProtocolWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 프로토콜 한 줄 표시를 갱신한다.
	//  Level        : 레벨 (숫자)
	//  SynergyStage : 해금된 단계 개수. 1~SynergyStage 순번 칸이 해금 색으로 표시된다.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Protocol")
	void SetProtocol(int32 Level, int32 SynergyStage);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Protocol")
	void SetProtocolLevels(int32 CurrentLevel, int32 PreviousLevel, int32 SynergyStage);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Protocol")
	void SetProtocolType(ELSProtocolType InProtocolType);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Protocol")
	void SetProtocolBorderColor(const FLinearColor& InColor);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 호버 시 프로토콜 한 줄 전체(단계 칸·이름 이미지)의 색에 곱하는 배수.
	// 절대 색으로 갈아치우지 않고 RGB 에 균일하게 곱하므로 원래 색조가 유지된다. 1.0 이면 변화 없음.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol", meta=(ClampMin="0.0", UIMin="0.0", UIMax="3.0"))
	float HoveredColorMultiplier = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	FLinearColor ProtocolBorderColor = FLinearColor::White;

	// 호버 툴팁이 마우스 커서 기준으로 떨어지는 오프셋(픽셀). X 양수면 커서 오른쪽에 표시된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	FVector2D TooltipCursorOffset = FVector2D(48.0f, 8.0f);

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UBorder> ProtocolBorder;

	// 프로토콜 이름 이미지. 배치한 WBP 에서 인스턴스별로 ProtocolNameTexture 를 지정한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UImage> ProtocolNameImage;

	// ProtocolNameImage 에 표시할 텍스처. 호버 툴팁 아이콘도 같은 텍스처를 쓴다.
	// 미지정이면 WBP 브러시를 그대로 둔다(런타임 경고 로그).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	TObjectPtr<UTexture2D> ProtocolNameTexture;

	// 단계 칸 7개. 순번은 이름 순서(1~7)와 같고, C++ 이 순번 숫자와 해금 색을 채운다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSProtocolStageWidget> ProtocolStage_1;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSProtocolStageWidget> ProtocolStage_2;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSProtocolStageWidget> ProtocolStage_3;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSProtocolStageWidget> ProtocolStage_4;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSProtocolStageWidget> ProtocolStage_5;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSProtocolStageWidget> ProtocolStage_6;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSProtocolStageWidget> ProtocolStage_7;

	// 툴팁이 프로토콜 이름·설명·해금 항목을 찾는 키. 배치한 부모 위젯이 SetProtocolType 으로 채우므로
	// WBP 에서는 지정하지 않는다(지정해도 런타임에 덮어써진다).
	UPROPERTY(BlueprintReadOnly, Category="LS/UI|Protocol")
	ELSProtocolType ProtocolType = ELSProtocolType::Survival;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	TSubclassOf<ULSProtocolTooltipWidget> ProtocolTooltipWidgetClass;

private:
	// 단계 칸 7개를 순번 순서로 모은다(미바인딩이면 null 이 그대로 들어간다).
	TArray<ULSProtocolStageWidget*> GetProtocolStageWidgets() const;
	// 순번이 UnlockedStageCount 이하인 칸만 해금 표시로 갱신한다.
	void RefreshProtocolStages(int32 UnlockedStageCount) const;
	ULSProtocolTooltipWidget* CreateProtocolTooltipWidget();

	// 호버(틴트/툴팁)가 동작하도록 루트를 히트테스트 가능하게 보정한다.
	void EnsureHoverHitTestable();
	// 커서를 따라다니는 호버 툴팁을 직접 띄우고/지우고/위치를 갱신한다. (Slate 자동 툴팁은 커서 기준 위치 제어가 안 됨)
	void ShowProtocolTooltip();
	void HideProtocolTooltip();
	void UpdateTooltipPosition();
	void ApplyProtocolBorderColor(const FLinearColor& InColor) const;

	// 표시 중인 호버 툴팁 인스턴스. 뷰포트에 올라가 있는 동안 GC되지 않도록 UPROPERTY로 추적한다.
	UPROPERTY(Transient)
	TObjectPtr<ULSProtocolTooltipWidget> ActiveTooltipWidget;

	int32 CurrentProtocolLevel = 0;
	int32 PreviousProtocolLevel = 0;
};
