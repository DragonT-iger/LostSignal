#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Skills/LSSkillAreaTypes.h"
#include "Skills/LSSkillTypes.h"
#include "LSSkillDataAsset.generated.h"

class ULSSkill;
struct FLSCharacterSkillRow;

UCLASS(BlueprintType)
class LOSTSIGNAL_API ULSSkillDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="LS/Skill")
	FLSSkillAreaPreviewSpec BuildPreviewSpec() const;

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	bool TryGetSkillRow(FLSCharacterSkillRow& OutRow) const;

	bool ActivateSkill(const FLSSkillActivationContext& Context) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TSubclassOf<ULSSkill> SkillClass;

private:
	ULSSkill* GetSkillDefaultObject() const;
};
