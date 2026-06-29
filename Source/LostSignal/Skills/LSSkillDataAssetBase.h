#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LSSkillDataAssetBase.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class USoundBase;

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

	// 스킬 발동 순간 재생할 시전음. 발동 시 GameplayCueParameters로 실려 전 클라에서 재생된다(미할당이면 무음).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Audio")
	TObjectPtr<USoundBase> CastSound;
};
