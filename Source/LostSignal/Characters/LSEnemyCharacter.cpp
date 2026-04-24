// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/LSEnemyCharacter.h"

#include "AI/LSAIController.h"
#include "AI/LSMonsterCombatComponent.h"
#include "AI/LSMonsterSenseComponent.h"
#include "Data/LSMonsterArchetypeRow.h"
#include "Engine/DataTable.h"
#include "LostSignal.h"

ALSEnemyCharacter::ALSEnemyCharacter()
{
	AIControllerClass = ALSAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	MonsterSenseComponent = CreateDefaultSubobject<ULSMonsterSenseComponent>(TEXT("MonsterSenseComponent"));
	MonsterCombatComponent = CreateDefaultSubobject<ULSMonsterCombatComponent>(TEXT("MonsterCombatComponent"));
}

void ALSEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitializeMonsterArchetype();

	if (HasAuthority())
	{
		if (ALSAIController* LSAIController = Cast<ALSAIController>(GetController()))
		{
			LSAIController->TryStartStateTreeLogic();
		}
	}
}

const FLSMonsterArchetypeRow* ALSEnemyCharacter::FindMonsterArchetypeRow() const
{
	if (!MonsterArchetypeTable || MonsterRowName.IsNone())
	{
		return nullptr;
	}

	return MonsterArchetypeTable->FindRow<FLSMonsterArchetypeRow>(MonsterRowName, TEXT("LSEnemyCharacter"));
}

void ALSEnemyCharacter::InitializeMonsterArchetype()
{
	const FLSMonsterArchetypeRow* Row = FindMonsterArchetypeRow();
	if (!Row)
	{
		UE_LOG(LogLS, Log, TEXT("%s: Monster archetype row missing, using component defaults."), *GetNameSafe(this));
		return;
	}

	if (MonsterSenseComponent)
	{
		MonsterSenseComponent->ApplyArchetype(*Row);
	}

	if (MonsterCombatComponent)
	{
		MonsterCombatComponent->ApplyArchetype(*Row);
	}
}
