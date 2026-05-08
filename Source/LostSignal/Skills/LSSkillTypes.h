#pragma once

#include "CoreMinimal.h"
#include "LSSkillTypes.generated.h"

class ULSSkillDataAsset;

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
