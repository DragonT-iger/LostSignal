#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSStorageButtonWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLSStorageButtonClicked);

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSStorageButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintAssignable, Category="LS/UI|Storage")
	FLSStorageButtonClicked OnClicked;

	UFUNCTION(BlueprintCallable, Category="LS/UI|Storage")
	void SetLabelText(const FText& NewText) const;

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<UButton> Button;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<UTextBlock> Text;

private:
	UFUNCTION()
	void HandleButtonClicked();
};
