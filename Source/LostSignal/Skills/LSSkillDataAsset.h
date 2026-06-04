#pragma once

#include "CoreMinimal.h"
#include "Combat/LSCombatTypes.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Skills/LSSkillAreaTypes.h"
#include "Skills/LSSkillTypes.h"
#include "LSSkillDataAsset.generated.h"

class UGameplayAbility;
class UGameplayEffect;

UCLASS(BlueprintType)
class LOSTSIGNAL_API ULSSkillDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	ULSSkillDataAsset();

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	FLSSkillAreaPreviewSpec BuildPreviewSpec() const;

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	TSubclassOf<UGameplayAbility> GetAbilityClass() const;

	UFUNCTION(BlueprintPure, Category="LS/Skill|Cooldown")
	float GetCooldownDuration() const;

	UFUNCTION(BlueprintPure, Category="LS/Skill|Cooldown")
	FGameplayTag GetCooldownTag() const;

	UFUNCTION(BlueprintPure, Category="LS/Skill|DataTable")
	int32 GetSkillID() const;

	UFUNCTION(BlueprintPure, Category="LS/Skill|DataTable")
	FName GetSkillRowName() const;

	UFUNCTION(BlueprintPure, Category="LS/Skill|Enhancement")
	ULSSkillDataAsset* GetEnhancementVariant(int32 Index) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Cooldown")
	FGameplayTag CooldownTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Cooldown", meta=(ClampMin="0.0"))
	float FallbackCooldown = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Cooldown")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Damage", meta=(ClampMin="0.0"))
	float AttackCoefficient = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Damage")
	bool bCanCrit = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Damage")
	ELSBreakPowerTier BreakPower = ELSBreakPowerTier::NormalAttack;

	//UI Info
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LS/Skill|UI")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LS/Skill|UI")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LS/Skill|UI")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Enhancement")
	TArray<TObjectPtr<ULSSkillDataAsset>> EnhancementVariants;

};
