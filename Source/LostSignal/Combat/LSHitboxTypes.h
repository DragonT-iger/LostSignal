#pragma once

#include "CoreMinimal.h"
#include "LSHitboxTypes.generated.h"

/** 공격 범위 판정/표시에 공용으로 쓰는 히트박스 형태. 플레이어 스킬·몬스터 액션 공용. */
UENUM(BlueprintType)
enum class ELSHitboxShape : uint8
{
	Circle,
	Cone,
	Box
};
