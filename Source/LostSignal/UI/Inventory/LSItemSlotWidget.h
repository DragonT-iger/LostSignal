#pragma once

#include "CoreMinimal.h"
#include "Session/LSSessionSubsystem.h"
#include "UI/Inventory/LSItemTooltipSlotWidget.h"
#include "LSItemSlotWidget.generated.h"

class UImage;
class ULSInventoryWidget;
class UTextBlock;
class UTexture2D;
class UDragDropOperation;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSItemSlotWidget : public ULSItemTooltipSlotWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void SetItem(FName ItemRowName, int32 Amount);

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void ClearItem();

	void SetSlotContext(ULSInventoryWidget* InInventoryWidget, ELSInventorySlotArea InSlotArea, int32 InSlotIndex, bool bInHasItem);
	void RestoreDragSourceVisual();

protected:
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UImage> ItemIconImage;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> AmountText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	FLinearColor NormalIconTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	FLinearColor HoveredIconTint = FLinearColor(0.55f, 0.9f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	FLinearColor DragTargetIconTint = FLinearColor(1.0f, 0.84f, 0.35f, 1.0f);

private:
	TWeakObjectPtr<ULSInventoryWidget> InventoryWidget;
	ELSInventorySlotArea SlotArea = ELSInventorySlotArea::Inventory;
	int32 SlotIndex = INDEX_NONE;
	bool bHasItem = false;
	bool bIsHovered = false;
	bool bIsDragTarget = false;

	void ApplyHoverVisual();
	bool CanStartInventoryDrag() const;
	bool IsValidInventoryDropTarget(const UDragDropOperation* InOperation) const;
	UTexture2D* LoadIconTextureByRowName(FName ItemRowName) const;
	UTexture2D* LoadDefaultIconTexture() const;
	static FString BuildIconObjectPath(const FString& IconNameOrPath, const FString& BaseFolder);
	static FString GetIconBaseFolderByRowName(FName ItemRowName);
};
