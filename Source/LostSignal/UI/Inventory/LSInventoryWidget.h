#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Session/LSSessionSubsystem.h"
#include "LSInventoryWidget.generated.h"

class ALSWorldDroppedItem;
class UButton;
class UDragDropOperation;
class UWrapBox;
class ULSInventoryItemSlotWidget;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void SetInventorySlotCount(int32 NewInventorySlotCount);

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void SetConfirmedStorageSlotCount(int32 NewConfirmedStorageSlotCount);

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void RebuildInventorySlots();

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void RebuildConfirmedStorageSlots();

	bool HandleInventorySlotDrop(ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex);

protected:
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UWrapBox> InventoryWrapBox;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UWrapBox> ConfirmedStorageSlotWrapBox;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UButton> StoreAllButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UButton> SortButton;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	TSubclassOf<ULSInventoryItemSlotWidget> InventoryItemSlotWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI", meta=(ClampMin="0"))
	int32 InventorySlotCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI", meta=(ClampMin="0"))
	int32 ConfirmedStorageSlotCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	TSubclassOf<ALSWorldDroppedItem> DroppedItemActorClass;

private:
	UFUNCTION()
	void HandleStoreAllButtonClicked();

	UFUNCTION()
	void HandleSortButtonClicked();

	bool HandleInventoryBackgroundDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation);
	bool ResolveDroppedItemLocation(FVector& OutDropLocation) const;
	bool ResolveDroppedItemYaw(float& OutDropYaw) const;
};
