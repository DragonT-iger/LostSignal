#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Skills/LSSkillAreaTypes.h"
#include "Skills/LSSkillTypes.h"
#include "LSSkillDataAsset.generated.h"

class ULSSkill;
class UGameplayAbility;
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

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	TSubclassOf<UGameplayAbility> GetAbilityClass() const;

	bool ActivateSkill(const FLSSkillActivationContext& Context) const;
	bool HandleBasicAttackHit(const FLSBasicAttackHitContext& Context) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TSubclassOf<ULSSkill> SkillClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TSubclassOf<UGameplayAbility> AbilityClass;

private:
	ULSSkill* GetSkillDefaultObject() const;
};
