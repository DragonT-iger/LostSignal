#pragma once

#include "CoreMinimal.h"
#include "Session/LSSessionSubsystem.h"
#include "UI/Common/LSLayoutRevealWidget.h"
#include "LSInventoryWidget.generated.h"

class ALSWorldDroppedItem;
class UBorder;
class UButton;
class UDragDropOperation;
class UWrapBox;
class ULSConfirmDialogWidget;
class ULSInventoryDragDropOperation;
class ULSItemSlotWidget;
class ULSLootDropWidget;
class ULSQuickSlotBarWidget;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSInventoryWidget : public ULSLayoutRevealWidget
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
	bool CanAcceptEquipmentDrop(FName ItemRowName, int32 EquipmentSlotIndex) const;

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void SetStoreAllButtonVisible(bool bVisible);

	// 인벤토리에 배치된 퀵슬롯 바 표시를 켜고 끈다. Tab 인벤토리에서만 켜고 루팅 박스로 연 인벤토리에선 끈다.
	void SetQuickSlotBarVisible(bool bVisible);

	bool HandleInventorySlotDrop(ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex);
	bool HandleLootSlotDrop(ULSLootDropWidget* LootDropWidget, int32 LootSlotIndex, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex);
	bool TryDropInventoryDragToWorld(const ULSInventoryDragDropOperation& DragOperation, const FPointerEvent& PointerEvent);
	bool IsSlotLocked(ELSInventorySlotArea SlotArea, int32 SlotIndex) const;

	// 장착칸 Shift 빠른이동 시 인벤토리에 빈 칸이 없을 때 "인벤토리가 가득 찼습니다" 알림을 띄운다.
	// (칩 스테이션의 용량 차단 알림과 동일 패턴 — 다음 틱 생성 + 중복 방지)
	void ShowInventoryFullNotification();

	// 로비 루트의 포커스 회수 예외와 패널 전환 시 고아 모달 정리에 사용한다.
	bool HasActiveNotificationDialog() const;
	void CloseActiveNotificationDialog();

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

	// 인벤토리 안에 배치되는 퀵슬롯 바(소모품 6칸). 여는 경로에 따라 표시가 켜지고 꺼진다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UUserWidget> QuickSlotBar;

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

	// 인벤토리 화면에서 사용하는 일반/보호/장비 슬롯의 레이아웃 크기.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI")
	FVector2D InventoryItemSlotSize = FVector2D(80.f, 80.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	TSubclassOf<ALSWorldDroppedItem> DroppedItemActorClass;

	// 인벤토리 가득참 알림에 쓰는 확인 다이얼로그. WBP_Inventory에서 WBP_ConfirmDialog를 매핑한다(아트 매핑 필요).
	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	TSubclassOf<ULSConfirmDialogWidget> ConfirmDialogClass;

private:
	UFUNCTION()
	void HandleStoreAllButtonClicked();

	UFUNCTION()
	void HandleSortButtonClicked();

	// 알림 다이얼로그 닫힘 콜백(확인/취소/ESC 공통). 참조만 비운다.
	UFUNCTION()
	void HandleNotificationDialogClosed();

	// ShowInventoryFullNotification이 다음 틱에 호출하는 실제 다이얼로그 생성부.
	void PresentInventoryFullNotification();

	// 장비 슬롯(무기/방어구) 드롭 처리. 로비=SaveSubsystem::MoveEquipmentSlot, 레이드=서버 DropInventorySlot 라우팅.
	bool HandleEquipmentSlotDrop(ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex);
	bool HandleInventoryBackgroundDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation);
	bool DropInventoryDragToWorld(const ULSInventoryDragDropOperation& DragOperation, FVector2D ScreenPosition);
	bool IsPointerInsideInventoryWindow(FVector2D ScreenPosition) const;
	bool IsPointerOverUserWidget(const FPointerEvent& PointerEvent) const;

	// 현재 떠 있는 알림 다이얼로그. 중복 생성을 막는 데 쓴다.
	UPROPERTY(Transient)
	TObjectPtr<ULSConfirmDialogWidget> ActiveNotificationDialog;
};
