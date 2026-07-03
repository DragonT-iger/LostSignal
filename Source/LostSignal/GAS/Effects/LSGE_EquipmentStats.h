#pragma once

#include "GameplayEffect.h"
#include "LSGE_EquipmentStats.generated.h"

/**
 * 장착 무기/방어구 합산 전투 스탯을 캐릭터 어트리뷰트에 적용하는 무한 지속 GameplayEffect.
 * 각 모디파이어 크기는 SetByCaller(LS.Data.Equip.*)로 런타임에 채운다.
 * 적용/갱신 로직은 ULSEquipmentStatComponent 참조.
 */
UCLASS()
class LOSTSIGNAL_API ULSGE_EquipmentStats : public UGameplayEffect
{
	GENERATED_BODY()

public:
	ULSGE_EquipmentStats(const FObjectInitializer& ObjectInitializer);
};
