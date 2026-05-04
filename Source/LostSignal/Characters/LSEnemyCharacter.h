// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/LSCharacterBase.h"
#include "GameplayTagContainer.h"
#include "LSEnemyCharacter.generated.h"

class UDataTable;
class UGameplayAbility;
class UAnimMontage;
class ULSMonsterCombatComponent;
class ULSMonsterSenseComponent;
struct FLSMonsterArchetypeRow;

/**
 * Per-ability montage entry owned by the monster actor, not by the GameplayAbility.
 * This keeps authored animation choices on the character/BP side, just like the player character setup.
 */
USTRUCT(BlueprintType)
struct FLSMonsterAbilityMontageEntry
{
	GENERATED_BODY()

	/** Ability tag used by StateTree/GAS when this attack is requested. */
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	FGameplayTag AbilityTag;

	/** Montage the monster should play when the matching ability activates. */
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TObjectPtr<UAnimMontage> Montage = nullptr;
};

/** Base enemy pawn that wires monster AI components, animation data, and grants the first melee ability slice. */
UCLASS(Abstract)
class LOSTSIGNAL_API ALSEnemyCharacter : public ALSCharacterBase
{
	GENERATED_BODY()

public:
	ALSEnemyCharacter();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category="LS/AI")
	ULSMonsterSenseComponent* GetMonsterSenseComponent() const { return MonsterSenseComponent; }

	UFUNCTION(BlueprintPure, Category="LS/AI")
	ULSMonsterCombatComponent* GetMonsterCombatComponent() const { return MonsterCombatComponent; }

	/** Returns the montage authored for the requested ability tag, if any. */
	UFUNCTION(BlueprintPure, Category="LS/Combat")
	UAnimMontage* GetAbilityMontage(FGameplayTag AbilityTag) const;

protected:
	/** Ability granted on BeginPlay so StateTree can request a basic attack by gameplay tag. */
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TSubclassOf<UGameplayAbility> DefaultAttackAbilityClass;

	/** Character-owned montage map read by monster abilities at runtime. */
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TArray<FLSMonsterAbilityMontageEntry> AbilityMontages;

	UPROPERTY(EditDefaultsOnly, Category="LS/AI|DataTable")
	TObjectPtr<UDataTable> MonsterArchetypeTable;

	UPROPERTY(EditDefaultsOnly, Category="LS/AI|DataTable")
	FName MonsterRowName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/AI")
	TObjectPtr<ULSMonsterSenseComponent> MonsterSenseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/AI")
	TObjectPtr<ULSMonsterCombatComponent> MonsterCombatComponent;

private:
	const FLSMonsterArchetypeRow* FindMonsterArchetypeRow() const;
	void InitializeMonsterArchetype();
};
