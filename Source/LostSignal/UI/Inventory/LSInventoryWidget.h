#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Session/LSSessionSubsystem.h"
#include "LSInventoryWidget.generated.h"

class ALSWorldDroppedItem;
class UBorder;
class UButton;
class UDragDropOperation;
class UWrapBox;
class ULSInventoryDragDropOperation;
class ULSItemSlotWidget;
class ULSLootDropWidget;

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

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void RebuildEquipmentSlots();

	// 드래그 중인 아이템이 장착될 장비칸 1개에 후보 하이라이트를 켜고 나머지는 끈다.
	// 장착 불가 아이템이면 어느 칸도 켜지지 않는다. 드래그 종료 시 ClearEquipmentDragHighlight로 전부 끈다.
	void SetEquipmentDragHighlight(FName DraggedItemRowName);
	void ClearEquipmentDragHighlight();

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void SetStoreAllButtonVisible(bool bVisible);

	bool HandleInventorySlotDrop(ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex);
	bool HandleLootSlotDrop(ULSLootDropWidget* LootDropWidget, int32 LootSlotIndex, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex);
	bool TryDropInventoryDragToWorld(const ULSInventoryDragDropOperation& DragOperation, const FPointerEvent& PointerEvent);
	bool IsSlotLocked(ELSInventorySlotArea SlotArea, int32 SlotIndex) const;

protected:
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UBorder> InventoryWindowBorder;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UWrapBox> InventoryWrapBox;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UWrapBox> ConfirmedStorageSlotWrapBox;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UButton> StoreAllButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UButton> SortButton;

	// 장비 장착 슬롯. BindWidget 이름은 장비 타입과 일치시킨다(ELSEquipmentSlot 순서).
	// 무기=Weapon, 프로세서(머리)=Processor, 코어(몸)=Core, 구동계(손)=Actuator, 프레임(발)=Frame.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSItemSlotWidget> WeaponSlot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSItemSlotWidget> ProcessorSlot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSItemSlotWidget> CoreSlot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSItemSlotWidget> ActuatorSlot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSItemSlotWidget> FrameSlot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	TSubclassOf<ULSItemSlotWidget> ItemSlotWidgetClass;

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

	// 장비 슬롯(무기/방어구) 드롭 처리. 로비 전용이며(레이드 중 거부) SaveSubsystem::MoveEquipmentSlot으로 확정한다.
	bool HandleEquipmentSlotDrop(ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex);
	bool HandleInventoryBackgroundDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation);
	bool DropInventoryDragToWorld(const ULSInventoryDragDropOperation& DragOperation, FVector2D ScreenPosition);
	bool IsPointerInsideInventoryWindow(FVector2D ScreenPosition) const;
	bool IsPointerOverUserWidget(const FPointerEvent& PointerEvent) const;
};
