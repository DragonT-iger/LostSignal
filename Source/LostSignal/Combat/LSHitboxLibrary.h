#pragma once

#include "CoreMinimal.h"
#include "Combat/LSHitboxTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LSHitboxLibrary.generated.h"

enum class ELSCharacterSkillRangeShape : uint8;

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

	/**
	 * DataTable Row 규약(Range_Shape/Range_X/Range_Y)으로 shape 판정.
	 * Circle: 반경=RangeX / Cone: 반경=RangeX, 각도=RangeY / Box: 길이=RangeX, 폭=RangeY. None은 Circle로 처리.
	 */
	UFUNCTION(BlueprintPure, Category="LS/Combat|Hitbox")
	static bool IsTargetInsideSkillRange(
		const FVector& SourceLocation,
		const FVector& AimDirection2D,
		const FVector& TargetLocation,
		ELSCharacterSkillRangeShape Shape,
		float RangeX,
		float RangeY);

	/** Row 규약 범위를 전부 덮는 브로드페이즈 SphereOverlap 반경. Box는 sqrt(길이² + (폭/2)²), 그 외 RangeX. */
	UFUNCTION(BlueprintPure, Category="LS/Combat|Hitbox")
	static float GetSkillRangeQueryRadius(ELSCharacterSkillRangeShape Shape, float RangeX, float RangeY);
};
