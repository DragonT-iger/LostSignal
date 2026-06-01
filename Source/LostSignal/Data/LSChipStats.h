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
};

namespace LSChipStats
{
	// 새 칩 획득 시 부여할 비-0 시드를 생성한다. (서버 권위 드랍 생성 지점에서 호출)
	LOSTSIGNAL_API int32 RollNewChipSeed();

	// 칩의 확정 전투 스탯을 결정론적으로 산출한다.
	// (등급은 RowName에서 파싱, 스탯 개수는 ChipRow.Item_Chip_Status_Count, 값 범위는 ChipStat 등급 행)
	// 같은 (RowName, StatSeed) 입력은 항상 같은 결과. StatSeed==0이면 RowName 해시를 폴백 시드로 사용.
	LOSTSIGNAL_API TArray<FLSChipResolvedStat> ResolveChipStats(FName ChipRowName, int32 StatSeed);

	// 스탯 키 → 표시용 라벨 (예: "Chip_Attack" → "공격력").
	LOSTSIGNAL_API FText GetChipStatLabel(FName StatKey);
}
