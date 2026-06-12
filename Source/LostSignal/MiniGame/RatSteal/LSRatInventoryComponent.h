#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MiniGame/RatSteal/LSRatTypes.h"
#include "LSRatInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLSRatOnInventoryChanged);

/**
 * 3슬롯 인벤토리 (16_Mechanic_Inventory).
 * 같은 종류는 같은 슬롯에 누적(종류당 1슬롯), 크기→카운트(S+1/M+6/L+9).
 * 감속은 적재 카운트의 지수식(SpeedMultiplier)으로만 적용 — 원작 GetWeight 경로는 미사용(죽은 코드)이라 이식하지 않음.
 */
UCLASS(ClassGroup = (LS), meta = (BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSRatInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSRatInventoryComponent();

	/** 훔친 작물 적재. 빈 슬롯 또는 같은 종류 슬롯에 누적 (원작 AddCrop 순회 순서 유지) */
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void AddCrop(ELSRatCropType Type, ELSRatCropSize Size);

	/** 현재 슬롯 전환 0→1→2→0 (X키) */
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void ChangeSlot();

	/** 현재 슬롯 카운트 1 감소 (C키, 버리기 → 즉시 가속) */
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	bool ThrowItem();

	/** 제출: 전 슬롯 데이터 반환 후 비움 (원작 SubMissonItem) */
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	TArray<FLSRatSlotData> SubmitAll();

	/**
	 * 감속 배수 (원작 GetSpeedMultiplier):
	 * (1.01)^감자카운트 × (1.02)^가지카운트 × (1.03)^호박카운트
	 */
	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	float GetSpeedMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	const TArray<FLSRatSlotData>& GetSlots() const { return Slots; }

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	int32 GetCurrentSlotIndex() const { return CurrentSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	ELSRatCropType GetSlotCropType(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	ELSRatCropType GetCurrentSlotCropType() const { return GetSlotCropType(CurrentSlotIndex); }

	UPROPERTY(BlueprintAssignable, Category = "LS/RatSteal")
	FLSRatOnInventoryChanged OnInventoryChanged;

private:
	/** 1카운트당 감속률 (원작: 감자0.01/가지0.02/호박0.03, 복리) */
	void ResetFixedSlots();
	int32 GetSlotIndexForType(ELSRatCropType Type) const;

	UPROPERTY(EditAnywhere, Category = "LS/RatSteal|Balance")
	float PotatoBonus = 0.01f;

	UPROPERTY(EditAnywhere, Category = "LS/RatSteal|Balance")
	float EggplantBonus = 0.02f;

	UPROPERTY(EditAnywhere, Category = "LS/RatSteal|Balance")
	float PumpkinBonus = 0.03f;

	UPROPERTY(EditAnywhere, Category = "LS/RatSteal|Balance", meta = (ClampMin = 1))
	int32 SlotNum = 3;

	UPROPERTY(VisibleInstanceOnly, Category = "LS/RatSteal")
	TArray<FLSRatSlotData> Slots;

	int32 CurrentSlotIndex = 0;
};
