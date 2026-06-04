#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSChipStationWidget.generated.h"

enum class ELSInventorySlotArea : uint8;

class UBorder;
class UDragDropOperation;
class UProgressBar;
class UTextBlock;
class ULSChipStatWidget;
class ULSChipEquipmentSlotWidget;
class ULSInventoryDragDropOperation;
class ULSItemSlotWidget;
class ULSProtocolWidget;
class USlider;
class UWrapBox;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSChipStationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 칩 장착/설정 UI를 최신 데이터로 갱신한다.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LS/UI|Chip")
	void RefreshChipStation();

	// 스탯 키(예: "Chip_Attack")에 해당하는 ChipStat 칸을 갱신한다.
	// 이름 라벨은 키에서 자동 변환(GetChipStatLabel)되며, 값/신호유실은 임의값/파라미터.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Chip")
	void SetChipStat(FName StatKey, int32 StatValue, int32 SignalLoss);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Chip")
	void SetSignalGaugePercent(float Percent);

	bool EquipChipToHardwareSlot(const ULSInventoryDragDropOperation& DragOperation, int32 EquipmentSlotIndex);
	bool DropEquippedChipToHardwareSlot(const ULSInventoryDragDropOperation& DragOperation, int32 TargetEquipmentSlotIndex);
	bool UnequipChipToWarehouse(const ULSInventoryDragDropOperation& DragOperation);
	bool SwapEquippedChipWithStoredSlot(const ULSInventoryDragDropOperation& DragOperation, ELSInventorySlotArea TargetArea, int32 TargetSlotIndex);

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	// 키 → ChipStat 칸 매핑. 8개 스탯.
	ULSChipStatWidget* GetStatWidget(FName StatKey) const;
	void SetProtocolWidget(ULSProtocolWidget* ProtocolWidget, const TCHAR* ProtocolName, int32 Level, int32 SynergyStage) const;
	void RefreshChipSlots();
	void RefreshEquipmentSlots();
	void RefreshEquippedChipSummary();
	void SetEquippedChipMemoryText(int32 CurrentMemory);
	void QueueRefreshChipStation();
	ULSItemSlotWidget* CreateChipSlotWidget() const;
	void InitializeEquipmentSlots();
	bool IsPointerInsideChipSlotBorder(FVector2D ScreenPosition) const;
	float GetSignalGaugePercent() const;
	int32 GetInactiveSignalSlotCount() const;
	void SynchronizeSignalGauge(float Percent);

	UFUNCTION()
	void HandleSignalSliderValueChanged(float Value);

	// ---- 8개 ChipStat 칸 (WBP_ChipStat) ----
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSChipStatWidget> ChipStat_Attack;              // 공격력

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSChipStatWidget> ChipStat_CriticalRate;        // 치명타 확률

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSChipStatWidget> ChipStat_CriticalDamage;      // 치명타 피해

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSChipStatWidget> ChipStat_DefensePenetration;  // 방어 관통

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSChipStatWidget> ChipStat_Health;             // 체력

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSChipStatWidget> ChipStat_Defense;            // 방어력

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSChipStatWidget> ChipStat_AttackSpeed;        // 공격 속도

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSChipStatWidget> ChipStat_Skill_Haste;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSChipStatWidget> ChipStat_Recovery;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSChipStatWidget> ChipStat_MoveSpeed;          // 이동 속도
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSProtocolWidget> Protocol_Survival;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSProtocolWidget> Protocol_Carrying;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSProtocolWidget> Protocol_Battle;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSProtocolWidget> Protocol_Navigation;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<UWrapBox> ChipSlotWrapBox;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<UBorder> ChipSlotBorder;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<UProgressBar> SignalProgressBar;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<USlider> SignalSlider;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<UTextBlock> MemoryText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Chip", meta=(ClampMin="0"))
	int32 MaxChipMemory = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Chip")
	TSubclassOf<ULSItemSlotWidget> ItemSlotWidgetClass;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSChipEquipmentSlotWidget> EquipmentSlot_0;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSChipEquipmentSlotWidget> EquipmentSlot_1;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSChipEquipmentSlotWidget> EquipmentSlot_2;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSChipEquipmentSlotWidget> EquipmentSlot_3;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSChipEquipmentSlotWidget> EquipmentSlot_4;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSChipEquipmentSlotWidget> EquipmentSlot_5;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSChipEquipmentSlotWidget> EquipmentSlot_6;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSChipEquipmentSlotWidget> EquipmentSlot_7;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSChipEquipmentSlotWidget> EquipmentSlot_8;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSChipEquipmentSlotWidget> EquipmentSlot_9;
};
