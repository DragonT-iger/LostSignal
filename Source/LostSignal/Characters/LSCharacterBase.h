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
class ULSFootstepComponent;

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

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayLSMontage(UAnimMontage* Montage, FName StartSection = NAME_None, float PlayRate = 1.0f);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpLSMontageSection(UAnimMontage* Montage, FName SectionName);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetLSMontageNextSection(UAnimMontage* Montage, FName SectionNameToChange, FName NextSection);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopLSMontage(UAnimMontage* Montage, float BlendOutTime);

	void GrantAbility(TSubclassOf<UGameplayAbility> AbilityClass);

	/** 사망 상태가 바뀔 때 CharacterCombatComponent가 모든 머신에서 호출. 파생 클래스가 콜리전·마커 등 사망 후처리를 붙이는 확장점. 기본 동작 없음. */
	virtual void OnDeathStateChanged(bool bIsDead) {}

protected:
	virtual void BeginPlay() override;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Audio")
	TObjectPtr<ULSFootstepComponent> FootstepComponent;
};
