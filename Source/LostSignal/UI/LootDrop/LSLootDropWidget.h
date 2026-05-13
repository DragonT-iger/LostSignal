#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/LSDropSubsystem.h"
#include "LSLootDropWidget.generated.h"

class ULSItemSlotWidget;
class UTextBlock;
class UWrapBox;

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

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void ClearLootItems();

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void ClearLootSlotAt(int32 SlotIndex);

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> LootSourceNameText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UWrapBox> LootItemWrapBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	TSubclassOf<ULSItemSlotWidget> ItemSlotWidgetClass;

private:
	ULSItemSlotWidget* CreateLootSlotWidget() const;
};
