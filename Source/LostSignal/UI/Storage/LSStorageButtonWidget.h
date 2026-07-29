#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSStorageButtonWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLSStorageButtonClicked);

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSStorageButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 선택된 탭은 호버와 같은 색으로 고정해, 어떤 탭이 활성인지 색으로만 구분한다.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Storage")
	void SetSelected(bool bInSelected);

	UPROPERTY(BlueprintAssignable, Category="LS/UI|Storage")
	FLSStorageButtonClicked OnClicked;

protected:
	// 라벨/아이콘 등 버튼 안쪽 표시물은 WBP에서 자유롭게 구성한다. C++은 클릭만 중계한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<UButton> Button;

	// 버튼 배경 브러시에 곱해지는 틴트. WBP의 브러시 이미지·모서리 설정은 그대로 두고 색만 덮는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Storage")
	FLinearColor NormalColor = FLinearColor(FColor(0x2A, 0x30, 0x3C));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Storage")
	FLinearColor HoveredColor = FLinearColor(FColor(0x5B, 0x9B, 0xC4));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Storage")
	FLinearColor PressedColor = FLinearColor(FColor(0x3F, 0x77, 0x9C));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Storage")
	FLinearColor SelectedColor = FLinearColor(FColor(0x5B, 0x9B, 0xC4));

private:
	UFUNCTION()
	void HandleButtonClicked();

	void ApplyButtonColors() const;

	bool bIsSelected = false;
};
