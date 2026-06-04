#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSCharacterSkillRow.generated.h"

UENUM(BlueprintType)
enum class ELSCharacterSkillInputType : uint8
{
	None,
	Down,
	Hold,
	Togle
};

UENUM(BlueprintType)
enum class ELSCharacterSkillType : uint8
{
	None,
	Passive,
	Active,
	Buff,
	Ultimate
};

UENUM(BlueprintType)
enum class ELSCharacterSkillTarget : uint8
{
	None,
	Self,
	Area,
	Area_Ally
};

UENUM(BlueprintType)
enum class ELSCharacterSkillRangeShape : uint8
{
	None,
	Cone,
	Circle,
	Box
};

UENUM(BlueprintType)
enum class ELSCharacterSkillCrowdControlType : uint8
{
	None,
	KnockBack,
	Pull
};

UENUM(BlueprintType)
enum class ELSCharacterSkillEffectTarget : uint8
{
	None,
	Self,
	Ally,
	Target
};

/** 캐릭터 액티브 스킬 DataTable Row */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSCharacterSkillRow : public FTableRowBase
{
	GENERATED_BODY()

	virtual void OnPostDataImport(const UDataTable* InDataTable, const FName InRowName, TArray<FString>& OutCollectedImportProblems) override
	{
		NormalizeSkillIDFromRowName(InRowName);
	}

	virtual void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName) override
	{
		NormalizeSkillIDFromRowName(InRowName);
	}

	void NormalizeSkillIDFromRowName(const FName InRowName)
	{
		const FString RowNameString = InRowName.ToString();
		if (RowNameString.IsEmpty())
		{
			return;
		}

		for (const TCHAR Character : RowNameString)
		{
			if (!FChar::IsDigit(Character))
			{
				return;
			}
		}

		Skill_ID = FCString::Atoi(*RowNameString);
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	int32 Skill_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	int32 Parent_Skill_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	int32 Skill_Char = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	FText Skill_Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	FText Skill_Info;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	ELSCharacterSkillInputType Skill_Input = ELSCharacterSkillInputType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	ELSCharacterSkillType Skill_Type = ELSCharacterSkillType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	ELSCharacterSkillTarget Skill_Target = ELSCharacterSkillTarget::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	float Skill_Time = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Range")
	ELSCharacterSkillRangeShape Range_Shape = ELSCharacterSkillRangeShape::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Range")
	float Range_X = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Range")
	float Range_Y = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Range")
	float Range_Z = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Range")
	float Cast_Range = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Hit")
	int32 Skill_HitCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Hit")
	float Skill_HitRate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Damage")
	float Skill_Multiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/CrowdControl")
	ELSCharacterSkillCrowdControlType CC_Type = ELSCharacterSkillCrowdControlType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/CrowdControl")
	float CC_Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	float Skill_Cooldown = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Impact")
	int32 Skill_Guard = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Impact")
	int32 Skill_Impact = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Resource")
	int32 Consume_Res_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Resource")
	float Consume_Res_Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Resource")
	float Res_Multiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Movement")
	float Move_Distance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Movement")
	float Move_Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	FString Skill_Effects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	int32 Status_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	ELSCharacterSkillEffectTarget Effect_Target = ELSCharacterSkillEffectTarget::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	float Skill_Effect_Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	int32 Status_ID_2 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	ELSCharacterSkillEffectTarget Effect_Target_2 = ELSCharacterSkillEffectTarget::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	float Skill_Effect_Duration_2 = 0.0f;
};
