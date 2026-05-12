#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Session/LSSessionSubsystem.h"
#include "LSItemTooltipWidget.generated.h"

class ULSItemTooltipStatRowWidget;
class ULSItemTooltipExtraInfoRowWidget;
class UPanelWidget;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void SetItem(FName ItemRowName, int32 HoveredSlotAmount);

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> TooltipTypeText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> GradeText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UPanelWidget> StatsBox;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UPanelWidget> ExtraInfoBox;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> PriceText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	TSubclassOf<ULSItemTooltipStatRowWidget> StatRowWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	TSubclassOf<ULSItemTooltipExtraInfoRowWidget> ExtraInfoRowWidgetClass;

private:
	void ClearStats();
	void ClearExtraInfos();
	void AddStat(const FText& StatName, const FText& StatValue);
	void AddStatIfNonZero(const FText& StatName, float Value);
	void AddExtraInfo(const FText& ExtraInfoName, const FText& ExtraInfoValue);
	void SetCommonTexts(const FText& TooltipType, const FText& ItemName, int32 ItemGrade, const FText& Description, int32 ItemCost);
	void PopulateChipTooltip(FName ItemRowName);
	void PopulateWeaponTooltip(FName ItemRowName);
	void PopulateArmorTooltip(FName ItemRowName);
	void PopulateItemTooltip(FName ItemRowName, int32 HoveredSlotAmount);

	static FText GetGradeText(int32 ItemGrade);
	static FText GetEquipmentDisplayText(const FString& EquipmentName);
	static FText GetChipUIDisplayText(const FString& ChipUIName);
	static FText NormalizeDescriptionText(const FText& Description);
	static FText FormatNumber(float Value);
	static int32 CountItems(const TArray<FLSSessionItem>& Items, FName ItemRowName);
};
