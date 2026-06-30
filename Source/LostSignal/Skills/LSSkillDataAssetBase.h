#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LSSkillDataAssetBase.generated.h"

class UAnimMontage;
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

	// 확정 입력 시 재생할 스킬 몽타주. 실제 효과는 몽타주의 LSAN_SkillEffect 노티파이 시점에 발동한다.
	// 미할당이면 발동 즉시 효과가 나가는 즉발로 동작한다(애니메이션 미적용 스킬 호환).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TObjectPtr<UAnimMontage> SkillMontage;

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
