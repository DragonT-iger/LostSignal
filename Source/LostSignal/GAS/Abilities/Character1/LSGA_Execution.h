#pragma once

#include "GAS/Abilities/Character1/LSGA_PlayerSkillBase.h"
#include "Combat/LSCombatTypes.h"
#include "Data/LSCharacterSkillRow.h"
#include "LSGA_Execution.generated.h"

class UGameplayEffect;
class ULSExecutionSkillDataAsset;

/**
 * Ultimate skill: dash slash, sheath damage, and Short Circuit field detonation.
 * 대시(루트모션, DashDuration)와 발도 타격이 한 몽타주에 들어 있어 섹션 분할로 재생한다:
 * Dash 섹션은 DashDuration에 자동 스케일, 대시 타이머(서버 권위) 만료 시 Slash 섹션을 1.0 속도로 재생.
 * 타격은 Slash 구간의 LSAN_SkillEffect 노티파이 시점(누락 시 종료 직전 폴백 보장). 몽타주 미할당이면 대시 끝 즉발.
 */
UCLASS()
class LOSTSIGNAL_API ULSGA_Execution : public ULSGA_PlayerSkillBase
{
	GENERATED_BODY()

public:
	ULSGA_Execution();

	// 클라 예측(ULSPlayerSkillComponent)이 CDO로 호출한다. 시그니처 변경 금지.
	bool ResolveMovementParams(const class ULSSkillDataAsset* SkillData, const FLSCharacterSkillRow* SkillRow, float& OutDistance, float& OutDuration) const;

protected:
	virtual bool PrepareSkillExecution() override;
	virtual void OnSkillStarted() override;
	virtual void ExecuteSkillEffect() override;
	virtual float GetSkillMontagePlayRate() const override;
	virtual FName GetSkillMontageStartSection() const override;
	virtual bool ShouldMontageDriveEnd() const override { return false; }

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Execution")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Execution", meta=(ClampMin="0.0"))
	float FallbackAttackCoefficient = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Execution")
	ELSBreakPowerTier FallbackBreakPower = ELSBreakPowerTier::HardCrowdControl;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Execution")
	bool bCanCrit = false;

	// 몽타주의 대시/발도 섹션 이름. 아트가 다른 이름으로 오써링하면 여기서 맞춘다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Execution")
	FName DashSectionName = FName(TEXT("Dash"));

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Execution")
	FName SlashSectionName = FName(TEXT("Slash"));

private:
	// 대시 타이머 만료(서버 권위): Slash 섹션 전환 + 종료 타이머, 몽타주/섹션 없으면 즉시 종료 단계.
	void HandleDashFinished();
	// 종료 단계: 타격 미발동 시 폴백 1회 보장 후 EndAbility.
	void FinishExecution();
	int32 ConsumeCombatAccelerationStacks(AActor* SourceActor) const;
	bool IsPointInExecutionArea(const FVector& Point) const;
	void IgnoreEnemiesForDash(ACharacter* SourceCharacter);
	void ClearIgnoredEnemiesForDash(ACharacter* SourceCharacter);
	// ExecuteSkillEffect 분할: 대시 경로 범위 타격 / ShortCircuit 장판 폭발. 각각 명중/폭발 수를 반환한다.
	int32 ApplySheathDamage(AActor* SourceActor, class ULSCharacterCombatComponent* SourceCombatComponent, TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass, ELSBreakPowerTier ResolvedBreakPower) const;
	int32 ExplodeShortCircuitFields(AActor* SourceActor, ELSBreakPowerTier ResolvedBreakPower) const;

	// 대시 단계 → 종료 단계를 순차 재사용하는 단일 타이머 핸들.
	FTimerHandle PhaseTimerHandle;
	uint16 RootMotionSourceID = 0;
	TArray<TWeakObjectPtr<AActor>> IgnoredEnemyActors;

	UPROPERTY(Transient)
	TObjectPtr<ULSExecutionSkillDataAsset> ExecutionData = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class ULSSkillDataAsset> SkillData = nullptr;

	FVector CachedStartLocation = FVector::ZeroVector;
	FVector CachedDirection = FVector::ForwardVector;
	float CachedDashDistance = 0.0f;
	float CachedDashDuration = 0.0f;
	float CachedSlashWidth = 0.0f;
	float CachedAttackCoefficient = 0.0f;
	int32 CachedConsumedAccelerationStacks = 0;
	FLSCharacterSkillRow CachedSkillRow;
	bool bHasCachedSkillRow = false;
};
