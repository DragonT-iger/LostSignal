#pragma once

#include "CoreMinimal.h"
#include "Data/LSCharacterSkillRow.h"
#include "LSSkillTypes.generated.h"

class ULSSkillDataAsset;

USTRUCT(BlueprintType)
struct FLSSkillActivationContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	TObjectPtr<ULSSkillDataAsset> SkillData = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	float AimYaw = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	FLSCharacterSkillRow SkillRow;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	bool bHasSkillRow = false;
};

USTRUCT(BlueprintType)
struct FLSBasicAttackHitContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	TObjectPtr<ULSSkillDataAsset> SkillData = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	int32 ComboIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	int32 ValidHitCount = 0;
};

UENUM(BlueprintType)
enum class ELSPlayerSkillSlot : uint8
{
	Skill1,
	Skill2,
	Skill3,
	Skill4,
	Ultimate
};

USTRUCT(BlueprintType)
struct FLSPlayerSkillSlotSpec
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TObjectPtr<ULSSkillDataAsset> SkillData = nullptr;
};
