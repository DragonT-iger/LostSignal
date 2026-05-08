#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "LSInventoryItemSlotWidget.generated.h"

class UImage;
class ULSInventoryWidget;
class UTextBlock;
class UTexture2D;
class UDragDropOperation;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSInventoryItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void SetItem(FName ItemRowName, int32 Amount);

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void ClearItem();

	void SetSlotContext(ULSInventoryWidget* InInventoryWidget, int32 InSlotIndex, bool bInHasItem);
	void RestoreDragSourceVisual();

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UImage> ItemIconImage;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> AmountText;

private:
	TWeakObjectPtr<ULSInventoryWidget> InventoryWidget;
	int32 SlotIndex = INDEX_NONE;
	bool bHasItem = false;

	UTexture2D* LoadIconTextureByRowName(FName ItemRowName) const;
	UTexture2D* LoadDefaultIconTexture() const;
	static FString BuildIconObjectPath(const FString& IconNameOrPath, const FString& BaseFolder);
	static FString GetIconBaseFolderByRowName(FName ItemRowName);
};
