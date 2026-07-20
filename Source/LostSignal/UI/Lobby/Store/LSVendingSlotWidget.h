#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Session/LSSessionSubsystem.h"
#include "LSVendingSlotWidget.generated.h"

class UBorder;
class UDragDropOperation;
class UTextBlock;
class ULSItemSlotWidget;
class ULSVendingSlotWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLSVendingSlotClicked, ULSVendingSlotWidget*, ClickedSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLSVendingSlotDropped, ULSVendingSlotWidget*, SourceSlot, ULSVendingSlotWidget*, TargetSlot);

// 자판기 화면의 아이템 칸(WBP_VendingSlot) 부모 클래스. 자판기 판매 목록과 내(가방/안전/창고) 아이템 표시를 겸한다.
// 아이콘/수량/등급 배경/호버 강조는 내부의 표시 전용 ULSItemSlotWidget(WBP_ItemSlot)이 그대로 그리고,
// 이 위젯은 가격 표시(자판기 전용)와 클릭 선택, 선택 강조(SlotBorder 색 변경)만 담당한다.
// 클릭은 별도 버튼 없이 마우스 다운을 직접 받는다(표시 전용 ItemSlot은 클릭을 소비하지 않고 흘려보낸다).
// 내 아이템 칸끼리는 드래그 앤 드롭으로 이동/스왑할 수 있다(자판기 상품 칸은 드래그 불가/드롭 불가).
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSVendingSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// 좌클릭 시 자기 포인터를 실어 선택 알림을 쏜다. 내 아이템 칸이면 드래그 감지도 함께 무장한다.
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(BlueprintAssignable, Category="LS/UI|Store")
	FLSVendingSlotClicked OnClicked;

	// 내 아이템 칸에 다른 내 아이템 칸이 드롭되면 발행된다. 실제 이동/스왑은 자판기 위젯이 처리한다.
	UPROPERTY(BlueprintAssignable, Category="LS/UI|Store")
	FLSVendingSlotDropped OnDropped;

	// 자판기 판매 목록 칸으로 설정한다. 가격을 표시하고 수량은 표시하지 않는다.
	void SetStockItem(FName InItemRowName, int32 InPrice);

	// 내 아이템(가방/안전/창고) 칸으로 설정한다. 가격은 숨기고 수량을 표시한다.
	void SetOwnedItem(FName InItemRowName, int32 InAmount, ELSInventorySlotArea InArea, int32 InSlotIndex);

	// 내 영역의 빈 칸으로 설정한다. 빈 슬롯 배경만 그리고 클릭해도 선택되지 않는다.
	void SetOwnedEmpty(ELSInventorySlotArea InArea, int32 InSlotIndex);

	// 선택 강조 표시. 상세 패널에 표시 중인 칸의 SlotBorder를 선택 색으로 바꾼다.
	void SetSelected(bool bInSelected);

	FName GetItemRowName() const { return ItemRowName; }
	int32 GetPrice() const { return Price; }
	int32 GetAmount() const { return Amount; }
	bool IsStockSlot() const { return bStockSlot; }
	ELSInventorySlotArea GetArea() const { return Area; }
	int32 GetSlotIndex() const { return SlotIndex; }

protected:
	// 칸 전체 테두리. 선택되면 SelectedBorderColor로, 해제되면 WBP에서 설정한 원래 색으로 되돌린다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UBorder> SlotBorder;

	// 아이콘/수량/등급 배경/호버 강조 표시용. C++이 표시 전용 컨텍스트로 설정한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<ULSItemSlotWidget> ItemSlot;

	// 가격 영역(코인 아이콘 + 가격 텍스트 묶음). 내 아이템 칸에서는 통째로 숨긴다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UWidget> PriceBox;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> PriceText;

	// 선택된 칸의 테두리 색.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Store")
	FLinearColor SelectedBorderColor = FLinearColor(0.35f, 0.85f, 1.0f, 1.0f);

private:
	// 아이템이 든 내 아이템 칸인지. 드래그 출발지 판정용.
	bool CanDragFrom() const;

	// 이 드래그 오퍼레이션이 이 칸에 드롭 가능한지(출발지가 내 아이템 칸이고 도착지가 내 영역인지).
	bool CanAcceptDrop(const UDragDropOperation* InOperation) const;

	// 풀링 재사용 대비: 내용이 바뀔 때 이전 선택/드래그 강조 상태를 지운다.
	void ResetHighlightState();

	// 선택/드래그 호버 상태를 반영해 SlotBorder 색을 적용한다.
	void ApplyBorderColor() const;

	FName ItemRowName;
	int32 Price = 0;
	int32 Amount = 0;
	bool bStockSlot = false;
	ELSInventorySlotArea Area = ELSInventorySlotArea::Inventory;
	int32 SlotIndex = INDEX_NONE;
	bool bIsSelected = false;
	// 드래그 중인 아이템이 이 칸 위에 있을 때의 강조 상태.
	bool bIsDragHover = false;

	// WBP에서 설정한 SlotBorder 원래 색. 선택 해제 시 복원용으로 NativeConstruct에서 캐시한다.
	FLinearColor NormalBorderColor = FLinearColor::White;
};
