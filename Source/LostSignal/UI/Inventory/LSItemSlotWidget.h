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
	void SetSlotLocked(bool bInLocked);

	void SetDisplayOnlySlotContext();
	void SetSlotContext(ULSInventoryWidget* InInventoryWidget, ELSInventorySlotArea InSlotArea, int32 InSlotIndex, bool bInHasItem, bool bInLocked = false);
	void SetLootSlotContext(ULSLootDropWidget* InLootDropWidget, int32 InSlotIndex, bool bInHasItem);
	void SetWarehouseSlotContext(ULSLobbyStorageWidget* InStorageWidget, ELSInventorySlotArea InSlotArea, int32 InSlotIndex, bool bInHasItem);
	void SetChipStationSlotContext(ULSChipStationWidget* InChipStationWidget, ELSInventorySlotArea InSourceArea, int32 InSourceSlotIndex, FName InItemRowName, int32 InAmount, const TArray<FLSChipResolvedStat>& InChipStats);
	void SetChipEquipmentSlotContext(ULSChipEquipmentSlotWidget* InChipEquipmentSlotWidget, ULSChipStationWidget* InChipStationWidget, int32 InEquipmentSlotIndex);
	void RestoreDragSourceVisual();

	// 슬롯 위젯을 재사용할 때 이전 상호작용의 잔여 시각 상태를 초기화한다.
	void ResetTransientSlotState();

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	// 항상 표시되는 슬롯 배경 프레임. 아이템 아이콘과 분리되어 아이템이 있어도 배경이 사라지지 않는다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UImage> SlotBackgroundImage;

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
	FLinearColor LockedIconTint = FLinearColor(0.35f, 0.35f, 0.35f, 0.65f);

	// 호버/드래그 타겟일 때 슬롯을 키워 강조하는 배율. (1.0, 1.0)이면 크기 변화 없음.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	FVector2D HoveredRenderScale = FVector2D(1.1f, 1.1f);

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
	bool bIsLocked = false;
	bool bIsHovered = false;
	bool bIsDragTarget = false;
	// 드래그 중 커서를 따라가는 비주얼 인스턴스 표시. 이 슬롯은 아이템 아이콘만 보이고 배경 프레임은 숨긴다.
	bool bIsDragVisual = false;

	// 현재 아이콘 브러시를 식별하는 키. 같은 아이템을 다시 표시할 때 동기 텍스처 로딩을 건너뛰기 위한 캐시다.
	// 아이템 행 이름, 빈 슬롯 키, NAME_None(미적용/로드 실패) 중 하나를 가진다.
	FName DisplayedIconKey;

	void ApplyHoverVisual();
	void ApplySlotBackground();
	bool CanStartItemDrag() const;
	bool IsQuickTransferPointerEvent(const FPointerEvent& InMouseEvent) const;
	bool TryHandleQuickTransfer();
	bool TryHandleLootQuickTransfer();
	bool TryHandleInventoryQuickTransfer();
	bool TryHandleWarehouseQuickTransfer();
	bool TryHandleChipEquipmentQuickTransfer();
	bool TryHandleChipStationQuickTransfer();
	void RefreshStoredSlotVisual();
	bool IsValidInventoryDropTarget(const UDragDropOperation* InOperation) const;
	bool IsValidLootDropTarget(const UDragDropOperation* InOperation) const;
	bool IsValidWarehouseDropTarget(const UDragDropOperation* InOperation) const;
	UTexture2D* LoadIconTextureByRowName(FName ItemRowName) const;
	UTexture2D* LoadDefaultIconTexture() const;
	static FString BuildIconObjectPath(const FString& IconNameOrPath, const FString& BaseFolder);
	static FString GetIconBaseFolderByRowName(FName ItemRowName);
};
