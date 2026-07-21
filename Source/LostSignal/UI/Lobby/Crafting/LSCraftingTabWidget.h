#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSCraftingTabWidget.generated.h"

class UButton;
class UTextBlock;
class ULSCraftingTabWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLSCraftingTabClicked, ULSCraftingTabWidget*, ClickedTab);

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSCraftingTabWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintAssignable, Category="LS/UI|Crafting")
	FLSCraftingTabClicked OnClicked;

	void SetLabel(const FText& NewLabel) const;
	void SetSelected(bool bSelected) const;

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<UButton> Button;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<UTextBlock> LabelText;

private:
	UFUNCTION()
	void HandleButtonClicked();
};
