#pragma once

#include "CoreMinimal.h"
#include "LSChipStats.generated.h"

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

namespace LSChipStats
{
	// 칩 획득 시점에 전투 스탯을 굴려 스냅샷 배열로 반환한다.
	// (등급은 RowName에서 파싱, 스탯 개수는 ChipRow.Item_Chip_Status_Count, 값 범위는 ChipStat 등급 행)
	LOSTSIGNAL_API TArray<FLSChipResolvedStat> RollChipStats(FName ChipRowName);

	// 스탯 키 → 표시용 라벨 (예: "Chip_Attack" → "공격력").
	LOSTSIGNAL_API FText GetChipStatLabel(FName StatKey);
}
