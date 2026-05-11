#pragma once

#include "CoreMinimal.h"
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
