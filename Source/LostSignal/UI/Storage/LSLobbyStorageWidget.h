#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Session/LSSessionSubsystem.h"
#include "LSLobbyStorageWidget.generated.h"

class ALSWorldDroppedItem;
class UDragDropOperation;
class ULSInventoryDragDropOperation;
class ULSItemSlotWidget;
class ULSStorageButtonWidget;
class ULSSaveSubsystem;
class UTextBlock;
class UWrapBox;

UENUM(BlueprintType)
enum class ELSStorageFilter : uint8
{
	All,
	Weapon,
	Armor,
	Consumable,
	Misc,
	Chip,
};

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSLobbyStorageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="LS/UI|Storage")
	void SetFilter(ELSStorageFilter NewFilter);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Storage")
	void RefreshStorage();

	void RefreshStorageCountText();

	UFUNCTION(BlueprintPure, Category="LS/UI|Storage")
	int32 GetMaxStorageSlotCount() const { return MaxStorageSlotCount; }

	bool HandleStorageSlotDrop(ELSInventorySlotArea FromArea, int32 FromIndex, int32 ToWarehouseIndex);
	bool TryDropStorageDragToWorld(const ULSInventoryDragDropOperation& DragOperation, const FPointerEvent& PointerEvent);
	bool TransferStorageSlotToInventory(int32 WarehouseSlotIndex, bool bRefreshSourceStorage = true);

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<UWrapBox> StorageSlotWrapBox;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<UTextBlock> StorageCountText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<ULSStorageButtonWidget> SortButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<ULSStorageButtonWidget> AllTabButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<ULSStorageButtonWidget> WeaponTabButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<ULSStorageButtonWidget> ArmorTabButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<ULSStorageButtonWidget> ConsumableTabButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<ULSStorageButtonWidget> MiscTabButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<ULSStorageButtonWidget> ChipTabButton;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Storage")
	TSubclassOf<ULSItemSlotWidget> ItemSlotWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Storage", meta=(ClampMin="0"))
	int32 MaxStorageSlotCount = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Storage")
	TSubclassOf<ALSWorldDroppedItem> DroppedItemActorClass;

private:
	ELSStorageFilter CurrentFilter = ELSStorageFilter::All;

	UFUNCTION()
	void HandleSortButtonClicked();

	UFUNCTION()
	void HandleAllTabButtonClicked();

	UFUNCTION()
	void HandleWeaponTabButtonClicked();

	UFUNCTION()
	void HandleArmorTabButtonClicked();

	UFUNCTION()
	void HandleConsumableTabButtonClicked();

	UFUNCTION()
	void HandleMiscTabButtonClicked();

	UFUNCTION()
	void HandleChipTabButtonClicked();

	void BindStorageButtons();
	void UnbindStorageButtons();
	void UpdateStorageCountText(const TArray<FLSSessionItem>& StashItems) const;
	void ApplyFilterButtonState() const;
	void BuildFilteredItems(const TArray<FLSSessionItem>& StashItems, TArray<TPair<int32, FLSSessionItem>>& OutIndexedItems) const;
	bool DoesItemMatchCurrentFilter(FName ItemRowName) const;
	bool IsConsumableItem(FName ItemRowName) const;
	bool IsPointerOverUserWidget(const FPointerEvent& PointerEvent) const;
	ULSSaveSubsystem* GetSaveSubsystem() const;
};
