#pragma once

#include "CoreMinimal.h"
#include "Data/LSCharacterSkillRow.h"
#include "Engine/DataTable.h"
#include "LSComboAttackRow.generated.h"

/** Basic combo attack DataTable Row. Combo_ID is normalized from the row name on CSV import. */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSComboAttackRow : public FTableRowBase
{
	GENERATED_BODY()

	virtual void OnPostDataImport(const UDataTable* InDataTable, const FName InRowName, TArray<FString>& OutCollectedImportProblems) override
	{
		NormalizeComboIDFromRowName(InRowName);
	}

	virtual void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName) override
	{
		NormalizeComboIDFromRowName(InRowName);
	}

	void NormalizeComboIDFromRowName(const FName InRowName)
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

		Combo_ID = FCString::Atoi(*RowNameString);
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo")
	int32 Combo_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo")
	int32 Combo_Char = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo")
	FText Combo_Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo")
	int32 Combo_Tag = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo")
	int32 Combo_Index = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo")
	float Combo_Time = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo")
	float Combo_Input_Window = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo/Range")
	ELSCharacterSkillRangeShape Range_Shape = ELSCharacterSkillRangeShape::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo/Range")
	float Range_X = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo/Range")
	float Range_Y = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo/Range")
	float Range_Z = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo")
	int32 Combo_HitCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo")
	float Combo_HitRate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo")
	float Combo_Multiplier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo")
	int32 Combo_Guard = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo")
	int32 Combo_Impact = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo/Resource")
	int32 Consume_Res_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo/Resource")
	float Consume_Res_Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo/Animation")
	FName Combo_Ani;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo/Effect")
	FString Combo_Effects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo/Effect")
	int32 Status_ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo/Effect")
	ELSCharacterSkillEffectTarget Effect_Target = ELSCharacterSkillEffectTarget::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo/Effect")
	float Effect_Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo/Effect")
	int32 Status_ID_2 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo/Effect")
	ELSCharacterSkillEffectTarget Effect_Target_2 = ELSCharacterSkillEffectTarget::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combo/Effect")
	float Effect_Duration_2 = 0.0f;
};
