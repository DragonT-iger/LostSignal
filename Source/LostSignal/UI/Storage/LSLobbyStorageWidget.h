#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Session/LSSessionSubsystem.h"
#include "LSLobbyStorageWidget.generated.h"

class ULSItemSlotWidget;
class ULSStorageButtonWidget;
class ULSSaveSubsystem;
class UTextBlock;
class UWrapBox;

UENUM(BlueprintType)
enum class ELSStorageFilter : uint8
{
	All,
	Weapon,
	Armor,
	Consumable,
	Misc,
	Chip,
};

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSLobbyStorageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="LS/UI|Storage")
	void SetFilter(ELSStorageFilter NewFilter);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Storage")
	void RefreshStorage();

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<UWrapBox> StorageSlotWrapBox;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<UTextBlock> StorageCountText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<ULSStorageButtonWidget> SortButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<ULSStorageButtonWidget> AllTabButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<ULSStorageButtonWidget> WeaponTabButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<ULSStorageButtonWidget> ArmorTabButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<ULSStorageButtonWidget> ConsumableTabButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<ULSStorageButtonWidget> MiscTabButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Storage")
	TObjectPtr<ULSStorageButtonWidget> ChipTabButton;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Storage")
	TSubclassOf<ULSItemSlotWidget> ItemSlotWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Storage", meta=(ClampMin="0"))
	int32 MaxStorageSlotCount = 100;

private:
	ELSStorageFilter CurrentFilter = ELSStorageFilter::All;

	UFUNCTION()
	void HandleSortButtonClicked();

	UFUNCTION()
	void HandleAllTabButtonClicked();

	UFUNCTION()
	void HandleWeaponTabButtonClicked();

	UFUNCTION()
	void HandleArmorTabButtonClicked();

	UFUNCTION()
	void HandleConsumableTabButtonClicked();

	UFUNCTION()
	void HandleMiscTabButtonClicked();

	UFUNCTION()
	void HandleChipTabButtonClicked();

	void BindStorageButtons();
	void UnbindStorageButtons();
	void UpdateStorageCountText(const TArray<FLSSessionItem>& StashItems) const;
	void ApplyFilterButtonState() const;
	void BuildFilteredItems(const TArray<FLSSessionItem>& StashItems, TArray<FLSSessionItem>& OutItems) const;
	bool DoesItemMatchCurrentFilter(FName ItemRowName) const;
	bool IsConsumableItem(FName ItemRowName) const;
	ULSSaveSubsystem* GetSaveSubsystem() const;
	static bool IsFilledStorageSlot(const FLSSessionItem& Item);
};
