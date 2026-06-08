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
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LostSignal.h"
#include "Minimap/LSMinimapMarkerComponent.h"
#include "UI/Debug/LSHpDebugWidget.h"

namespace
{
	int32 GNextMonsterHpDebugWidgetStackIndex = 0;
	TWeakObjectPtr<UWorld> GMonsterHpDebugWidgetWorld;
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
	MinimapMarkerComponent = CreateDefaultSubobject<ULSMinimapMarkerComponent>(TEXT("MinimapMarkerComponent"));
	MinimapMarkerComponent->SetMarkerType(ELSMinimapMarkerType::Enemy);
	MinimapMarkerComponent->SetMarkerColor(FLinearColor(1.0f, 0.12f, 0.1f, 1.0f));
	MonsterAttributeSet = CreateDefaultSubobject<ULSCharacterAttributeSet>(TEXT("MonsterAttributeSet"));
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

	ApplyMonsterAttributes(*Row);
}

void ALSEnemyCharacter::ApplyMonsterAttributes(const FLSMonsterArchetypeRow& Row)
{
	if (!HasAuthority())
	{
		return;
	}

	ULSCombatAttributeSet* LocalCombatAttributeSet = GetCombatAttributeSet();
	if (!LocalCombatAttributeSet || !MonsterAttributeSet)
	{
		UE_LOG(LogLS, Warning, TEXT("%s: missing monster AttributeSet, monster attributes were not initialized."), *GetNameSafe(this));
		return;
	}

	const float MonsterHealth = FMath::Max(0.0f, Row.Monster_HP);
	LocalCombatAttributeSet->InitMaxHealth(MonsterHealth);
	LocalCombatAttributeSet->InitCurrentHealth(MonsterHealth);

	MonsterAttributeSet->InitAttack(FMath::Max(0.0f, Row.Monster_ATK));
	MonsterAttributeSet->InitDefence(FMath::Max(0.0f, Row.Monster_DEF));
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

	if (GMonsterHpDebugWidgetWorld.Get() != World)
	{
		GMonsterHpDebugWidgetWorld = World;
		GNextMonsterHpDebugWidgetStackIndex = 0;
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

	TSubclassOf<ULSHpDebugWidget> WidgetClassToUse = DebugHpWidgetClass;
	if (!WidgetClassToUse && LocalPlayerController)
	{
		WidgetClassToUse = LocalPlayerController->GetDebugHpWidgetClass();
	}

	if (!WidgetClassToUse)
	{
		if (!bWarnedMissingDebugHpWidgetClass)
		{
			bWarnedMissingDebugHpWidgetClass = true;
			UE_LOG(
				LogLS,
				Warning,
				TEXT("%s cannot create enemy HP debug widget because DebugHpWidgetClass is not set on enemy or local player controller."),
				*GetNameSafe(this));
		}
		return;
	}

	DebugHpWidgetInstance = CreateWidget<ULSHpDebugWidget>(LocalPlayerController, WidgetClassToUse);
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
