#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSSkillNodeRow.generated.h"

// 노드가 어느 테이블에서 왔는지. DataTable 컬럼이 아니라 통합 인덱스가 채우는 값이다.
// 기획 시트가 종류별로 나뉘어 있으므로 테이블 자체가 종류를 결정한다.
UENUM(BlueprintType)
enum class ELSSkillNodeKind : uint8
{
	None,
	Core,
	MainStat,
	SubStat,
	SkillEnhance,
	SkillEvolve
};

// 노드가 값을 바꾸는 방식.
// Add      = 대상에 Value 를 더한다.
// Multiply = 대상에 (1 + Value) 를 곱한다. Value 가 음수면 감소(쿨타임 등).
UENUM(BlueprintType)
enum class ELSSkillNodeOperation : uint8
{
	None,
	Add,
	Multiply
};

/**
 * 코어 노드. 캐릭터별 토폴로지 시작점이며 시스템 활성 시 자동으로 활성된다.
 *
 * 선행 컬럼이 없다. 코어는 "선행이 없는 노드"라는 것이 정의이고 DT_CoreNodes.csv 에도 그 컬럼이 없다.
 * 통합 인덱스가 코어의 선행을 빈 값으로 채운다.
 */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSSkillNodeCoreRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	int32 Character_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	FText Node_Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	int32 Ring = 0;

	// 기획 시트의 배치 라벨(CORE / R1-01 / R2-M03 …). 각도 인덱스가 아니므로 좌표 계산에 쓰지 않는다.
	// 종류별로 번호가 1부터 다시 시작하기 때문이다. 로그·디버그 표시용.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	FName Slot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Cost")
	FName Chip_Grade;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Cost")
	int32 Required_Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Cost")
	int32 Coin_Cost = 0;
};

/**
 * 스탯 노드. 메인스탯(DT_MainStatNodes)과 서브스탯(DT_SubStatNodes)이 이 구조체를 공유한다.
 * 두 테이블의 컬럼 구성이 완전히 같고 해석·적용 경로도 같다 — 차이는 수치 크기·비용·UI 도형뿐이다.
 */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSSkillNodeStatRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	int32 Character_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	FText Node_Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	int32 Ring = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	FName Slot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	FName Prerequisite_1;

	// 두 번째 선행. 둘 중 하나만 활성이면 충족한다(ANY). 둘 다 요구하는 ALL 이 아니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	FName Prerequisite_2;

	// 변경 대상 스탯 토큰. 매핑은 LSSkillNodeIndex 의 알려진 토큰 목록이 단일 출처다.
	// 토큰이 FLSCharacterStatRow 의 필드명과 항상 같지는 않다(예: Char_HP_Recovery).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Stat")
	FName Stat_Field;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Stat")
	ELSSkillNodeOperation Operation = ELSSkillNodeOperation::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Stat")
	float Value = 0.0f;

	// 정수 / % / %p. UI 표시 전용이며 계산에 쓰지 않는다(계산은 Operation 이 결정한다).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Stat")
	FName Unit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Cost")
	FName Chip_Grade;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Cost")
	int32 Required_Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Cost")
	int32 Coin_Cost = 0;
};

/** 스킬 강화 노드. 기본 스킬 row 의 특정 필드 값을 비율로 가로챈다(row 자체를 바꾸지 않는다). */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSSkillNodeEnhanceRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	int32 Character_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	FText Node_Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	int32 Ring = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	FName Slot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	FName Prerequisite_1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	FName Prerequisite_2;

	// 강화 대상 기본 스킬. DT_ActiveSkill 의 Skill_ID 와 같은 키다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Enhance")
	int32 Skill_ID = 0;

	// 강화할 스킬 row 필드 — Skill_Multiplier / Range_X / Skill_Cooldown.
	// 알려진 목록은 LSSkillNodeIndex 가 소유한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Enhance")
	FName Parameter_Field;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Enhance")
	ELSSkillNodeOperation Operation = ELSSkillNodeOperation::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Enhance")
	float Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Enhance")
	FName Unit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Cost")
	FName Chip_Grade;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Cost")
	int32 Required_Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Cost")
	int32 Coin_Cost = 0;
};

/**
 * 스킬 진화 노드. 기본 스킬을 다른 스킬로 교체한다.
 * 같은 Base_Skill_ID 를 가진 진화 노드끼리는 하나만 활성 가능하다 — Base_Skill_ID 가 배타 그룹을 겸하므로
 * 시트의 Exclusive_Group 컬럼은 임포트하지 않는다.
 */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSSkillNodeEvolveRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	int32 Character_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	FText Node_Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	int32 Ring = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	FName Slot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	FName Prerequisite_1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode")
	FName Prerequisite_2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Evolve")
	int32 Base_Skill_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Evolve")
	int32 Evolution_Skill_ID = 0;

	// 진화 분류(연계 / 제어 / 버프 / 형태변경 …). 플레이어에게 보이는 라벨이라 FText 다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Evolve")
	FText Change_Type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Evolve")
	FText Change_Summary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Cost")
	FName Chip_Grade;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Cost")
	int32 Required_Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode/Cost")
	int32 Coin_Cost = 0;
};
