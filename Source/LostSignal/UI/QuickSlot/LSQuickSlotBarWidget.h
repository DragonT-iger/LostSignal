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

private:
	// 바인딩된 6칸을 인덱스 순서대로 모아 InitializeSlot(0..5)을 호출한다.
	void InitializeBar();
	ULSSaveSubsystem* ResolveSaveSubsystem() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSQuickSlotWidget>> Slots;

	FDelegateHandle QuickSlotsChangedHandle;
};
