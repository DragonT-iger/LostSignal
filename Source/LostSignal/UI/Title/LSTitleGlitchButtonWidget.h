#pragma once

#include "CoreMinimal.h"
#include "UI/Title/LSTitleMenuButtonWidget.h"
#include "LSTitleGlitchButtonWidget.generated.h"

class UMaterialInstanceDynamic;
class URetainerBox;

// 타이틀 메뉴 버튼에 마우스 Hover 및 키보드/패드 Focus 글리치 상태를 전달한다.
// 평상시의 간헐 글리치는 머티리얼 자체가 처리하고, 상호작용 중에는 HoverAmount를 1로 유지한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSTitleGlitchButtonWidget : public ULSTitleMenuButtonWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

protected:
	// WBP에서 기존 Button/Text를 감싸는 RetainerBox.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Title")
	TObjectPtr<URetainerBox> GlitchRetainer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Title")
	FName HoverAmountParameterName = TEXT("HoverAmount");

private:
	UFUNCTION()
	void HandleGlitchHovered();

	UFUNCTION()
	void HandleGlitchUnhovered();

	void InitializeGlitchMaterial();
	void UpdateGlitchInteractionState();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlitchMaterialInstance;

	bool bMouseHovered = false;
	bool bFocusWithin = false;
};
