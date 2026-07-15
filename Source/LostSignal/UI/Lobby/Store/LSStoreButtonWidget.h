#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSStoreButtonWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class ULSStoreButtonWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLSStoreButtonClicked, ULSStoreButtonWidget*, ClickedButton);

// 상점 화면(WBP_Store)에서 쓰는 공용 버튼(WBP_StoreButton)의 부모 클래스. Border 안에 Button + 라벨 텍스트 +
// 퀘스트 아이콘 구조. 라벨과 아이콘 표시는 C++에서 상태별로 갈아끼우므로 둘 다 바인딩한다.
// 클릭 델리게이트는 자기 포인터를 실어 보내, 상점 위젯이 어느 버튼인지 구분하게 한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSStoreButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintAssignable, Category="LS/UI|Store")
	FLSStoreButtonClicked OnClicked;

	// 버튼에 표시할 라벨 텍스트를 교체한다.
	void SetLabel(const FText& NewLabel) const;

	// 퀘스트 아이콘 표시 여부. 숨길 때는 Collapsed로 라벨이 중앙 정렬을 유지하게 한다.
	void SetQuestIconVisible(bool bVisible) const;

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UButton> Button;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> LabelText;

	// 퀘스트 대화 표시용 아이콘. 브러시는 WBP에서 매핑하고 C++은 표시 여부만 제어한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UImage> QuestIcon;

private:
	UFUNCTION()
	void HandleButtonClicked();
};
