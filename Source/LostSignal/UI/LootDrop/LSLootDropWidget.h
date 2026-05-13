#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/LSDropSubsystem.h"
#include "Session/LSSessionSubsystem.h"
#include "LSLootDropWidget.generated.h"

class ULSItemSlotWidget;
class UTextBlock;
class UWrapBox;
class ALSLootBox;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSLootDropWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void SetLootSourceName(FText InSourceName);

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void SetLootItems(const TArray<FLSDropResult>& InItems);

	void SetSourceLootBox(ALSLootBox* InSourceLootBox);
	bool IsShowingLootSource(const ALSLootBox* InSourceLootBox) const;
	void RefreshLootItemsFromSource(ALSLootBox* InSourceLootBox, const TArray<FLSDropResult>& InItems);

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void ClearLootItems();

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void ClearLootSlotAt(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	bool TransferLootSlotToInventory(int32 SlotIndex);

	bool TransferLootSlotToInventorySlot(int32 SlotIndex, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex);

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	bool TransferHoveredLootSlotToInventory();

	bool TransferInventorySlotToLootSlot(ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, int32 LootSlotIndex);
	bool TransferInventorySlotToFirstEmptyLootSlot(ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex);

	void NotifyLootSlotHovered(int32 SlotIndex);
	void NotifyLootSlotUnhovered(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category="LS/UI")
	bool HasLootItems() const;

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> LootSourceNameText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UWrapBox> LootItemWrapBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	TSubclassOf<ULSItemSlotWidget> ItemSlotWidgetClass;

private:
	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/UI")
	TArray<FLSDropResult> LootItems;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/UI")
	TObjectPtr<ALSLootBox> SourceLootBox;

	int32 HoveredLootSlotIndex = INDEX_NONE;

	void SetLootSlotFromSessionItem(int32 SlotIndex, const FLSSessionItem& SessionItem);
	void RebuildLootSlots();
	ULSItemSlotWidget* CreateLootSlotWidget() const;
};
