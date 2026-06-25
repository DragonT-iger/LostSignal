#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/SoftObjectPath.h"
#include "LSMonsterArchetypeRow.generated.h"

/** 몬스터 전투 성향. 테이블 Monster_Combat_Type Enum과 일치. */
UENUM(BlueprintType)
enum class ELSMonsterCombatType : uint8
{
	Melee,
	Ranged
};

/** 몬스터 등급. 테이블 Monster_Rank Enum과 일치. */
UENUM(BlueprintType)
enum class ELSMonsterRank : uint8
{
	Normal,
	Boss
};

/** DataTable row matching the monster planning CSV. */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSMonsterArchetypeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI")
	FText Monster_Name_KR;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI")
	FSoftObjectPath Monster_Resource_Path;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI")
	ELSMonsterRank Monster_Rank = ELSMonsterRank::Normal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat")
	ELSMonsterCombatType Monster_Combat_Type = ELSMonsterCombatType::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", meta=(ClampMin="0.0"))
	float Monster_HP = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", meta=(ClampMin="0.0"))
	float Monster_ATK = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", meta=(ClampMin="0.0"))
	float Monster_DEF = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", meta=(ClampMin="0"))
	int32 Monster_Guard = 1;

	// 공격을 맞을 때 방어 관통 수치를 감소시키는 저항. (계산식 적용은 데미지 파이프라인 후속)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", meta=(ClampMin="0.0"))
	float Monster_ArmorPen_Resist = 0.0f;

	// 공격을 맞을 때 치명타 확률 수치를 감소시키는 저항. (계산식 적용은 데미지 파이프라인 후속)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", meta=(ClampMin="0.0"))
	float Monster_Crit_Resist = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float Sight_Radius = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float Hearing_Radius = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float Patrol_Speed = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float Chase_Speed = 1.0f;

	// 사용하는 액션 그룹. DT_MonsterAction(FLSMonsterActionRow) row 이름 목록.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat")
	TArray<FName> Action_Group;
};
