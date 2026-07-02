#pragma once

#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "Skills/LSSkillTypes.h"
#include "LSGA_PlayerSkillBase.generated.h"

class UAnimMontage;

/**
 * 플레이어 액티브 스킬 Ability 공통 베이스.
 *
 * 흐름:
 *   ActivateAbility
 *   -> 컨텍스트 소비 + PrepareSkillExecution()(서브클래스 검증/캐싱)
 *   -> CommitAbility + 쿨타임 적용
 *   -> SkillData.SkillMontage 재생 (전 클라 멀티캐스트)
 *   -> [임팩트 프레임] LSAN_SkillEffect 노티파이 -> GameplayEvent -> ExecuteSkillEffect()
 *   -> 몽타주 종료 -> EndAbility
 *
 * 몽타주가 없으면 즉발(발동 즉시 ExecuteSkillEffect 후 종료)로 동작한다.
 * 발동 차단/캔슬 태그, Instancing/Net 정책 등 공통 설정도 이 베이스 생성자에서 처리한다.
 */
UCLASS(Abstract)
class LOSTSIGNAL_API ULSGA_PlayerSkillBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULSGA_PlayerSkillBase();

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

	// 발동 직전 서브클래스 검증/캐싱. false를 반환하면 커밋·쿨타임 없이 발동을 취소한다.
	virtual bool PrepareSkillExecution() { return true; }

	// 커밋/쿨타임 직후·몽타주 재생 전 호출. 이동 루트모션·버프 등 "발동 즉시" 세팅용.
	virtual void OnSkillStarted() {}

	// 실제 스킬 효과(판정/데미지/CC/버프). 노티파이 시점 또는 즉발 fallback 시점에 한 번 호출된다.
	virtual void ExecuteSkillEffect() {}

	// 몽타주 재생 playRate. 이동 스킬은 override해 몽타주 길이를 이동 Duration에 맞춘다.
	virtual float GetSkillMontagePlayRate() const { return 1.0f; }

	// 몽타주 재생 시작 섹션. 다구간 스킬(Execution 대시 섹션 등)이 override한다. None이면 처음부터.
	virtual FName GetSkillMontageStartSection() const { return NAME_None; }

	// true면 몽타주 끝이 능력 종료를 주관(즉발/연출형). false면 서브클래스 타이머가 종료를 책임진다(이동 스킬).
	// false일 때 베이스는 몽타주를 스케일 재생만 하고 종료 델리게이트를 바인딩하지 않는다.
	virtual bool ShouldMontageDriveEnd() const { return true; }

	// 몽타주 길이를 TargetDuration에 맞추는 playRate. SectionName=None이면 전체 길이 기준. [0.25,3.0] 클램프.
	float ComputeMontagePlayRateForDuration(const UAnimMontage* Montage, FName SectionName, float TargetDuration) const;

	AActor* GetSkillSourceActor() const;
	UAnimMontage* GetSkillMontage() const;
	const FLSSkillActivationContext& GetSkillContext() const { return SkillContext; }

	// 효과를 아직 안 냈으면 ExecuteSkillEffect를 1회 실행(중복 가드). 타이머 종료형 스킬의 폴백 보장용.
	void TriggerSkillEffectOnce();

	// 효과를 발동시킬 GameplayEvent 태그. 비우면 LS.Event.Skill.Hit.
	UPROPERTY(EditDefaultsOnly, Category="LS/Skill")
	FGameplayTag SkillEffectEventTag;

private:
	UFUNCTION()
	void OnSkillEffectEventReceived(FGameplayEventData Payload);

	void HandleSkillMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 몽타주에 입력차단 NotifyState(LSANS_BlockInput)가 배치돼 있는지. 있으면 기본 전체 차단을 적용하지 않는다.
	static bool MontageHasInputBlockNotify(const UAnimMontage* Montage);

	// 기본 입력 차단(몽타주 전체) 적용/해제. NotifyState가 없는 몽타주에만 사용한다.
	void SetDefaultInputBlockActive(bool bActive);

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveSkillMontage = nullptr;

	FLSSkillActivationContext SkillContext;
	bool bSkillEffectExecuted = false;
	bool bEndingAbility = false;
	bool bDefaultInputBlockApplied = false;
};
