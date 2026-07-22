#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSQuickSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UDragDropOperation;
class ULSSaveSubsystem;

/**
 * 퀵슬롯 한 칸. 소모품 RowName 참조 하나를 가리키며, 표시 개수는 인벤토리에서 실시간 합산한다.
 * 아이템 스택을 담지 않는다. 인벤토리 슬롯에서 드래그앤드랍하면 그 소모품이 이 칸에 등록된다.
 * WBP는 IconImage, AmountText를 바인딩해야 한다.
 */
UCLASS()
class LOSTSIGNAL_API ULSQuickSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 이 칸이 담당할 퀵슬롯 인덱스(0-based)를 지정한다. 바 위젯이 생성 직후 1회 호출한다.
	void InitializeSlot(int32 InSlotIndex);

	// 등록된 소모품의 아이콘과 인벤토리 합산 개수를 다시 그린다. 비어 있으면 아이콘을 숨긴다.
	void Refresh();

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|QuickSlot")
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|QuickSlot")
	TObjectPtr<UTextBlock> AmountText;

private:
	ULSSaveSubsystem* ResolveSaveSubsystem() const;
	// 등록된 소모품(RowName)이 현재 인벤토리에 몇 개 있는지 합산한다(레이드=세션, 로비=세이브).
	int32 CountOwnedAmount(FName ItemRowName) const;

	int32 SlotIndex = INDEX_NONE;
};
