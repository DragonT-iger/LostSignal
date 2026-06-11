// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/LSCharacterBase.h"
#include "GameplayTagContainer.h"
#include "LSEnemyCharacter.generated.h"

class UDataTable;
class UGameplayAbility;
class UAnimMontage;
class ULSCharacterAttributeSet;
class ULSHpDebugWidget;
class ULSMinimapMarkerComponent;
class ULSMonsterCombatComponent;
class ULSMonsterSenseComponent;
class ULSNoiseEmitterComponent;
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

	UFUNCTION(BlueprintPure, Category="LS/Minimap")
	ULSMinimapMarkerComponent* GetMinimapMarkerComponent() const { return MinimapMarkerComponent; }

	UFUNCTION(BlueprintPure, Category="LS/Noise")
	ULSNoiseEmitterComponent* GetNoiseEmitterComponent() const { return NoiseEmitterComponent; }

	UFUNCTION(BlueprintPure, Category="LS/GAS")
	ULSCharacterAttributeSet* GetMonsterAttributeSet() const { return MonsterAttributeSet; }

	/** Returns the montage authored for the requested ability tag, if any. */
	UFUNCTION(BlueprintPure, Category="LS/Combat")
	UAnimMontage* GetAbilityMontage(FGameplayTag AbilityTag) const;

	/** Returns the death montage authored for this enemy. */
	UFUNCTION(BlueprintPure, Category="LS/Combat")
	UAnimMontage* GetDeathMontage() const { return DeathMontage; }

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayAbilityMontage(UAnimMontage* Montage);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopAbilityMontage(UAnimMontage* Montage, float BlendOutTime);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Ability granted on BeginPlay so StateTree can request a basic attack by gameplay tag. */
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TSubclassOf<UGameplayAbility> DefaultAttackAbilityClass;

	/** Character-owned montage map read by monster abilities at runtime. */
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TArray<FLSMonsterAbilityMontageEntry> AbilityMontages;

	/** Death animation authored per enemy BP and played by the Dead state before AI logic stops. */
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TObjectPtr<UAnimMontage> DeathMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="LS/AI|DataTable")
	TObjectPtr<UDataTable> MonsterArchetypeTable;

	UPROPERTY(EditDefaultsOnly, Category="LS/AI|DataTable")
	FName MonsterRowName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/AI")
	TObjectPtr<ULSMonsterSenseComponent> MonsterSenseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/AI")
	TObjectPtr<ULSMonsterCombatComponent> MonsterCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Minimap")
	TObjectPtr<ULSMinimapMarkerComponent> MinimapMarkerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Noise")
	TObjectPtr<ULSNoiseEmitterComponent> NoiseEmitterComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/GAS")
	TObjectPtr<ULSCharacterAttributeSet> MonsterAttributeSet;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Debug")
	bool bCreateDebugHpWidget = true;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Debug")
	TSubclassOf<ULSHpDebugWidget> DebugHpWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Debug")
	FVector2D DebugHpWidgetBasePosition = FVector2D(40.0f, 120.0f);

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Debug", meta=(ClampMin="0.0"))
	float DebugHpWidgetVerticalSpacing = 60.0f;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/UI|Debug")
	TObjectPtr<ULSHpDebugWidget> DebugHpWidgetInstance;

private:
	const FLSMonsterArchetypeRow* FindMonsterArchetypeRow() const;
	void InitializeMonsterArchetype();
	void ApplyMonsterAttributes(const FLSMonsterArchetypeRow& Row);
	void TryCreateDebugHpWidget();
	void DestroyDebugHpWidget();

	bool bWarnedMissingDebugHpWidgetClass = false;
};
