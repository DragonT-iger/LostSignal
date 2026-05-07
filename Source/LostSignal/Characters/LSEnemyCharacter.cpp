// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/LSEnemyCharacter.h"

#include "AI/LSAIController.h"
#include "AI/LSMonsterCombatComponent.h"
#include "AI/LSMonsterSenseComponent.h"
#include "Animation/AnimInstance.h"
#include "Blueprint/UserWidget.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSMonsterArchetypeRow.h"
#include "Engine/DataTable.h"
#include "GAS/Abilities/LSGA_MonsterMelee.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LostSignal.h"
#include "UI/Debug/LSHpDebugWidget.h"

namespace
{
	int32 GNextMonsterHpDebugWidgetStackIndex = 0;
}

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

void ALSEnemyCharacter::MulticastPlayAbilityMontage_Implementation(UAnimMontage* Montage)
{
	if (!Montage)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	AnimInstance->Montage_Play(Montage);
}

void ALSEnemyCharacter::MulticastStopAbilityMontage_Implementation(UAnimMontage* Montage, float BlendOutTime)
{
	if (!Montage)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !AnimInstance->Montage_IsPlaying(Montage))
	{
		return;
	}

	AnimInstance->Montage_Stop(BlendOutTime, Montage);
}

void ALSEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitializeMonsterArchetype();
	TryCreateDebugHpWidget();

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

void ALSEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyDebugHpWidget();

	Super::EndPlay(EndPlayReason);
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

void ALSEnemyCharacter::TryCreateDebugHpWidget()
{
	if (!bCreateDebugHpWidget || DebugHpWidgetInstance || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ALSPlayerControllerBase* LocalPlayerController = nullptr;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(It->Get());
		if (PlayerController && PlayerController->IsLocalPlayerController())
		{
			LocalPlayerController = PlayerController;
			break;
		}
	}

	if (!LocalPlayerController)
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ALSEnemyCharacter::TryCreateDebugHpWidget);
		return;
	}

	TSubclassOf<ULSHpDebugWidget> DebugHpWidgetClass = LocalPlayerController->GetDebugHpWidgetClass();
	if (!DebugHpWidgetClass)
	{
		return;
	}

	DebugHpWidgetInstance = CreateWidget<ULSHpDebugWidget>(LocalPlayerController, DebugHpWidgetClass);
	if (!DebugHpWidgetInstance)
	{
		return;
	}

	const int32 StackIndex = GNextMonsterHpDebugWidgetStackIndex++;
	DebugHpWidgetInstance->SetObservedCharacter(this);
	DebugHpWidgetInstance->AddToViewport();
	DebugHpWidgetInstance->SetPositionInViewport(
		DebugHpWidgetBasePosition + FVector2D(0.0f, DebugHpWidgetVerticalSpacing * StackIndex));
}

void ALSEnemyCharacter::DestroyDebugHpWidget()
{
	if (!DebugHpWidgetInstance)
	{
		return;
	}

	DebugHpWidgetInstance->RemoveFromParent();
	DebugHpWidgetInstance = nullptr;
}
