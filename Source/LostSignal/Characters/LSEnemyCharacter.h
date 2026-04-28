// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/LSCharacterBase.h"
#include "LSEnemyCharacter.generated.h"

class UDataTable;
class ULSMonsterCombatComponent;
class ULSMonsterSenseComponent;
struct FLSMonsterArchetypeRow;

/** Base enemy pawn that wires monster AI components and archetype data. */
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

protected:
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
