#pragma once

#include "CoreMinimal.h"
#include "LSChipStats.generated.h"

struct FLSSessionItem;

// 칩 인스턴스에서 확정된 전투 스탯 하나.
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSChipResolvedStat
{
	GENERATED_BODY()

	// 스탯 키 (예: "Chip_Attack"). 표시 라벨은 GetChipStatLabel로 변환.
	UPROPERTY(BlueprintReadOnly, Category="LS/Chip")
	FName StatKey;

	// 시드로 롤링된 확정 값.
	UPROPERTY(BlueprintReadOnly, Category="LS/Chip")
	int32 Value = 0;

	bool operator==(const FLSChipResolvedStat& Other) const
	{
		return StatKey == Other.StatKey && Value == Other.Value;
	}
};

struct LOSTSIGNAL_API FLSChipProtocolTotals
{
	int32 Survival = 0;
	int32 Carrying = 0;
	int32 Battle = 0;
	int32 Navigation = 0;
};

namespace LSChipStats
{
	// 칩 획득 시점에 전투 스탯을 굴려 스냅샷 배열로 반환한다.
	// (등급은 RowName에서 파싱, 스탯 개수는 ChipRow.Item_Chip_Status_Count, 값 범위는 ChipStat 등급 행)
	LOSTSIGNAL_API TArray<FLSChipResolvedStat> RollChipStats(FName ChipRowName);

	LOSTSIGNAL_API int32 ResolveInactiveSignalSlotCount(float SignalGaugePercent);
	// 레이드 신호 드레인 한 주기에서 다음으로 비활성화할 장착 칩의 게이지 값을 계산한다.
	// 빈 슬롯과 마지막 칩 뒤의 빈 구간은 건너뛰며, 남은 장착 칩이 없으면 false와 0을 반환한다.
	LOSTSIGNAL_API bool TryResolveNextSignalGaugePercent(const TArray<FLSSessionItem>& Items, float CurrentPercent, float& OutNextPercent);
	LOSTSIGNAL_API TArray<FLSSessionItem> BuildSignalActiveEquipmentItems(const TArray<FLSSessionItem>& Items, int32 InactiveSlotCount);
	LOSTSIGNAL_API TMap<FName, int32> AggregateChipStatTotals(const TArray<FLSSessionItem>& Items);

	// 게임플레이에 실제 적용할 전투 스탯 합산. 활성 칩 100% + 비활성 칩 50% 규칙.
	// (UI 표시 로직과 동일한 단일 출처: 전체 합 - 비활성 합의 절반 반올림)
	LOSTSIGNAL_API TMap<FName, int32> ComputeEffectiveChipStatTotals(const TArray<FLSSessionItem>& Items, int32 InactiveSlotCount);
	LOSTSIGNAL_API FLSChipProtocolTotals AggregateChipProtocolTotals(const TArray<FLSSessionItem>& Items, const UObject* LogContext);

	// 스탯 키 → 표시용 라벨 (예: "Chip_Attack" → "공격력").
	LOSTSIGNAL_API FText GetChipStatLabel(FName StatKey);
}
