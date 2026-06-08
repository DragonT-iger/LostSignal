#pragma once

#include "CoreMinimal.h"
#include "Session/LSSessionSubsystem.h"
#include "UI/Inventory/LSItemTooltipSlotWidget.h"
#include "LSItemSlotWidget.generated.h"

class UImage;
class ULSChipEquipmentSlotWidget;
class ULSChipStationWidget;
class ULSInventoryWidget;
class ULSLobbyStorageWidget;
class ULSLootDropWidget;
class UTextBlock;
class UTexture2D;
class UDragDropOperation;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSItemSlotWidget : public ULSItemTooltipSlotWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void SetItem(FName ItemRowName, int32 Amount, const TArray<FLSChipResolvedStat>& ChipStats);

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void ClearItem();

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void SetDefaultSlotTexture(UTexture2D* InDefaultSlotTexture);

	void SetDisplayOnlySlotContext();
	void SetSlotContext(ULSInventoryWidget* InInventoryWidget, ELSInventorySlotArea InSlotArea, int32 InSlotIndex, bool bInHasItem);
	void SetLootSlotContext(ULSLootDropWidget* InLootDropWidget, int32 InSlotIndex, bool bInHasItem);
	void SetWarehouseSlotContext(ULSLobbyStorageWidget* InStorageWidget, ELSInventorySlotArea InSlotArea, int32 InSlotIndex, bool bInHasItem);
	void SetChipStationSlotContext(ULSChipStationWidget* InChipStationWidget, ELSInventorySlotArea InSourceArea, int32 InSourceSlotIndex, FName InItemRowName, int32 InAmount, const TArray<FLSChipResolvedStat>& InChipStats);
	void SetChipEquipmentSlotContext(ULSChipEquipmentSlotWidget* InChipEquipmentSlotWidget, ULSChipStationWidget* InChipStationWidget, int32 InEquipmentSlotIndex);
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTexture2D> DefaultSlotTexture;

private:
	TWeakObjectPtr<ULSInventoryWidget> InventoryWidget;
	TWeakObjectPtr<ULSLootDropWidget> LootDropWidget;
	TWeakObjectPtr<ULSLobbyStorageWidget> LobbyStorageWidget;
	TWeakObjectPtr<ULSChipStationWidget> ChipStationWidget;
	TWeakObjectPtr<ULSChipEquipmentSlotWidget> ChipEquipmentSlotWidget;
	ELSInventorySlotArea SlotArea = ELSInventorySlotArea::Inventory;
	int32 SlotIndex = INDEX_NONE;
	int32 EquipmentSlotIndex = INDEX_NONE;
	FName DragItemRowName;
	int32 DragAmount = 0;
	TArray<FLSChipResolvedStat> DragChipStats;
	bool bHasItem = false;
	bool bIsHovered = false;
	bool bIsDragTarget = false;

	void ApplyHoverVisual();
	bool CanStartItemDrag() const;
	bool IsValidInventoryDropTarget(const UDragDropOperation* InOperation) const;
	bool IsValidLootDropTarget(const UDragDropOperation* InOperation) const;
	bool IsValidWarehouseDropTarget(const UDragDropOperation* InOperation) const;
	UTexture2D* LoadIconTextureByRowName(FName ItemRowName) const;
	UTexture2D* LoadSlotDefaultTexture() const;
	UTexture2D* LoadDefaultIconTexture() const;
	static FString BuildIconObjectPath(const FString& IconNameOrPath, const FString& BaseFolder);
	static FString GetIconBaseFolderByRowName(FName ItemRowName);
};
