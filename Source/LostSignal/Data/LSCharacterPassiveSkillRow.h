#pragma once

#include "CoreMinimal.h"
#include "Data/LSCharacterSkillRow.h"
#include "Engine/DataTable.h"
#include "LSCharacterPassiveSkillRow.generated.h"

UENUM(BlueprintType)
enum class ELSCharacterPassiveTriggerEvent : uint8
{
	None,
	Always,
	OnComboEnd,
	OnDamaged,
	OnGuard,
	OnIdle,
	Onkill,
	OnKill,
	OnSkillCast,
	OnStrike
};

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSCharacterPassiveStatusEffectEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	int32 Status_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	ELSCharacterSkillEffectTarget Effect_Target = ELSCharacterSkillEffectTarget::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	float Effect_Duration = 0.0f;
};

/** 캐릭터 패시브 스킬 DataTable Row */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSCharacterPassiveSkillRow : public FTableRowBase
{
	GENERATED_BODY()

	virtual void OnPostDataImport(const UDataTable* InDataTable, const FName InRowName, TArray<FString>& OutCollectedImportProblems) override
	{
		NormalizePassiveSkillIDFromRowName(InRowName);
	}

	virtual void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName) override
	{
		NormalizePassiveSkillIDFromRowName(InRowName);
	}

	void NormalizePassiveSkillIDFromRowName(const FName InRowName)
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

		PassiveSkill_ID = FCString::Atoi(*RowNameString);
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	int32 PassiveSkill_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	int32 Skill_Char = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	FText Skill_Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	FText Skill_Info;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	ELSCharacterSkillTarget Skill_Target = ELSCharacterSkillTarget::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Trigger")
	ELSCharacterPassiveTriggerEvent Trigger_Event = ELSCharacterPassiveTriggerEvent::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Trigger")
	int32 Trigger_Target_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill")
	float Skill_Cooldown = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	TArray<FLSCharacterPassiveStatusEffectEntry> Skill_Effects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	int32 Status_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	ELSCharacterSkillEffectTarget Effect_Target = ELSCharacterSkillEffectTarget::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	float Effect_Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	int32 Status_ID_2 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	ELSCharacterSkillEffectTarget Effect_Target_2 = ELSCharacterSkillEffectTarget::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Skill/Effect")
	float Effect_Duration_2 = 0.0f;
};
