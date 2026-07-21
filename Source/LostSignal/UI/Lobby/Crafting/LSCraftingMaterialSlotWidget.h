#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSCraftingMaterialSlotWidget.generated.h"

class UTextBlock;
class ULSItemSlotWidget;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSCraftingMaterialSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	void SetMaterial(FName ItemRowName, int32 OwnedAmount, int32 RequiredAmount) const;

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<ULSItemSlotWidget> ItemSlot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<UTextBlock> AmountText;
};
