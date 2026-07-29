#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "LSProtocolStageWidget.generated.h"

class UImage;
class UTextBlock;

// 프로토콜 단계 한 칸 위젯 (WBP_ProtocolStage 의 부모 클래스).
//
// 표시 요소
//  - StageImage : 단계 박스. 해금 여부에 따라 틴트 색만 바뀐다(텍스처는 WBP 브러시 그대로).
//  - StageText  : 단계 순번 숫자(1~N). 값은 ULSProtocolWidget 이 순번으로 채운다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSProtocolStageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 단계 한 칸 표시를 갱신한다.
	//  StageOrder : 표시할 순번 숫자 (1부터)
	//  bUnlocked  : 현재 프로토콜 레벨로 해금된 단계인지
	UFUNCTION(BlueprintCallable, Category="LS/UI|Protocol")
	void SetProtocolStage(int32 StageOrder, bool bUnlocked);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Protocol")
	TObjectPtr<UImage> StageImage;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Protocol")
	TObjectPtr<UTextBlock> StageText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	FLinearColor UnlockedBoxColor = FLinearColor(FColor(0x5B, 0xAD, 0xD5));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	FLinearColor LockedBoxColor = FLinearColor(0.22f, 0.22f, 0.22f, 0.55f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	FSlateColor UnlockedTextColor = FSlateColor(FLinearColor::White);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Protocol")
	FSlateColor LockedTextColor = FSlateColor(FLinearColor(0.35f, 0.35f, 0.35f, 0.75f));
};
