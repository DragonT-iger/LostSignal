#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSTitleMenuButtonWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLSTitleMenuButtonClicked);

// 타이틀 메뉴 항목 버튼. Button 아래 자식으로 Text를 둔다.
// WBP_TitleMenuButton의 부모 클래스로 사용한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSTitleMenuButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintAssignable, Category="LS/UI|Title")
	FLSTitleMenuButtonClicked OnClicked;

	UFUNCTION(BlueprintCallable, Category="LS/UI|Title")
	void SetLabelText(const FText& NewText) const;

	UFUNCTION(BlueprintCallable, Category="LS/UI|Title")
	void SetButtonEnabled(bool bEnabled);

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Title")
	TObjectPtr<UButton> Button;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Title")
	TObjectPtr<UTextBlock> Text;

private:
	UFUNCTION()
	void HandleButtonClicked();
};
