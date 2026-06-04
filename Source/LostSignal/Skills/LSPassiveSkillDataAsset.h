#pragma once

#include "CoreMinimal.h"
#include "Skills/LSSkillDataAssetBase.h"
#include "LSPassiveSkillDataAsset.generated.h"

/** Passive skill DataAsset base. Passive rows are looked up separately from active skill rows. */
UCLASS(BlueprintType)
class LOSTSIGNAL_API ULSPassiveSkillDataAsset : public ULSSkillDataAssetBase
{
	GENERATED_BODY()

public:
	ULSPassiveSkillDataAsset();

	UFUNCTION(BlueprintPure, Category="LS/Skill|DataTable")
	int32 GetPassiveSkillID() const;

	UFUNCTION(BlueprintPure, Category="LS/Skill|DataTable")
	FName GetPassiveSkillRowName() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|DataTable")
	int32 PassiveSkill_ID = 0;
};
