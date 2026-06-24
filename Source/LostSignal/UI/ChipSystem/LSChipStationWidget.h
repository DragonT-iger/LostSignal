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

	// Shift+좌클릭으로 칩 1개를 장착한 직후 호출한다. 버튼을 계속 누르고 있으면 InitialDelay 뒤부터
	// Interval 간격으로 칩을 우수수 자동 장착한다(리스트 정렬 상단부터). 칩 스테이션 리스트는 정렬 리빌드로
	// 커서 위치에 칩이 계속 들어와 MouseMove 기반 반복으로는 1개만 장착하기 어려워, 시간 기반 반복으로 분리했다.
	void StartQuickEquipAutoRepeat();
	void StopQuickEquipAutoRepeat();

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
	void HandleCarryingSlotCapacityChanged();
	void InitializeEquipmentSlots();
	void SetPreviewMinimapNavigationLevels(int32 CurrentNavigationProtocol, int32 PreviousNavigationProtocol);
	void SetPreviewSurvivalStatus(int32 CurrentSurvivalProtocol, int32 PreviousSurvivalProtocol);
	void SetPreviewSignalChip(const TArray<FLSSessionItem>& EquipmentItems, float SignalPercent);
	void SetPreviewBattleProtocol(int32 CurrentBattleProtocol, int32 PreviousBattleProtocol);
	// 프로토콜 디버그 오버라이드가 켜져 있으면 그 값을, 아니면 장착 칩 합산값(현재=활성칩, 이전=전체칩)을 돌려준다.
	void ResolveProtocolPreviewLevels(ELSProtocolType ProtocolType, const FLSChipProtocolTotals& ActiveTotals, const FLSChipProtocolTotals& AllTotals, int32& OutCurrentLevel, int32& OutPreviousLevel) const;
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

	// 칩 리스트→장착칸 빠른이동 자동반복: 첫 1개 장착 후 이 시간(초)이 지나야 다음 칩이 들어간다.
	// 이 안에 버튼을 떼면 1개만 장착된다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Chip", meta=(ClampMin="0"))
	float QuickEquipAutoRepeatInitialDelay = 0.28f;

	// 자동반복 시작 후 칩이 우수수 장착되는 간격(초).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Chip", meta=(ClampMin="0.01"))
	float QuickEquipAutoRepeatInterval = 0.05f;

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

private:
	// 자동반복 1틱: Shift+좌클릭이 유지되는 동안 정렬 상단 칩을 장착하고, 입력이 풀리거나
	// 더 장착할 수 없으면 멈춘다.
	void TickQuickEquipAutoRepeat();
	// 인벤토리+창고를 합쳐 리스트와 동일하게 정렬한 뒤, 상단 칩을 첫 빈 장착칸에 장착한다.
	bool QuickEquipFirstAvailableChip();
	// 자동반복 유지 조건: Shift + 마우스 좌버튼이 모두 눌린 상태인지 전역 입력으로 확인한다.
	bool IsQuickEquipAutoRepeatInputHeld() const;

	FTimerHandle QuickEquipAutoRepeatTimerHandle;
};
