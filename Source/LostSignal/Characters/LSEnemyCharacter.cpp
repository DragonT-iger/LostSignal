// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/LSEnemyCharacter.h"

#include "AI/LSAIController.h"
#include "AI/LSMonsterCombatComponent.h"
#include "AI/LSMonsterSenseComponent.h"
#include "Data/LSMonsterArchetypeRow.h"
#include "Engine/DataTable.h"
#include "GAS/Abilities/LSGA_MonsterMelee.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LostSignal.h"

ALSEnemyCharacter::ALSEnemyCharacter()
{
	AIControllerClass = ALSAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	DefaultAttackAbilityClass = ULSGA_MonsterMelee::StaticClass();

	// Player uses its own facing logic, but AI should rotate from controller focus while chasing targets.
	// bUseControllerDesiredRotation is the CharacterMovement-side switch that turns SetFocus() yaw into body rotation.
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

	MonsterSenseComponent = CreateDefaultSubobject<ULSMonsterSenseComponent>(TEXT("MonsterSenseComponent"));
	MonsterCombatComponent = CreateDefaultSubobject<ULSMonsterCombatComponent>(TEXT("MonsterCombatComponent"));
}

UAnimMontage* ALSEnemyCharacter::GetAbilityMontage(FGameplayTag AbilityTag) const
{
	for (const FLSMonsterAbilityMontageEntry& Entry : AbilityMontages)
	{
		if (Entry.AbilityTag == AbilityTag)
		{
			return Entry.Montage;
		}
	}

	return nullptr;
}

void ALSEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitializeMonsterArchetype();

	if (HasAuthority())
	{
		if (DefaultAttackAbilityClass)
		{
			// StateTree can only request abilities that already exist on the monster ASC.
			GrantAbility(DefaultAttackAbilityClass);
		}

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
