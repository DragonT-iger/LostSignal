#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LSCharacterBase.generated.h"

class UAbilitySystemComponent;
class ULSCombatAttributeSet;
class UGameplayAbility;
class UAnimMontage;
class ULSCharacterCombatComponent;
class ULSCombatStateComponent;
class ULSStatusEffectComponent;
class UNiagaraSystem;
class USoundBase;

UCLASS(Abstract)
class LOSTSIGNAL_API ALSCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ALSCharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category="LS/GAS")
	ULSCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributeSet; }

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	ULSCharacterCombatComponent* GetCharacterCombatComponent() const { return CharacterCombatComponent; }

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	ULSCombatStateComponent* GetCombatStateComponent() const { return CombatStateComponent; }

	UFUNCTION(BlueprintPure, Category="LS/StatusEffect")
	ULSStatusEffectComponent* GetStatusEffectComponent() const { return StatusEffectComponent; }

	USoundBase* GetFootstepSound() const { return FootstepSound; }
	UNiagaraSystem* GetFootstepVFX() const { return FootstepVFX; }

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayLSMontage(UAnimMontage* Montage, FName StartSection = NAME_None, float PlayRate = 1.0f);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayLSSkillMontage(UAnimMontage* Montage, FRotator SkillActivationRotation, FName StartSection = NAME_None, float PlayRate = 1.0f);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpLSMontageSection(UAnimMontage* Montage, FName SectionName);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetLSMontageNextSection(UAnimMontage* Montage, FName SectionNameToChange, FName NextSection);

	// 재생 중인 몽타주의 playRate만 바꾼다(재생 재시작·블렌드 없음). 다구간 스킬의 구간별 속도 전환용.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetLSMontagePlayRate(UAnimMontage* Montage, float PlayRate);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopLSMontage(UAnimMontage* Montage, float BlendOutTime);

	void SetSkillActivationRotation(const FRotator& InRotation);
	bool TryGetSkillActivationRotation(FRotator& OutRotation) const;
	void ClearSkillActivationRotation();

	void GrantAbility(TSubclassOf<UGameplayAbility> AbilityClass);

	/** 사망 상태가 바뀔 때 CharacterCombatComponent가 모든 머신에서 호출. 파생 클래스가 콜리전·마커 등 사망 후처리를 붙이는 확장점. 기본 동작 없음. */
	virtual void OnDeathStateChanged(bool bIsDead) {}

	/** 넉백 시작/종료 시 CharacterCombatComponent가 서버 권한에서 호출. 파생(몬스터)이 연출(몽타주/애니 정지)을 붙이는 확장점. 기본 동작 없음. */
	virtual void OnKnockbackStateChanged(bool bKnockbackActive) {}

protected:
	virtual void BeginPlay() override;

	void PlayLSMontageLocal(UAnimMontage* Montage, FName StartSection, float PlayRate);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/GAS")
	TObjectPtr<ULSCombatAttributeSet> CombatAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Combat")
	TObjectPtr<ULSCharacterCombatComponent> CharacterCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Combat")
	TObjectPtr<ULSCombatStateComponent> CombatStateComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/StatusEffect")
	TObjectPtr<ULSStatusEffectComponent> StatusEffectComponent;

	// 발소리(Sound Cue로 변주). LSAN_Footstep 노티파이가 접지 프레임마다 조회. 캐릭터 BP에서 매핑(미할당이면 무음).
	UPROPERTY(EditDefaultsOnly, Category="LS/Audio")
	TObjectPtr<USoundBase> FootstepSound;

	// 발소리와 함께 접지한 발 위치에 스폰되는 이펙트(먼지 등). 캐릭터 BP에서 매핑(미할당이면 스폰 없음).
	UPROPERTY(EditDefaultsOnly, Category="LS/VFX")
	TObjectPtr<UNiagaraSystem> FootstepVFX;

	FRotator CachedSkillActivationRotation = FRotator::ZeroRotator;
	bool bHasSkillActivationRotation = false;
};
