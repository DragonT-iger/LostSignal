#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/LSProtocolTypes.h"
#include "LSChipStationWidget.generated.h"

enum class ELSInventorySlotArea : uint8;

struct FLSSessionItem;
struct FLSChipProtocolTotals;

class UBorder;
class UDragDropOperation;
class UProgressBar;
class UTextBlock;
class ULSChipStatWidget;
class ULSChipEquipmentSlotWidget;
class ULSInventoryDragDropOperation;
class ULSItemSlotWidget;
class ULSMinimapWidget;
class ULSProtocolWidget;
class ULSSkillBarWidget;
class ULSSurvivalStatusWidget;
class ULSSoundDirectionIndicatorWidget;
class USlider;
class USoundBase;
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

	// Shift+좌클릭 빠른 조작: 칩 목록 슬롯을 첫 빈 장착 슬롯(index 0부터)에 순서대로 장착한다.
	bool QuickEquipChipToFirstEmptyHardwareSlot(ELSInventorySlotArea SourceArea, int32 SourceSlotIndex);
	// Shift+좌클릭 빠른 조작: 장착된 칩을 창고로 해제한다.
	bool QuickUnequipEquippedChipToWarehouse(int32 EquipmentSlotIndex);

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	// 키 → ChipStat 칸 매핑. 8개 스탯.
	ULSChipStatWidget* GetStatWidget(FName StatKey) const;
	void SetProtocolWidget(ULSProtocolWidget* ProtocolWidget, const TCHAR* ProtocolName, ELSProtocolType ProtocolType, int32 CurrentLevel, int32 PreviousLevel) const;
	void RefreshChipSlots();
	void RefreshEquipmentSlots();
	void RefreshEquippedChipSummary();
	void SetEquippedChipMemoryText(int32 CurrentMemory);
	void QueueRefreshChipStation();
	// 빠른 장착 전용 경량 갱신(칩 리스트 정렬/리빌드 제외, 장착칸·요약·용량만 다음 틱에 1회로 합쳐 처리).
	void QueueRefreshEquippedChipState();
	// 장착 칩을 창고로 해제하고, 칩 리스트는 재정렬 없이 돌아온 칩을 빈 칸(없으면 맨 뒤)에 꽂는다. 드래그/Shift 해제 공용.
	bool UnequipChipFromSlotToWarehouse(int32 EquipmentSlotIndex);
	// 칩 한 개를 칩 리스트의 첫 빈 슬롯(hole)에 넣거나, 없으면 맨 뒤에 새 슬롯으로 추가한다(정렬/리빌드 없음).
	void InsertChipListSlot(const FLSSessionItem& Chip, ELSInventorySlotArea SourceArea, int32 SourceSlotIndex);
	void HandleCarryingSlotCapacityChanged();
	void InitializeEquipmentSlots();
	void SetPreviewMinimapNavigationLevels(int32 CurrentNavigationProtocol, int32 PreviousNavigationProtocol);
	void SetPreviewSurvivalStatus(int32 CurrentSurvivalProtocol, int32 PreviousSurvivalProtocol);
	void SetPreviewSignalChip(const TArray<FLSSessionItem>& EquipmentItems, float SignalPercent);
	void SetPreviewBattleProtocol(int32 CurrentBattleProtocol, int32 PreviousBattleProtocol);
	// 프로토콜 디버그 오버라이드가 켜져 있으면 그 값을, 아니면 장착 칩 합산값(현재=활성칩, 이전=전체칩)을 돌려준다.
	void ResolveProtocolPreviewLevels(ELSProtocolType ProtocolType, const FLSChipProtocolTotals& ActiveTotals, const FLSChipProtocolTotals& AllTotals, int32& OutCurrentLevel, int32& OutPreviousLevel) const;
	bool IsPointerInsideChipSlotBorder(FVector2D ScreenPosition) const;
	// 장착/해제 사운드 재생. 미할당이면 경고 로그만 남긴다.
	void PlayChipSound(USoundBase* Sound, const TCHAR* SoundPropertyName) const;
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

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Minimap")
	TObjectPtr<ULSMinimapWidget> Minimap;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Survival")
	TObjectPtr<ULSSurvivalStatusWidget> SurvivalStatus;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<ULSSkillBarWidget> SkillBar;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "LS/UI|Survival")
	TObjectPtr<ULSSoundDirectionIndicatorWidget> SoundIndicator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Chip", meta=(ClampMin="0"))
	int32 MaxChipMemory = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Chip")
	TSubclassOf<ULSItemSlotWidget> ItemSlotWidgetClass;

	// 칩 장착 성공 시 재생할 사운드. WBP에서 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Chip")
	TObjectPtr<USoundBase> ChipEquipSound;

	// 칩 장착 해제(창고 반환) 성공 시 재생할 사운드. WBP에서 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Chip")
	TObjectPtr<USoundBase> ChipUnequipSound;

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

private:
	// QueueRefreshEquippedChipState가 같은 틱에 여러 번 예약되지 않도록 막는 코얼레스 가드.
	bool bPendingEquippedStateRefresh = false;
};
