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

	// 리빌드 직후 커서가 여전히 이 슬롯 위에 있으면 호버 강조를 복원한다.
	// 풀링 재사용으로 위젯 인스턴스가 그대로라 Slate가 MouseEnter를 다시 쏘지 않기 때문이다.
	void RefreshHoverStateFromCursor();

	// 이 슬롯에 아이템이 들어 있는지. 빈 슬롯(hole) 재사용 여부 판단 등에 쓴다.
	bool HasItem() const { return bHasItem; }

	// 루트 단계 공개: 아직 공개되지 않은 미공개 슬롯으로 만든다(미확인 아이콘 + 펄스, 클릭/드래그/줍기 불가).
	// 이후 SetItem이 호출되면 placeholder→아이템 전환을 감지해 등장 pop-in을 재생한다.
	void SetPlaceholder();

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
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

	// 아이템 등급별 슬롯 배경색. 등급은 Row Name 토큰에서 파싱한다(LSInventorySlotUtils::ResolveItemGradeFromRowName).
	// 등급이 없거나 알 수 없는 아이템은 DefaultGradeColor, 아이템이 없는 빈 슬롯은 EmptySlotBackgroundColor를 쓴다.
	// FColor(sRGB) → FLinearColor 변환 생성자를 써서 에디터 색 선택기와 동일한 색으로 표시한다. (#124B6B 짙은 청록 블루)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|GradeColor")
	FLinearColor DefaultGradeColor = FLinearColor(FColor(0x12, 0x4B, 0x6B));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|GradeColor")
	FLinearColor EmptySlotBackgroundColor = FLinearColor(FColor(0x12, 0x4B, 0x6B));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|GradeColor")
	FLinearColor SupplyGradeColor = FLinearColor(0.62f, 0.62f, 0.62f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|GradeColor")
	FLinearColor StandardGradeColor = FLinearColor(0.45f, 0.85f, 0.45f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|GradeColor")
	FLinearColor PrecisionGradeColor = FLinearColor(0.35f, 0.6f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|GradeColor")
	FLinearColor TuningGradeColor = FLinearColor(0.7f, 0.4f, 0.95f, 1.0f);

	// 프로토타입은 노랑 계열로, 마스터피스는 빨강으로 둬서 서로 확실히 구분한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|GradeColor")
	FLinearColor PrototypeGradeColor = FLinearColor(1.0f, 0.82f, 0.25f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|GradeColor")
	FLinearColor MasterpieceGradeColor = FLinearColor(0.95f, 0.22f, 0.18f, 1.0f);

	// 호버/드래그 타겟일 때 슬롯을 키워 강조하는 배율. (1.0, 1.0)이면 크기 변화 없음.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	FVector2D HoveredRenderScale = FVector2D(1.1f, 1.1f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTexture2D> DefaultSlotTexture;

	// 루트 단계 공개 연출(전부 C++ NativeTick 구동). 수치는 연출 튜닝용이라 에디터에서 조정.
	// 등장 pop-in 지속 시간(초)과 시작 스케일(1.0까지 커짐).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Loot")
	float PopInDuration = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Loot")
	float PopInStartScale = 0.7f;

	// 미공개 placeholder 펄스 속도(Hz)와 아이콘 알파 범위(Min↔Max).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Loot")
	float PlaceholderPulseSpeed = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Loot")
	float PlaceholderPulseMinAlpha = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Loot")
	float PlaceholderPulseMaxAlpha = 0.85f;

	// 미공개 슬롯에 표시할 "미확인" 아이콘. 아트가 WBP 기본값으로 에셋만 매핑한다(로직 아님).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Loot")
	TObjectPtr<UTexture2D> UnconfirmedIconTexture;

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
	// 현재 표시 중인 아이템 등급에 해당하는 배경색. 빈 슬롯·등급 없는 아이템은 DefaultGradeColor.
	FLinearColor CurrentGradeBackgroundColor = FLinearColor::White;

	// 루트 단계 공개: 미공개 placeholder 상태(클릭/드래그/줍기 차단, 아이콘 펄스).
	bool bIsPlaceholder = false;
	// 등장 pop-in 애니 진행 상태(NativeTick에서 스케일/투명도 보간).
	bool bIsPopInAnimating = false;
	float PopInElapsed = 0.f;

	// 현재 아이콘 브러시를 식별하는 키. 같은 아이템을 다시 표시할 때 동기 텍스처 로딩을 건너뛰기 위한 캐시다.
	// 아이템 행 이름, 빈 슬롯 키, NAME_None(미적용/로드 실패) 중 하나를 가진다.
	FName DisplayedIconKey;

	void ApplyHoverVisual();
	void ApplySlotBackground();
	// 아이템 Row Name의 등급 토큰을 슬롯 배경색으로 매핑한다. 등급이 없으면 DefaultGradeColor를 반환.
	FLinearColor ResolveGradeBackgroundColor(FName ItemRowName) const;
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
	bool IsValidChipEquipmentDropTarget(const UDragDropOperation* InOperation) const;
	bool IsValidChipStationDropTarget(const UDragDropOperation* InOperation) const;
	UTexture2D* LoadIconTextureByRowName(FName ItemRowName) const;
	UTexture2D* LoadDefaultIconTexture() const;
	static FString BuildIconObjectPath(const FString& IconNameOrPath, const FString& BaseFolder);
	static FString GetIconBaseFolderByRowName(FName ItemRowName);
};
