#pragma once

#include "Abilities/GameplayAbility.h"
#include "LSGA_Bypass.generated.h"

/** GameplayAbility version of Bypass sliding movement. */
UCLASS()
class LOSTSIGNAL_API ULSGA_Bypass : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULSGA_Bypass();

	bool ResolveMovementParams(const class ULSSkillDataAsset* SkillData, float& OutDistance, float& OutDuration) const;

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Bypass", meta=(ClampMin="0.0"))
	float FallbackDistance = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Bypass", meta=(ClampMin="0.0"))
	float FallbackDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Bypass|Debug")
	bool bEnableDebugLog = false;

private:
	void FinishBypass();
	void ApplyBypassStartEffects(float Duration);
	void SetInvincibleTagActive(bool bActive);

	FTimerHandle BypassTimerHandle;
	uint16 RootMotionSourceID = 0;
	bool bInvincibleTagActive = false;

	UPROPERTY(Transient)
	TObjectPtr<class ULSSkillDataAsset> ActiveSkillData = nullptr;
};
