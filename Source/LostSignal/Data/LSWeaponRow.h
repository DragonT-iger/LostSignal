#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSWeaponRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSWeaponRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FString Item_Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Type = 1;

	// 0=보급, 1=표준, 2=정밀, 3=튜닝, 4=프로토타입, 5=마스터피스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Grade = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Max = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FString Item_Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Cost = 0;

	// Weapon_1=근접, Weapon_2=원거리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FString Item_Equipment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Stats")
	float Item_Attack = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Stats")
	float Item_Attack_Speed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Stats")
	float Item_Skill_Haste = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Stats")
	float Item_Critical_Rate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Stats")
	float Item_Critical_Damage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Stats")
	float Item_Defense_Penetration = 0.0f;
};
