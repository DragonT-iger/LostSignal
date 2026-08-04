#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSQuickSlotWidget.generated.h"

class ULSItemSlotWidget;
class UTextBlock;
class UDragDropOperation;
class UInputAction;
class ULSSaveSubsystem;

/**
 * 퀵슬롯 한 칸. 소모품 RowName 참조 하나를 가리키며, 표시 개수는 인벤토리에서 실시간 합산한다.
 * 아이템 스택을 담지 않는다. 인벤토리 슬롯에서 드래그앤드랍하면 그 소모품이 이 칸에 등록된다.
 * WBP는 표시 전용 ItemSlot과 BindKeyText를 바인딩해야 한다.
 */
UCLASS()
class LOSTSIGNAL_API ULSQuickSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 이 칸이 담당할 퀵슬롯 인덱스(0-based)를 지정한다. 바 위젯이 생성 직후 1회 호출한다.
	void InitializeSlot(int32 InSlotIndex);
	// 인벤토리 안에 배치된 바의 슬롯만 마우스 편집(호버·우클릭·드롭)을 허용한다.
	void SetInventoryInteractionEnabled(bool bEnabled);

	// 등록된 소모품의 아이콘과 인벤토리 합산 개수를 다시 그린다. 비어 있으면 아이콘을 숨긴다.
	void Refresh();

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	// 아이콘·수량·등급/빈 슬롯 배경·호버 연출을 담당하는 표시 전용 WBP_ItemSlot.
	// 입력은 퀵슬롯 루트가 받도록 NativeConstruct에서 HitTestInvisible로 고정한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|QuickSlot")
	TObjectPtr<ULSItemSlotWidget> ItemSlot;

	// 이 칸의 소비 바인딩 키(Item1~6Action)를 표시하는 텍스트. 스킬 슬롯의 ShortcutText와 동일 역할.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|QuickSlot")
	TObjectPtr<UTextBlock> BindKeyText;

private:
	// 이 칸의 바인딩 키를 다시 그린다. 레이드 HUD에선 실제 매핑 키, 폰이 없는 로비에선 빈 텍스트.
	void RefreshShortcutText();
	FText ResolveShortcutText() const;
	// InputAction에 매핑된 첫 유효 키의 표시 이름(키보드 우선). 매핑이 없으면 빈 텍스트.
	FText ResolveShortcutTextFromInputMappings(const UInputAction* InputAction) const;

	ULSSaveSubsystem* ResolveSaveSubsystem() const;
	// 등록된 소모품(RowName)이 현재 인벤토리에 몇 개 있는지 합산한다(레이드=세션, 로비=세이브).
	int32 CountOwnedAmount(FName ItemRowName) const;
	// 호버 상태에 맞춰 틴트/스케일/배경색을 다시 적용한다. 호버 진입·이탈·Refresh 뒤 단일 진입점.
	void ApplyHoverVisual();
	// 이 슬롯이 인벤토리 바 소속이고 인벤토리 패널도 열려 있는지 확인한다.
	bool IsInventoryInteractionAllowed() const;

	int32 SlotIndex = INDEX_NONE;
	bool bInventoryInteractionEnabled = false;
	bool bIsHovered = false;
};
