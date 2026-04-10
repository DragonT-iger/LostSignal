#pragma once

#include "Abilities/GameplayAbility.h"
#include "LSGA_Dash.generated.h"

/**
 * 대쉬 GameplayAbility.
 *
 * 동작 흐름:
 *   1. 입력 방향(없으면 캐릭터 전방)으로 LaunchCharacter
 *   2. InvincibilityEffectClass GE 적용 → ASC에 LS.State.Invincible 태그 부여
 *   3. DashDuration 후 타이머 발화 → GE 제거 → EndAbility
 *
 * 무적 판정:
 *   데미지 처리 코드에서 ASC->HasMatchingGameplayTag(State_Invincible) 체크.
 *
 * 에디터 설정:
 *   BP_GA_Dash (ULSGA_Dash 파생 BP)에서 InvincibilityEffectClass에
 *   GE_DashInvincible Blueprint 에셋을 할당해야 한다.
 */
UCLASS()
class LOSTSIGNAL_API ULSGA_Dash : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULSGA_Dash();

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

	/** 대쉬 속도 (cm/s). Duration과 무관하게 독립적으로 조정 */
	UPROPERTY(EditDefaultsOnly, Category="Dash|Config")
	float DashSpeed = 2000.f;

	/** 대쉬 지속 시간 (초). 무적 유지 시간과 동일 */
	UPROPERTY(EditDefaultsOnly, Category="Dash|Config")
	float DashDuration = 0.3f;

	/**
	 * 대쉬 중 무적을 부여하는 GameplayEffect 클래스.
	 * GE는 Blueprint 에셋으로 관리한다 — 에디터에서 GE_DashInvincible 할당 필요.
	 *
	 * GE 설정값:
	 *   - Duration Policy: Infinite
	 *   - Granted Tags: LS.State.Invincible, LS.State.Dodging
	 */
	UPROPERTY(EditDefaultsOnly, Category="Dash|Effects")
	TSubclassOf<UGameplayEffect> InvincibilityEffectClass;

private:
	FTimerHandle DashTimerHandle;
	FActiveGameplayEffectHandle InvincibilityHandle;
	uint16 RootMotionSourceID = 0;
};
