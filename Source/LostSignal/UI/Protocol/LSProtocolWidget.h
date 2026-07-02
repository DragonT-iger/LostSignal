#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Data/LSProtocolTypes.h"
#include "LSProtocolWidget.generated.h"

class UImage;
class ULSProtocolTooltipWidget;
class UTextBlock;
class URichTextBlock;
class UTexture2D;

// 프로토콜 한 줄 위젯 (WBP_Protocol 등 한 칸의 부모 클래스).
//
// 표시 요소
//  - ProtocolNameImage : 프로토콜 이름 이미지. 배치한 쪽(WBP_ChipStation 등)에서
//      인스턴스별로 ProtocolNameTexture 를 지정해 4종 프로토콜 이미지를 구분한다.
//  - LevelText        : 레벨 (숫자 텍스트)
//  - SynergyStageText : 시너지 단계 1~N 을 RichTextBlock 으로 표시.
//      활성 단계(1..SynergyStage)는 <Bold>, 나머지는 <Light> 태그로 감싼다.
//      예) 단계 3 → "<Bold>1/2/3</><Light>/4/5/6/7/8</>"
//
// 데코레이터 태그(Bold/Light)는 RichTextBlock 에 지정한 Text Style Set 의 행 이름과 일치해야 한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSProtocolWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 프로토콜 한 줄 표시를 갱신한다.
	//  Level        : 레벨 (숫자)
	//  SynergyStage : 활성 시너지 단계 (0~SynergyStageCount). Bold 로 표시되는 개수.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Protocol")
	void SetProtocol(int32 Level, int32 SynergyStage);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Protocol")
	void SetProtocolLevels(int32 CurrentLevel, int32 PreviousLevel, int32 SynergyStage);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Protocol")
	void SetProtocolStageCount(int32 InSynergyStageCount);

	void SetProtocolStageLevels(const TArray<int32>& InSynergyStageLevels);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Protocol")
	void SetProtocolType(ELSProtocolType InProtocolType);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 호버 시 프로토콜 한 줄 전체(텍스트·이름 이미지)에 입히는 강조 틴트.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	FLinearColor HoveredTint = FLinearColor(0.55f, 0.9f, 1.0f, 1.0f);

	// 호버 툴팁이 마우스 커서 기준으로 떨어지는 오프셋(픽셀). X 양수면 커서 오른쪽에 표시된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	FVector2D TooltipCursorOffset = FVector2D(48.0f, 8.0f);

	// 프로토콜 이름 이미지. 배치한 WBP 에서 인스턴스별로 ProtocolNameTexture 를 지정한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UImage> ProtocolNameImage;

	// ProtocolNameImage 에 표시할 텍스처. 미지정이면 WBP 브러시를 그대로 둔다(런타임 경고 로그).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	TObjectPtr<UTexture2D> ProtocolNameTexture;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<URichTextBlock> SynergyStageText;

	// 시너지 단계 총 개수 (1~N). 기본 8.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/UI")
	int32 SynergyStageCount = 8;

	TArray<int32> SynergyStageLevels;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	ELSProtocolType ProtocolType = ELSProtocolType::Survival;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	TObjectPtr<UTexture2D> TooltipIconTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	TSubclassOf<ULSProtocolTooltipWidget> ProtocolTooltipWidgetClass;

private:
	// 활성 단계까지 Bold, 나머지 Light 로 감싼 RichText 마크업을 만든다.
	FString BuildSynergyMarkup(int32 ActiveStage) const;
	ULSProtocolTooltipWidget* CreateProtocolTooltipWidget();

	// 호버(틴트/툴팁)가 동작하도록 루트를 히트테스트 가능하게 보정한다.
	void EnsureHoverHitTestable();
	// 커서를 따라다니는 호버 툴팁을 직접 띄우고/지우고/위치를 갱신한다. (Slate 자동 툴팁은 커서 기준 위치 제어가 안 됨)
	void ShowProtocolTooltip();
	void HideProtocolTooltip();
	void UpdateTooltipPosition();

	// 표시 중인 호버 툴팁 인스턴스. 뷰포트에 올라가 있는 동안 GC되지 않도록 UPROPERTY로 추적한다.
	UPROPERTY(Transient)
	TObjectPtr<ULSProtocolTooltipWidget> ActiveTooltipWidget;

	int32 CurrentProtocolLevel = 0;
	int32 PreviousProtocolLevel = 0;
};
