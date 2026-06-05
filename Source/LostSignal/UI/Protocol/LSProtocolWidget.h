#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "LSProtocolWidget.generated.h"

class ULSProtocolTooltipWidget;
class UTextBlock;
class URichTextBlock;
class UTexture2D;

UENUM(BlueprintType)
enum class ELSProtocolType : uint8
{
	Survival,
	Carrying,
	Battle,
	Navigation
};

// 프로토콜 한 줄 위젯 (WBP_Protocol 등 한 칸의 부모 클래스).
// 프로토콜 이름은 WBP 에서 이미지로 직접 표시한다(여기서 다루지 않음).
//
// 표시 요소
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
	void SetProtocolType(ELSProtocolType InProtocolType);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<URichTextBlock> SynergyStageText;

	// 시너지 단계 총 개수 (1~N). 기본 8.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/UI")
	int32 SynergyStageCount = 8;

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
	void RefreshProtocolTooltip();
};
