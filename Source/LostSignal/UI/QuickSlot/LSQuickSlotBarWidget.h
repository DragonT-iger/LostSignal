#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSQuickSlotBarWidget.generated.h"

class ULSQuickSlotWidget;
class ULSSaveSubsystem;

/**
 * 퀵슬롯 바. 고정 6칸(QuickSlot1~6)을 소유하며 로비/레이드 HUD 양쪽에서 재사용한다.
 * 생성 시 스스로 PlayerController에 등록하고 SaveSubsystem의 등록 변경 알림을 구독한다.
 * 개수 갱신은 PlayerController의 RefreshAllInventoryUI funnel이 RefreshAll을 호출해 처리한다.
 * WBP는 QuickSlot1~QuickSlot6을 바인딩해야 한다.
 */
UCLASS()
class LOSTSIGNAL_API ULSQuickSlotBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 모든 칸의 아이콘/개수를 다시 그린다. 인벤토리 변경 funnel과 등록 변경 알림의 공통 진입점.
	void RefreshAll();
	// 인벤토리 안에 배치된 바만 슬롯의 마우스 편집과 hit-test를 활성화한다. HUD 바는 기본 false.
	void SetInventoryInteractionEnabled(bool bEnabled);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|QuickSlot")
	TObjectPtr<ULSQuickSlotWidget> QuickSlot1;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|QuickSlot")
	TObjectPtr<ULSQuickSlotWidget> QuickSlot2;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|QuickSlot")
	TObjectPtr<ULSQuickSlotWidget> QuickSlot3;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|QuickSlot")
	TObjectPtr<ULSQuickSlotWidget> QuickSlot4;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|QuickSlot")
	TObjectPtr<ULSQuickSlotWidget> QuickSlot5;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|QuickSlot")
	TObjectPtr<ULSQuickSlotWidget> QuickSlot6;

	// true면 적재 프로토콜로 해금되지 않은 칸을 접어 표시 영역을 줄인다(HUD 바 기본값).
	// false면 레벨과 무관하게 6칸을 항상 표시한다(인벤토리 바 — 아트가 인스턴스에서 해제).
	UPROPERTY(EditAnywhere, Category="LS/UI|QuickSlot")
	bool bHideLockedSlots = true;

private:
	// 바인딩된 6칸을 인덱스 순서대로 모아 InitializeSlot(0..5)을 호출한다.
	void InitializeBar();
	// 적재 프로토콜 해금 칸 수에 맞춰 각 칸의 가시성을 적용한다(bHideLockedSlots에 따라 동작 분기).
	void ApplyProtocolVisibility();
	ULSSaveSubsystem* ResolveSaveSubsystem() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSQuickSlotWidget>> Slots;
	bool bInventoryInteractionEnabled = false;

	FDelegateHandle QuickSlotsChangedHandle;
	// 칩 장착/신호 게이지 변경(=적재 프로토콜 레벨 변동) 구독 핸들. 변경 시 RefreshAll로 가시성 재평가.
	FDelegateHandle ChipLoadoutChangedHandle;
};
