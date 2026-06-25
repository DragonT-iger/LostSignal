#pragma once

#include "CoreMinimal.h"
#include "Combat/LSHitboxTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LSHitboxLibrary.generated.h"

/** 공격 범위(Circle/Cone/Box) 정밀 판정 공용 유틸. 플레이어 스킬·몬스터 액션이 공유한다. */
UCLASS()
class LOSTSIGNAL_API ULSHitboxLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 2D(XY) 평면에서 타겟이 공격 히트박스 안에 있는지 판정한다.
	 * - Circle: 거리 <= Radius
	 * - Cone:   거리 <= Radius 이고 조준 방향과의 각도 <= ConeAngleDegrees/2
	 * - Box:    전방 0~Length, 좌우 |.| <= Width/2
	 * @param AimDirection2D 정규화된 전방 방향(XY). 영벡터면 false.
	 */
	UFUNCTION(BlueprintPure, Category="LS/Combat|Hitbox")
	static bool IsTargetInsideHitbox(
		const FVector& SourceLocation,
		const FVector& AimDirection2D,
		const FVector& TargetLocation,
		ELSHitboxShape Shape,
		float Radius,
		float Length,
		float Width,
		float ConeAngleDegrees);
};
