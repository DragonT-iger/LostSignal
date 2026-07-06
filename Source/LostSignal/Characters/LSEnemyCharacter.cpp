// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/LSEnemyCharacter.h"

#include "AI/LSAIController.h"
#include "AI/LSMonsterCombatComponent.h"
#include "AI/LSMonsterSenseComponent.h"
#include "Animation/AnimInstance.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSMonsterArchetypeRow.h"
#include "Engine/DataTable.h"
#include "GAS/Abilities/LSGA_MonsterAction.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gameplay/LSNoiseEmitterComponent.h"
#include "LostSignal.h"
#include "Minimap/LSMinimapMarkerComponent.h"
#include "Skills/Preview/LSSkillPreviewComponent.h"
#include "UI/Debug/LSHpDebugWidget.h"
#include "UI/Combat/LSEnemyHealthBarComponent.h"
#include "Vision/LSVisionTargetComponent.h"

namespace
{
	int32 GNextMonsterHpDebugWidgetStackIndex = 0;
	TWeakObjectPtr<UWorld> GMonsterHpDebugWidgetWorld;
}

ALSEnemyCharacter::ALSEnemyCharacter()
{
	AIControllerClass = ALSAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	MonsterActionAbilityClass = ULSGA_MonsterAction::StaticClass();

	// Player uses its own facing logic, but AI should rotate from controller focus while chasing targets.
	// bUseControllerDesiredRotation is the CharacterMovement-side switch that turns SetFocus() yaw into body rotation.
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);

	// 화면 밖 몬스터는 포즈/본 갱신 생략(몽타주는 계속 틱 → 공격 노티파이/데미지 판정 유지).
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;

	MonsterSenseComponent = CreateDefaultSubobject<ULSMonsterSenseComponent>(TEXT("MonsterSenseComponent"));
	MonsterCombatComponent = CreateDefaultSubobject<ULSMonsterCombatComponent>(TEXT("MonsterCombatComponent"));
	MinimapMarkerComponent = CreateDefaultSubobject<ULSMinimapMarkerComponent>(TEXT("MinimapMarkerComponent"));
	NoiseEmitterComponent = CreateDefaultSubobject<ULSNoiseEmitterComponent>(TEXT("NoiseEmitterComponent"));
	VisionTargetComponent = CreateDefaultSubobject<ULSVisionTargetComponent>(TEXT("VisionTargetComponent"));
	HealthBarComponent = CreateDefaultSubobject<ULSEnemyHealthBarComponent>(TEXT("HealthBarComponent"));
	HealthBarComponent->SetupAttachment(GetRootComponent());
	NoiseEmitterComponent->SetNotifyMonsterSense(false);
	MinimapMarkerComponent->SetMarkerType(ELSMinimapMarkerType::Enemy);
	MinimapMarkerComponent->SetMarkerColor(FLinearColor(1.0f, 0.12f, 0.1f, 1.0f));
	MonsterAttributeSet = CreateDefaultSubobject<ULSCharacterAttributeSet>(TEXT("MonsterAttributeSet"));
	SkillPreviewComponent = CreateDefaultSubobject<ULSSkillPreviewComponent>(TEXT("SkillPreviewComponent"));
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

	// 이동 Task가 속도를 바꾸기 전(StateTree 시작 전) base 속도를 캡처해 단일 출처로 보관.
	if (const UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		DefaultMaxWalkSpeed = MovementComponent->MaxWalkSpeed;
	}

	InitializeMonsterArchetype();
	TryCreateDebugHpWidget();

	if (HasAuthority())
	{
		if (MonsterActionAbilityClass)
		{
			// StateTree can only request abilities that already exist on the monster ASC.
			GrantAbility(MonsterActionAbilityClass);
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

void ALSEnemyCharacter::OnDeathStateChanged(bool bIsDead)
{
	Super::OnDeathStateChanged(bIsDead);

	// Dead는 몬스터의 터미널 상태(부활 없음)라 사망 진입만 처리. 부활이 생기면 여기서 프로파일 복원 필요.
	if (!bIsDead)
	{
		return;
	}

	// 시체가 이동을 막지 않고, 추가 타격/타겟팅 대상에서 빠지도록 캡슐·메시 콜리전 모두 해제.
	// 메시는 계속 보이며 사망 몽타주가 재생된다(가시성은 건드리지 않음).
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
		UE_LOG(LogLS, Warning, TEXT("%s: Monster archetype row missing, DataTable-driven monster values were not applied."), *GetNameSafe(this));
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
	MonsterAttributeSet->InitArmorPenetrationResistance(FMath::Max(0.0f, Row.Monster_ArmorPen_Resist));
	MonsterAttributeSet->InitCritChanceResistance(FMath::Max(0.0f, Row.Monster_Crit_Resist));
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
