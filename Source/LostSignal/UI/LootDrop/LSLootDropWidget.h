#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/LSDropSubsystem.h"
#include "Session/LSSessionSubsystem.h"
#include "LSLootDropWidget.generated.h"

class ULSItemSlotWidget;
class UTextBlock;
class USoundBase;
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
	bool DropLootSlot(int32 FromLootSlotIndex, int32 ToLootSlotIndex);

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

	// 루트드랍 화면에서 생성하는 아이템 슬롯의 레이아웃 크기. 인벤토리와 같은 크기로 맞춘다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI")
	FVector2D LootItemSlotSize = FVector2D(80.f, 80.f);

	// 등급명(Supply/Standard/Precision/Tuning/Prototype/Masterpiece) → 슬롯 공개 순간 재생할 사운드. WBP에서 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	TMap<FName, TObjectPtr<USoundBase>> GradeRevealSounds;

private:
	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/UI")
	TArray<FLSDropResult> LootItems;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/UI")
	TObjectPtr<ALSLootBox> SourceLootBox;

	int32 HoveredLootSlotIndex = INDEX_NONE;

	void SetLootSlotFromSessionItem(int32 SlotIndex, const FLSSessionItem& SessionItem);
	void RebuildLootSlots();
	void PlayRevealSoundForNewItems(int32 FirstNewItemIndex);
};
