#pragma once

#include "CoreMinimal.h"
#include "Data/LSProtocolTypes.h"
#include "UI/Common/LSLayoutRevealWidget.h"
#include "LSChipStationWidget.generated.h"

enum class ELSInventorySlotArea : uint8;

struct FLSSessionItem;
struct FLSChipProtocolTotals;

class UBorder;
class UDragDropOperation;
class UProgressBar;
class UTextBlock;
class ULSChipEquipmentSlotWidget;
class ULSConfirmDialogWidget;
class ULSInventoryDragDropOperation;
class ULSItemSlotWidget;
class ULSMinimapWidget;
class ULSProtocolWidget;
class ULSSaveSubsystem;
class ULSSkillBarWidget;
class ULSStorageButtonWidget;
class ULSSurvivalStatusWidget;
class ULSSoundDirectionIndicatorWidget;
class USlider;
class USoundBase;
class UWrapBox;

// 이 화면 세션에서 장착한 칩의 출처(영역+슬롯). 저장하지 않는 transient 상태 — 스테이션을 다시 열면(RefreshChipStation) 소멸한다.
struct FLSChipOriginRecord
{
	ELSInventorySlotArea Area = ELSInventorySlotArea{};
	int32 SlotIndex = INDEX_NONE;
};

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSChipStationWidget : public ULSLayoutRevealWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 칩 장착/설정 UI를 최신 데이터로 갱신한다.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="LS/UI|Chip")
	void RefreshChipStation();

	UFUNCTION(BlueprintCallable, Category="LS/UI|Chip")
	void SetSignalGaugePercent(float Percent);

	bool EquipChipToHardwareSlot(const ULSInventoryDragDropOperation& DragOperation, int32 EquipmentSlotIndex);
	bool DropEquippedChipToHardwareSlot(const ULSInventoryDragDropOperation& DragOperation, int32 TargetEquipmentSlotIndex);
	bool UnequipChipToWarehouse(const ULSInventoryDragDropOperation& DragOperation);
	bool SwapEquippedChipWithStoredSlot(const ULSInventoryDragDropOperation& DragOperation, ULSItemSlotWidget* TargetStoredSlotWidget, ELSInventorySlotArea TargetArea, int32 TargetSlotIndex);

	// Shift+좌클릭 빠른 조작: 칩 목록 슬롯을 첫 빈 장착 슬롯(마지막 인덱스부터 역방향)에 순서대로 장착한다.
	bool QuickEquipChipToFirstEmptyHardwareSlot(ELSInventorySlotArea SourceArea, int32 SourceSlotIndex);
	// Shift+좌클릭 빠른 조작: 장착된 칩을 해제한다(출처 기억에 따라 인벤토리 복귀 우선, 자리 없으면 창고 폴백).
	bool QuickUnequipEquippedChipToWarehouse(int32 EquipmentSlotIndex);
	bool IsChipVisibleForCurrentFilter(FName ItemRowName) const;

	// 이 스테이션이 띄운 용량 차단 알림 다이얼로그가 화면에 떠 있는지. 로비 메뉴(ULSLobbyMenuWidget)의 매 틱
	// 포커스 회수 가드가 외부 모달을 예외로 둘 때 직접 참조한다 — 보고하지 않으면 다이얼로그가 매 틱 포커스를
	// 뺏겨 확인 버튼 첫 클릭이 씹힌다.
	bool HasActiveConfirmDialog() const;
	void CloseActiveConfirmDialog();

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	void SetProtocolWidget(ULSProtocolWidget* ProtocolWidget, const TCHAR* ProtocolName, int32 CurrentLevel, int32 PreviousLevel) const;
	void RefreshChipSlots();
	void RefreshEquipmentSlots();
	void RefreshEquippedChipSummary();
	void SetEquippedChipMemoryText(int32 CurrentMemory);
	void QueueRefreshChipStation();
	// 빠른 장착 전용 경량 갱신(칩 리스트 정렬/리빌드 제외, 장착칸·요약·용량만 다음 틱에 1회로 합쳐 처리).
	void QueueRefreshEquippedChipState();
	// 장착 칩을 해제한다(드래그/Shift 해제 공용). 출처 기억이 인벤토리거나 기억이 없으면 인벤토리 복귀를 먼저
	// 시도하고, 자리가 없으면 창고로 폴백하며 알림을 띄운다. 칩 리스트는 재정렬 없이 돌아온 칩을 빈 칸에 꽂는다.
	bool UnequipChipFromSlot(int32 EquipmentSlotIndex);
	// 로비에서 해제로 인벤토리 초과 드롭이 예측되면 차단 다이얼로그를 띄우고 true. (기존 차단 정책, 최우선 검사)
	bool IsUnequipBlockedByCapacity(ULSSaveSubsystem& SaveSubsystem, int32 EquipmentSlotIndex);
	// 출처 기억 조회: 인벤토리 출신(선호=기록 슬롯) 또는 기억 없음(선호=INDEX_NONE)이면 true. 창고 출신이면 false.
	bool ShouldTryUnequipToInventory(int32 EquipmentSlotIndex, int32& OutPreferredInventoryIndex) const;
	// 인벤토리 복귀 시도. 성공 시 사운드 + 칩 리스트 갱신까지 처리한다.
	bool TryUnequipChipToInventory(ULSSaveSubsystem& SaveSubsystem, int32 EquipmentSlotIndex, int32 PreferredInventoryIndex);
	// 창고 해제 + 돌아온 칩 위치 diff 특정 + 칩 리스트 갱신(실패 시 풀 새로고침 폴백).
	bool UnequipChipToWarehouseWithListUpdate(ULSSaveSubsystem& SaveSubsystem, int32 EquipmentSlotIndex);
	// 화면 세션 출처 기억 기록/이동(장착칸끼리 이동·교환 시 레코드도 따라간다).
	void RecordChipOrigin(int32 EquipmentSlotIndex, ELSInventorySlotArea Area, int32 SlotIndex);
	void MoveChipOriginRecord(int32 FromEquipmentSlotIndex, int32 ToEquipmentSlotIndex);
	// 적재 용량 부족으로 해제가 차단됐을 때 공용 확인 다이얼로그(WBP_ConfirmDialog)를 코드로 띄운다.
	// 확인/취소/ESC 어느 쪽이든 그냥 닫힌다(정보 알림 용도). 타이틀/세팅의 알림 팝업과 동일 패턴.
	// 더블클릭·Shift·드래그드롭 제스처의 끝에서 호출되면 그 제스처의 마우스 Down이 이미 소비돼
	// 다이얼로그 첫 클릭이 씹힌다. 다음 프레임에 실제 생성(PresentCapacityBlockedDialog)을 미룬다.
	void ShowCapacityBlockedDialog(const FText& Message);
	// ShowCapacityBlockedDialog가 다음 틱에 호출하는 실제 다이얼로그 생성부.
	void PresentCapacityBlockedDialog(const FText& Message);
	UFUNCTION()
	void HandleCapacityDialogClosed();
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

	// ---- 칩 목록 필터 버튼 (WBP_SortButton 5개) ----
	// ALL은 전체를 표시하고, 나머지는 해당 프로토콜 값이 1 이상인 칩만 표시한다.
	void BindSortButtons();
	void UnbindSortButtons();
	void ApplySortButtonState() const;
	// 필터 기준 변경 + 버튼 색 갱신 + 칩 목록 재구성. 미설정(unset)은 ALL이다.
	void SetChipFilterProtocol(TOptional<ELSProtocolType> NewFilterProtocol);

	UFUNCTION()
	void HandleSortButtonAllClicked();

	UFUNCTION()
	void HandleSortButtonSurvivalClicked();

	UFUNCTION()
	void HandleSortButtonCarryingClicked();

	UFUNCTION()
	void HandleSortButtonNavigationClicked();

	UFUNCTION()
	void HandleSortButtonBattleClicked();

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

	// ---- 칩 목록 필터 버튼. WBP_ChipStation의 기존 위젯 이름을 그대로 따른다. ----
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSStorageButtonWidget> SortButton1;   // ALL — 전체 표시

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSStorageButtonWidget> SortButton2;   // 생존 프로토콜

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSStorageButtonWidget> SortButton3;   // 적재 프로토콜

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSStorageButtonWidget> SortButton4;   // 탐색 프로토콜

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Chip")
	TObjectPtr<ULSStorageButtonWidget> SortButton5;   // 전투 프로토콜

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

	// 칩 목록에서 생성하는 아이템 슬롯의 레이아웃 크기. 인벤토리·창고와 같은 크기로 맞춘다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Chip")
	FVector2D ChipItemSlotSize = FVector2D(80.f, 80.f);

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

	// 용량 부족 알림에 쓸 공용 확인 다이얼로그 클래스. WBP_ChipStation 기본값에서 WBP_ConfirmDialog로 매핑한다(에셋 매핑).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Chip")
	TSubclassOf<ULSConfirmDialogWidget> ConfirmDialogClass;

private:
	// 칩 목록 필터 기준. 미설정이면 ALL이다. 저장하지 않는 화면 상태다.
	TOptional<ELSProtocolType> ChipFilterProtocol;

	// QueueRefreshEquippedChipState가 같은 틱에 여러 번 예약되지 않도록 막는 코얼레스 가드.
	bool bPendingEquippedStateRefresh = false;

	// 이 화면 세션에서 장착한 칩의 출처(키=장착칸 인덱스). SaveGame에 저장하지 않으며 RefreshChipStation에서 리셋한다.
	TMap<int32, FLSChipOriginRecord> ChipOriginByEquipmentIndex;

	// 현재 떠 있는 알림 다이얼로그. 중복 생성 방지 + 스테이션 닫힐 때 정리에 쓴다.
	UPROPERTY(Transient)
	TObjectPtr<ULSConfirmDialogWidget> ActiveConfirmDialog;
};
