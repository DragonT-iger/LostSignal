#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LSCharacterBase.generated.h"

class UAbilitySystemComponent;
class ULSCombatAttributeSet;
class UGameplayAbility;
class ULSCharacterCombatComponent;

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

	void GrantAbility(TSubclassOf<UGameplayAbility> AbilityClass);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/GAS")
	TObjectPtr<ULSCombatAttributeSet> CombatAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Combat")
	TObjectPtr<ULSCharacterCombatComponent> CharacterCombatComponent;
};
