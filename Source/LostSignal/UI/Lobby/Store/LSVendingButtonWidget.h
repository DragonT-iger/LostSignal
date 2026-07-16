#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSVendingButtonWidget.generated.h"

class UButton;
class UTextBlock;
class ULSVendingButtonWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLSVendingButtonClicked, ULSVendingButtonWidget*, ClickedButton);

// 자판기 화면 공용 버튼(WBP_VendingButton)의 부모 클래스. 분류(카테고리) 버튼 등에 쓴다.
// WBP_StoreButton과 비슷하지만 퀘스트 아이콘이 없는 자판기 전용 스타일이라 클래스를 나눈다.
// 클릭 델리게이트는 자기 포인터를 실어 보내, 자판기 위젯이 어느 버튼인지 구분하게 한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSVendingButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintAssignable, Category="LS/UI|Store")
	FLSVendingButtonClicked OnClicked;

	// 버튼에 표시할 라벨 텍스트를 교체한다.
	void SetLabel(const FText& NewLabel) const;

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UButton> Button;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> LabelText;

private:
	UFUNCTION()
	void HandleButtonClicked();
};
