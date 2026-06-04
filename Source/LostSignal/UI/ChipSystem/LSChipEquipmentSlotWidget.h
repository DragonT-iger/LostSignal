#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Session/LSSessionSubsystem.h"
#include "LSChipEquipmentSlotWidget.generated.h"

class UDragDropOperation;
class ULSChipStationWidget;
class ULSInventoryDragDropOperation;
class ULSItemSlotWidget;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSChipEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void SetEquipmentSlotContext(ULSChipStationWidget* InChipStationWidget, int32 InEquipmentSlotIndex);
	void SetEquipmentItem(const FLSSessionItem& Item);
	bool HandleChipDrop(const ULSInventoryDragDropOperation& DragOperation);
	void ClearEquipmentSlot();

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSItemSlotWidget> ItemSlot;

private:
	TWeakObjectPtr<ULSChipStationWidget> ChipStationWidget;
	int32 EquipmentSlotIndex = INDEX_NONE;
};
