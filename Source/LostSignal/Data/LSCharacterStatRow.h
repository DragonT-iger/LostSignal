#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSCharacterStatRow.generated.h"

/** 기획자 엑셀 Character_stat 시트와 1:1 매핑되는 DataTable 행 구조체 */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSCharacterStatRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Character")
	FText Char_Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", DisplayName="공격력")
	float Char_Attack = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", DisplayName="공격속도")
	float Char_Atkspead = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", DisplayName="스킬가속")
	float Char_Cal = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", DisplayName="치명타 확률")
	float Char_Crit = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", DisplayName="치명타 배율")
	float Char_CritDmg = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", DisplayName="방관통")
	float Char_ArmorPen = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", DisplayName="체력")
	float Char_Health = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", DisplayName="방어")
	float Char_Defence = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", DisplayName="회복력")
	float Char_Recovery = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", DisplayName="스태미나")
	float Char_Stamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", DisplayName="이동속도")
	float Char_Speed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", DisplayName="대쉬속도")
	float Char_DashSpeed = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", DisplayName="대쉬 지속시간(초)")
	float Char_DashDuration = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", DisplayName="대쉬 쿨타임(초)")
	float Char_DashCooldown = 1.0f;
};
