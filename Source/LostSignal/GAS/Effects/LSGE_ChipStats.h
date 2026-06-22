#pragma once

#include "GameplayEffect.h"
#include "LSGE_ChipStats.generated.h"

/**
 * 칩 장착 합산 전투 스탯을 캐릭터 어트리뷰트에 적용하는 무한 지속 GameplayEffect.
 * 각 모디파이어 크기는 SetByCaller(LS.Data.Chip.*)로 런타임에 채운다.
 * 적용/갱신 로직은 ULSChipStatComponent 참조.
 */
UCLASS()
class LOSTSIGNAL_API ULSGE_ChipStats : public UGameplayEffect
{
	GENERATED_BODY()

public:
	ULSGE_ChipStats(const FObjectInitializer& ObjectInitializer);
};
