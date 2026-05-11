#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSCharacterSkillRow.generated.h"

UENUM(BlueprintType)
enum class ELSCharacterSkillType : uint8
{
	None,
	Passive,
	Active
};

UENUM(BlueprintType)
enum class ELSCharacterSkillTarget : uint8
{
	None,
	Self,
	Area
};

UENUM(BlueprintType)
enum class ELSCharacterSkillRangeShape : uint8
{
	None,
	Cone,
	Circle,
	Box
};

/** DataTable row mapped from the Character_skill sheet. */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSCharacterSkillRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill")
	int32 Skill_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill")
	int32 Parent_Skill_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill")
	int32 Skill_Char = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill")
	FText Skill_Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill")
	FText Skill_Info;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill")
	FString Skill_Unlock;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill")
	ELSCharacterSkillType Skill_Type = ELSCharacterSkillType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill")
	ELSCharacterSkillTarget Skill_Target = ELSCharacterSkillTarget::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill")
	float Skill_Time = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Range")
	ELSCharacterSkillRangeShape Range_Shape = ELSCharacterSkillRangeShape::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Range")
	float Range_X = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Range")
	float Range_Y = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Range")
	float Range_Z = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Hit")
	int32 Skill_HitCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Hit")
	float Skill_HitRate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Damage")
	float Skill_Multiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill")
	float Skill_Cooldown = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Impact")
	int32 Skill_Guard = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Impact")
	int32 Skill_Impact = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Stack")
	int32 Skill_Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Stack")
	float Skill_Count_Multiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Effect")
	FString Skill_Effects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Effect")
	FString Skill_Effect_Type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Effect")
	float Skill_Effect_Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Effect")
	float Skill_Effect_Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Effect")
	FString Skill_Effect_Type_2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Effect")
	float Skill_Effect_Value_2 = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character|Skill|Effect")
	float Skill_Effect_Duration_2 = 0.0f;
};
