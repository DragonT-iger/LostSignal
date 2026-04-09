#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "LSCharacterBase.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

/**
 * Abstract base character for LostSignal.
 *
 * ACharacter
 * └── ALSCharacterBase [abstract] ← GAS IAbilitySystemInterface, 공통 로직
 *     ├── ALSPlayerCharacter [abstract] ← 카메라, 마우스 추적, Enhanced Input
 *     │   └── BP_PlayerCharacter ← (메시, 에셋 매핑만)
 *     └── ALSEnemyCharacter [abstract] ← 카메라 없음, AI용 Move/Look 오버라이드
 *         └── BP_Enemy_Base ← (메시, 에셋 매핑만)
 *
 * GAS 구조:
 *   ASC를 캐릭터에 직접 소유 (PlayerState 아님 — 싱글 먼저).
 *   멀티 전환 시 ASC를 PlayerState로 이동하고 InitAbilityActorInfo 경로만 수정.
 */
UCLASS(Abstract)
class LOSTSIGNAL_API ALSCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ALSCharacterBase();

	// IAbilitySystemInterface — GAS 내부에서 ASC를 찾을 때 호출
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/**
	 * 어빌리티를 이 캐릭터에게 부여한다.
	 * Unity의 "컴포넌트 추가"와 유사 — 서버(HasAuthority)에서만 실행.
	 */
	void GrantAbility(TSubclassOf<UGameplayAbility> AbilityClass);

protected:
	virtual void BeginPlay() override;

	/**
	 * 모든 캐릭터(플레이어·적)가 공유하는 AbilitySystemComponent.
	 * Unity의 스탯/상태 컴포넌트에 해당하며, GAS의 핵심 허브 역할.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
};
