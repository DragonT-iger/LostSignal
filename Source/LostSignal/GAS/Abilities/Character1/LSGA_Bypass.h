#pragma once

#include "Abilities/GameplayAbility.h"
#include "LSGA_Bypass.generated.h"

struct FLSCharacterSkillRow;

/** GameplayAbility version of Bypass sliding movement. */
UCLASS()
class LOSTSIGNAL_API ULSGA_Bypass : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULSGA_Bypass();

	bool ResolveMovementParams(const class ULSSkillDataAsset* SkillData, const FLSCharacterSkillRow* SkillRow, float& OutDistance, float& OutDuration) const;

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
	void ApplySpoofingStartEffects(const FVector& HologramLocation);
	void IgnoreEnemiesForBypass(ACharacter* SourceCharacter, const FVector& StartLocation, const FVector& Direction, float Distance);
	void ClearIgnoredEnemiesForBypass(ACharacter* SourceCharacter);
	static void PullTargetsToHologram(AActor* SourceActor, FVector HologramLocation, class ULSBypassSkillDataAsset* BypassData);
	static void ApplySpoofingStunIfConfigured(AActor* TargetActor, const class ULSBypassSkillDataAsset* BypassData);
	static void ScheduleSpoofingStun(AActor* TargetActor, const class ULSBypassSkillDataAsset* BypassData, float DelaySeconds);
	void SetInvincibleTagActive(bool bActive);

	FTimerHandle BypassTimerHandle;
	FTimerHandle BypassSpoofingTimerHandle;
	uint16 RootMotionSourceID = 0;
	TArray<TWeakObjectPtr<AActor>> IgnoredEnemyActors;
	bool bInvincibleTagActive = false;

	UPROPERTY(Transient)
	TObjectPtr<class ULSSkillDataAsset> ActiveSkillData = nullptr;
};
