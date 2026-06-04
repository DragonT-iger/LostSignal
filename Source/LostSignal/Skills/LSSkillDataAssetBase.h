#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LSSkillDataAssetBase.generated.h"

class UGameplayAbility;
class UGameplayEffect;

/** Common DataAsset base shared by active and passive skill assets. */
UCLASS(Abstract, BlueprintType)
class LOSTSIGNAL_API ULSSkillDataAssetBase : public UDataAsset
{
	GENERATED_BODY()

public:
	ULSSkillDataAssetBase();

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	virtual TSubclassOf<UGameplayAbility> GetAbilityClass() const;

	UFUNCTION(BlueprintPure, Category="LS/Skill|Cooldown")
	virtual float GetCooldownDuration() const;

	UFUNCTION(BlueprintPure, Category="LS/Skill|Cooldown")
	virtual FGameplayTag GetCooldownTag() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Cooldown")
	FGameplayTag CooldownTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Cooldown", meta=(ClampMin="0.0"))
	float FallbackCooldown = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Cooldown")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|UI")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|UI")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|UI")
	TObjectPtr<UTexture2D> Icon;
};
