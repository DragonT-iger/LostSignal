#pragma once

#include "CoreMinimal.h"
#include "Combat/LSHitboxTypes.h"
#include "Engine/DataTable.h"
#include "UObject/SoftObjectPath.h"
#include "LSMonsterActionRow.generated.h"

/** 액션 적용 대상. 테이블 Action_Target Enum과 일치. */
UENUM(BlueprintType)
enum class ELSActionTarget : uint8
{
	Self,
	Area
};

/** DT_MonsterAction CSV row와 1:1로 맞춘 몬스터 액션 정의. 첫 컬럼 Name은 row key(중복 선언하지 않음). */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSMonsterActionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat")
	FText Action_Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float Action_Range_Min = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float Action_Range_Max = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float Action_Cooldown = 0.0f;

	// 액션의 대미지 계수.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float Action_Multiplier = 1.0f;

	// 액션 중 피격 반응을 구별하는 강인도.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0"))
	int32 Action_Guard = 1;

	// 적의 행동을 무너트리는 붕괴력.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0"))
	int32 Action_Impact = 1;

	// 플레이어 피격 시 증가하는 침식 수치.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float Erosion_Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat")
	ELSActionTarget Action_Target = ELSActionTarget::Area;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat")
	ELSHitboxShape Hitbox_Shape = ELSHitboxShape::Box;

	// 히트박스 1번 크기(원 반경, 사각 길이 등).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float Hitbox_X = 0.0f;

	// 히트박스 2번 크기(부채꼴 각도, 사각 너비 등).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float Hitbox_Y = 0.0f;

	// 히트박스 3번 크기(판정 높이).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float Hitbox_Z = 0.0f;

	// 이동하는 공격일 때 이동 거리.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float Dash_Distance = 0.0f;

	// 이동에 소요되는 시간.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float Duration = 0.0f;

	// 액션과 연동되는 애니메이션 몽타주 에셋 경로.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat")
	FSoftObjectPath Action_Ani;
};
