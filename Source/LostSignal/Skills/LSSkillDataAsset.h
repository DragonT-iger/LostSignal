#pragma once

#include "CoreMinimal.h"
#include "Skills/LSSkillDataAssetBase.h"
#include "Skills/LSSkillAreaTypes.h"
#include "Skills/LSSkillTypes.h"
#include "LSSkillDataAsset.generated.h"

class UGameplayEffect;

/** Active skill DataAsset. Active numeric values are resolved from FLSCharacterSkillRow by Skill_ID. */
UCLASS(BlueprintType)
class LOSTSIGNAL_API ULSSkillDataAsset : public ULSSkillDataAssetBase
{
	GENERATED_BODY()

public:
	ULSSkillDataAsset();

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	FLSSkillAreaPreviewSpec BuildPreviewSpec() const;

	virtual FGameplayTag GetCooldownTag() const override;

	UFUNCTION(BlueprintPure, Category="LS/Skill|DataTable")
	int32 GetSkillID() const;

	UFUNCTION(BlueprintPure, Category="LS/Skill|DataTable")
	FName GetSkillRowName() const;

	UFUNCTION(BlueprintPure, Category="LS/Skill|Enhancement")
	ULSSkillDataAsset* GetEnhancementVariant(int32 Index) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|DataTable")
	int32 Skill_ID = 0;

	//Preview Effect
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview")
	FLSSkillAreaPreviewSpec PreviewSpec;

	//Base Damage
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Damage")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Enhancement")
	TArray<TObjectPtr<ULSSkillDataAsset>> EnhancementVariants;

};
