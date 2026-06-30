#pragma once

#include "GAS/Abilities/Character1/LSGA_PlayerSkillBase.h"
#include "LSGA_Bypass.generated.h"

struct FLSCharacterSkillRow;

/**
 * Bypass 슬라이드 이동 스킬.
 * 이동은 FRootMotionSource(클라 예측 + 서버 권위)가 담당하고, 종료 권위는 슬라이드 Duration 타이머가 가진다.
 * 몽타주는 비주얼 전용으로, playRate를 슬라이드 Duration에 자동으로 맞춰 재생한다(루트모션 트랙 없이 author).
 */
UCLASS()
class LOSTSIGNAL_API ULSGA_Bypass : public ULSGA_PlayerSkillBase
{
	GENERATED_BODY()

public:
	bool ResolveMovementParams(const class ULSSkillDataAsset* SkillData, const FLSCharacterSkillRow* SkillRow, float& OutDistance, float& OutDuration) const;

protected:
	virtual bool PrepareSkillExecution() override;
	virtual void OnSkillStarted() override;
	virtual float GetSkillMontagePlayRate() const override;
	virtual bool ShouldMontageDriveEnd() const override { return false; }

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

	// PrepareSkillExecution에서 해석·캐싱하여 OnSkillStarted(루트모션)·GetSkillMontagePlayRate(스케일)에서 쓴다.
	float CachedDistance = 0.0f;
	float CachedDuration = 0.0f;
	FVector CachedDirection = FVector::ForwardVector;

	UPROPERTY(Transient)
	TObjectPtr<class ULSSkillDataAsset> ActiveSkillData = nullptr;
};
