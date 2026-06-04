#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/SoftObjectPath.h"
#include "LSStatusEffectRow.generated.h"

UENUM(BlueprintType)
enum class ELSStatusEffectGroup : uint8
{
	None,
	Buff,
	CC,
	Debuff,
	Tag
};

UENUM(BlueprintType)
enum class ELSStatusEffectStackRule : uint8
{
	None,
	Add,
	Ignore,
	Refresh
};

UENUM(BlueprintType)
enum class ELSStatusEffectMathType : uint8
{
	None,
	Flat,
	Percent
};

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSStatusEffectStatModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect/Modifier")
	FName Target_Stat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect/Modifier")
	ELSStatusEffectMathType Math_Type = ELSStatusEffectMathType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect/Modifier")
	float Mod_Value = 0.0f;
};

/** 캐릭터 상태이상 DataTable Row */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSStatusEffectRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect")
	FText Status_Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect")
	ELSStatusEffectGroup Status_Group = ELSStatusEffectGroup::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect")
	ELSStatusEffectStackRule Stack_Rule = ELSStatusEffectStackRule::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect/Modifier")
	TArray<FLSStatusEffectStatModifier> Stat_Modifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect")
	int32 Max_Stack = 1;

	// 기획 테이블의 오탈자 컬럼명(Is_Hiddien)을 그대로 둬 CSV import와 1:1 매칭한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect")
	bool Is_Hiddien = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect/Asset")
	FSoftObjectPath UI_Icon_Path;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect/Asset")
	FSoftObjectPath FX_Path;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect/Modifier")
	FName Target_Stat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect/Modifier")
	ELSStatusEffectMathType Math_Type = ELSStatusEffectMathType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect/Modifier")
	float Mod_Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect/Modifier")
	FName Target_Stat_2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect/Modifier")
	ELSStatusEffectMathType Math_Type_2 = ELSStatusEffectMathType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/StatusEffect/Modifier")
	float Mod_Value_2 = 0.0f;
};
