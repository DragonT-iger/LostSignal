// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/LSPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Characters/LSChipStatComponent.h"
#include "Characters/LSEquipmentStatComponent.h"
#include "Combat/LSAimComponent.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Combat/LSPlayerCombatComponent.h"
#include "Core/LSFarmingGameMode.h"
#include "Core/LSPlayerControllerBase.h"
#include "EnhancedInputComponent.h"
#include "AbilitySystemComponent.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "GAS/Effects/LSGE_StaminaChange.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "EngineUtils.h"
#include "Gameplay/LSInteractable.h"
#include "Gameplay/LSLobbyStorageActor.h"
#include "Gameplay/LSLootBox.h"
#include "Gameplay/LSNoiseEmitterComponent.h"
#include "InputActionValue.h"
#include "LostSignal.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "Skills/Preview/LSSkillPreviewComponent.h"
#include "Blueprint/UserWidget.h"
#include "UI/Inventory/LSInventoryWidget.h"
#include "UI/LSUILayer.h"
#include "Vision/LSCharacterLightingComponent.h"
#include "Vision/LSMPCVisionSourceComponent.h"
#include "Vision/LSPlayerXRayComponent.h"
#include "Vision/LSVisionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "UI/Survival/LSSurvivalOverheadWidget.h"

ALSPlayerCharacter::ALSPlayerCharacter()
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = TopDownCameraDistance;
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->SetRelativeRotation(FRotator(TopDownCameraPitch, TopDownCameraYaw, 0.0f));
	CameraBoom->TargetOffset = FVector::ZeroVector;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->TargetArmLength = 1100.0f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	MPCVisionSourceComponent = CreateDefaultSubobject<ULSMPCVisionSourceComponent>(TEXT("MPCVisionSourceComponent"));
	VisionComponent = CreateDefaultSubobject<ULSVisionComponent>(TEXT("VisionComponent"));
	PlayerXRayComponent = CreateDefaultSubobject<ULSPlayerXRayComponent>(TEXT("PlayerXRayComponent"));
	CharacterLightingComponent = CreateDefaultSubobject<ULSCharacterLightingComponent>(TEXT("CharacterLightingComponent"));
	NoiseEmitterComponent = CreateDefaultSubobject<ULSNoiseEmitterComponent>(TEXT("NoiseEmitterComponent"));
	AimComponent = CreateDefaultSubobject<ULSAimComponent>(TEXT("AimComponent"));
	PlayerCombatComponent = CreateDefaultSubobject<ULSPlayerCombatComponent>(TEXT("PlayerCombatComponent"));
	SkillPreviewComponent = CreateDefaultSubobject<ULSSkillPreviewComponent>(TEXT("SkillPreviewComponent"));
	PlayerSkillComponent = CreateDefaultSubobject<ULSPlayerSkillComponent>(TEXT("PlayerSkillComponent"));
	PlayerAttributeSet = CreateDefaultSubobject<ULSCharacterAttributeSet>(TEXT("PlayerAttributeSet"));
	ChipStatComponent = CreateDefaultSubobject<ULSChipStatComponent>(TEXT("ChipStatComponent"));
	EquipmentStatComponent = CreateDefaultSubobject<ULSEquipmentStatComponent>(TEXT("EquipmentStatComponent"));
	StaminaChangeEffectClass = ULSGE_StaminaChange::StaticClass();
	SurvivalOverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("SurvivalOverheadWidgetComponent"));
	SurvivalOverheadWidgetComponent->SetupAttachment(RootComponent);
	SurvivalOverheadWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SurvivalOverheadWidgetComponent->SetGenerateOverlapEvents(false);
	SurvivalOverheadWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	SurvivalOverheadWidgetComponent->SetDrawSize(SurvivalOverheadDrawSize);
	SurvivalOverheadWidgetComponent->SetRelativeLocation(SurvivalOverheadWidgetOffset);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = WalkSpeed;
		MovementComponent->MaxStepHeight = MaxAllowedStepHeight;
	}
}

void ALSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeSurvivalOverheadWidget();

	// 칩은 베이스 스탯 위에 얹는 GE이므로, 칩 적용 전에 파생 클래스의 베이스 어트리뷰트를 먼저 초기화한다.
	InitializeBaseAttributes();

	// Super::BeginPlay에서 ASC(InitAbilityActorInfo)가 준비된 뒤 칩 전투 스탯을 최초 적용한다.
	if (ChipStatComponent)
	{
		ChipStatComponent->RefreshChipStats(/*bRestoreFullHealth=*/true);
	}

	// 칩 적용 후 장비(무기/방어구) 전투 스탯을 얹는다. (장비 체력 보정까지 반영한 뒤 현재 체력을 최대치로 맞춘다)
	if (EquipmentStatComponent)
	{
		EquipmentStatComponent->RefreshEquipmentStats(/*bRestoreFullHealth=*/true);
	}

	// MoveSpeed 어트리뷰트 변화(칩·장비·신호 게이지)를 구독해 이동속도를 재계산하고, 현재 값으로 최초 반영한다.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(ULSCharacterAttributeSet::GetMoveSpeedAttribute())
			.AddUObject(this, &ALSPlayerCharacter::HandleMoveSpeedChanged);
	}
	RefreshMaxWalkSpeed();
}

void ALSPlayerCharacter::OnDeathStateChanged(bool bIsDead)
{
	Super::OnDeathStateChanged(bIsDead);

	// 훅은 모든 머신에서 호출되지만 레이드 종료 판정은 서버 권한에서만 시작한다.
	if (!bIsDead || !HasAuthority())
	{
		return;
	}

	// 파밍 레벨 판별을 GameMode 캐스트가 겸한다 — 로비 등에서는 캐스트 실패로 무시된다.
	if (ALSFarmingGameMode* FarmingGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALSFarmingGameMode>() : nullptr)
	{
		FarmingGameMode->OnPlayerDied();
	}
}

void ALSPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsFacingRotationLocked())
	{
		if (bIsRunning)
		{
			FaceMovementDirection(DeltaSeconds);
		}
		else if (AimComponent)
		{
			AimComponent->UpdateFacing(DeltaSeconds);
		}
	}

	UpdateInventoryWidgetDistance();
	UpdateActiveSkillPreview();
	UpdateRunStamina(DeltaSeconds);
	UpdateStaminaRecovery(DeltaSeconds);
	UpdateHealthRecovery(DeltaSeconds);
	UpdateClimbCeiling();
}

void ALSPlayerCharacter::UpdateClimbCeiling()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float CurrentBottomZ = GetActorLocation().Z - HalfHeight;

	// 시작(첫 착지) 위치의 Z를 기준 바닥(최저지면)으로 1회 고정
	if (!bBaseFloorZInitialized)
	{
		if (!MovementComponent->IsMovingOnGround())
		{
			return; // 아직 착지 전이면 기준 확정을 미룸
		}
		BaseFloorZ = CurrentBottomZ;
		bBaseFloorZInitialized = true;
	}

	const float Ceiling = BaseFloorZ + MaxAllowedStepHeight;

	// 1) 수직 턱(step): StepUp 게이트를 남은 높이만큼으로 조여 차단
	if (MovementComponent->IsMovingOnGround())
	{
		const float Remaining = Ceiling - CurrentBottomZ;
		MovementComponent->MaxStepHeight = FMath::Clamp(Remaining, 0.0f, MaxAllowedStepHeight);
	}

	// 2) 경사면 등 걷기로 올라가는 경로: 천장(기준 +N cm) 초과 시 하드 클램프
	if (CurrentBottomZ > Ceiling)
	{
		FVector NewLocation = GetActorLocation();
		NewLocation.Z = Ceiling + HalfHeight;
		SetActorLocation(NewLocation, false);

		if (MovementComponent->Velocity.Z > 0.0f)
		{
			MovementComponent->Velocity.Z = 0.0f;
		}
	}
}

void ALSPlayerCharacter::ApplyFacingRotation(const FRotator& NewRotation)
{
	const FRotator SanitizedRotation(0.0f, FRotator::NormalizeAxis(NewRotation.Yaw), 0.0f);
	SetActorRotation(SanitizedRotation);

	if (HasAuthority() || !IsLocallyControlled() || !ShouldSyncFacingRotation(SanitizedRotation.Yaw))
	{
		return;
	}

	LastSentFacingYaw = SanitizedRotation.Yaw;
	LastFacingSyncTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	bHasSentFacingRotation = true;
	ServerSyncFacingRotation(SanitizedRotation.Yaw);
}

void ALSPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogLS, Error, TEXT("%s missing EnhancedInputComponent."), *GetNameSafe(this));
		return;
	}

	if (MoveAction) { EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALSPlayerCharacter::Move); }
	if (RunAction)
	{
		EnhancedInput->BindAction(RunAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnRunStart);
		EnhancedInput->BindAction(RunAction, ETriggerEvent::Completed, this, &ALSPlayerCharacter::OnRunEnd);
	}

	if (AttackAction)
	{
		EnhancedInput->BindAction(AttackAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnAttack);
		EnhancedInput->BindAction(AttackAction, ETriggerEvent::Completed, this, &ALSPlayerCharacter::OnAttackReleased);
		// 매핑 컨텍스트 제거 등으로 Completed 대신 Canceled가 올 수 있어 홀드가 눌린 채 남지 않도록 함께 바인딩.
		EnhancedInput->BindAction(AttackAction, ETriggerEvent::Canceled, this, &ALSPlayerCharacter::OnAttackReleased);
	}
	if (DashAction) { EnhancedInput->BindAction(DashAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnDash); }
	if (SkillCancelAction) { EnhancedInput->BindAction(SkillCancelAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkillPreviewCancelInput); }
	// 스킬은 누름(Started)과 뗌(Completed/Canceled)을 함께 바인딩한다. 홀드-프리뷰 모드는 뗌에서 발동하고,
	// 매핑 컨텍스트 제거 등으로 Completed 대신 Canceled가 올 수 있어 릴리즈 처리를 함께 건다.
	if (Skill1Action)
	{
		EnhancedInput->BindAction(Skill1Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill1);
		EnhancedInput->BindAction(Skill1Action, ETriggerEvent::Completed, this, &ALSPlayerCharacter::OnSkill1Released);
		EnhancedInput->BindAction(Skill1Action, ETriggerEvent::Canceled, this, &ALSPlayerCharacter::OnSkill1Released);
	}
	if (Skill2Action)
	{
		EnhancedInput->BindAction(Skill2Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill2);
		EnhancedInput->BindAction(Skill2Action, ETriggerEvent::Completed, this, &ALSPlayerCharacter::OnSkill2Released);
		EnhancedInput->BindAction(Skill2Action, ETriggerEvent::Canceled, this, &ALSPlayerCharacter::OnSkill2Released);
	}
	if (Skill3Action)
	{
		EnhancedInput->BindAction(Skill3Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill3);
		EnhancedInput->BindAction(Skill3Action, ETriggerEvent::Completed, this, &ALSPlayerCharacter::OnSkill3Released);
		EnhancedInput->BindAction(Skill3Action, ETriggerEvent::Canceled, this, &ALSPlayerCharacter::OnSkill3Released);
	}
	if (Item1Action) { EnhancedInput->BindAction(Item1Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem1); }
	if (Item2Action) { EnhancedInput->BindAction(Item2Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem2); }
	if (Item3Action) { EnhancedInput->BindAction(Item3Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem3); }
	if (Item4Action) { EnhancedInput->BindAction(Item4Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem4); }
	if (Item5Action) { EnhancedInput->BindAction(Item5Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem5); }
	if (Item6Action) { EnhancedInput->BindAction(Item6Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem6); }
	if (InteractAction) { EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnInteract); }
	if (LootTransferAction) { EnhancedInput->BindAction(LootTransferAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnLootTransfer); }
	if (ToggleInventoryAction) { EnhancedInput->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnToggleInventory); }
	if (MenuAction) { EnhancedInput->BindAction(MenuAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnMenu); }
}

UInputAction* ALSPlayerCharacter::GetSkillInputAction(const ELSPlayerSkillSlot Slot) const
{
	switch (Slot)
	{
	case ELSPlayerSkillSlot::Skill1:
		return Skill1Action;
	case ELSPlayerSkillSlot::Skill2:
		return Skill2Action;
	case ELSPlayerSkillSlot::Skill3:
		return Skill3Action;
	case ELSPlayerSkillSlot::Dash:
		return DashAction;
	default:
		return nullptr;
	}
}

void ALSPlayerCharacter::OnAttack()
{
	if (IsInputBlocked() || IsModalUIBlockingInput())
	{
		return;
	}

	if (PlayerSkillComponent && PlayerSkillComponent->IsPreviewingSkill())
	{
		if (!ConfirmActiveSkillPreview())
		{
			UE_LOG(LogLS, Warning, TEXT("%s failed to confirm skill preview. Basic attack input was consumed."),
				*GetNameSafe(this));
		}

		return;
	}

	if (!PlayerCombatComponent)
	{
		return;
	}

	// 달리기 중 기본 공격이 들어오면 걷기로 전환한다. 공격 후 자동 복귀는 없다(달리기 키 재입력 필요).
	if (bIsRunning)
	{
		OnRunEnd();
	}

	PlayerCombatComponent->SetBasicAttackHeld(true);
	PlayerCombatComponent->RequestBasicAttack();
}

void ALSPlayerCharacter::OnAttackReleased()
{
	// 홀드 해제는 입력 블록 상태(모달 UI 등)와 무관하게 항상 서버에 전달한다. 눌린 적 없으면 dedup으로 no-op.
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->SetBasicAttackHeld(false);
	}
}

void ALSPlayerCharacter::OnDash()
{
	if (IsInputBlocked() || IsModalUIBlockingInput())
	{
		return;
	}

	if (CancelActiveSkillPreview())
	{
		return;
	}

	if (!PlayerCombatComponent)
	{
		return;
	}

	const FVector DashDirection = GetDashDirection();
	if (!HasAuthority())
	{
		if (!HasStamina(DashStaminaCost))
		{
			return;
		}

		bool bShouldExecuteImmediately = false;
		if (!PlayerCombatComponent->SubmitDashInput(DashDirection, bShouldExecuteImmediately))
		{
			return;
		}

		if (bShouldExecuteImmediately)
		{
			PlayerCombatComponent->PredictDashMovement(DashDirection);
		}

		ServerRequestDash(DashDirection);
		return;
	}

	if (HasStamina(DashStaminaCost) && PlayerCombatComponent->RequestDash(DashDirection))
	{
		TrySpendStamina(DashStaminaCost);
	}
}

void ALSPlayerCharacter::OnSkillPreviewCancelInput()
{
	CancelActiveSkillPreview();
}

void ALSPlayerCharacter::OnSkill1() { HandleSkillInputPressed(ELSPlayerSkillSlot::Skill1); }
void ALSPlayerCharacter::OnSkill2() { HandleSkillInputPressed(ELSPlayerSkillSlot::Skill2); }
void ALSPlayerCharacter::OnSkill3() { HandleSkillInputPressed(ELSPlayerSkillSlot::Skill3); }
void ALSPlayerCharacter::OnSkill1Released() { HandleSkillInputReleased(ELSPlayerSkillSlot::Skill1); }
void ALSPlayerCharacter::OnSkill2Released() { HandleSkillInputReleased(ELSPlayerSkillSlot::Skill2); }
void ALSPlayerCharacter::OnSkill3Released() { HandleSkillInputReleased(ELSPlayerSkillSlot::Skill3); }
void ALSPlayerCharacter::OnItem1() {}
void ALSPlayerCharacter::OnItem2() {}
void ALSPlayerCharacter::OnItem3() {}
void ALSPlayerCharacter::OnItem4() {}
void ALSPlayerCharacter::OnItem5() {}
void ALSPlayerCharacter::OnItem6() {}

void ALSPlayerCharacter::OnInteract()
{
	if (!IsLocallyControlled()) return;

	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetController()))
	{
		if (PlayerController->IsLobbyStorageWidgetOpen())
		{
			HideInventoryWidget();
			return;
		}
	}

	if (IsInventoryWidgetOpen())
	{
		HideInventoryWidget();
		return;
	}

	AActor* BestTarget = ResolveBestInteractTarget();
	if (BestTarget)
	{
		// 칩스테이션처럼 인벤토리 위젯을 거치지 않는 모달도 있으므로 상호작용 직전에 프리뷰를 정리한다.
		CancelActiveSkillPreview();

		ServerRequestInteract(BestTarget);

		if (BestTarget->IsA<ALSLootBox>() || BestTarget->IsA<ALSLobbyStorageActor>())
		{
			ShowInventoryWidgetForTarget(BestTarget);
		}
	}
}

AActor* ALSPlayerCharacter::ResolveBestInteractTarget()
{
	if (!IsLocallyControlled())
	{
		return nullptr;
	}

	const FVector MyLocation = GetActorLocation();

	FVector MouseWorldPoint = FVector::ZeroVector;
	if (!ResolveMouseWorldPoint(MouseWorldPoint))
	{
		return nullptr;
	}

	FVector AimDirection = MouseWorldPoint - MyLocation;
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		return nullptr;
	}

	AimDirection.Normalize();

	AActor* BestTarget = nullptr;
	float BestScore = -FLT_MAX;

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor == this || !Actor->Implements<ULSInteractable>()) continue;
		if (!ILSInteractable::Execute_CanInteract(Actor, this)) continue;

		const FVector ToActor = Actor->GetActorLocation() - MyLocation;
		const float DistSq = ToActor.SizeSquared();
		if (DistSq > FMath::Square(MaxInteractRange)) continue;

		FVector TargetDirection = ToActor;
		TargetDirection.Z = 0.0f;

		const float DistanceScore = 1.0f - FMath::Clamp(FMath::Sqrt(DistSq) / FMath::Max(MaxInteractRange, 1.0f), 0.0f, 1.0f);
		const float AngleScore = TargetDirection.IsNearlyZero()
			? 1.0f
			: (FMath::Clamp(FVector::DotProduct(AimDirection, TargetDirection.GetSafeNormal()), -1.0f, 1.0f) + 1.0f) * 0.5f;
		const float Score = (DistanceScore * InteractDistanceWeight) + (AngleScore * InteractAngleWeight);
		if (Score < InteractScoreThreshold) continue;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Actor;
		}
	}

	return BestTarget;
}

void ALSPlayerCharacter::OnLootTransfer()
{
	if (!IsLocallyControlled() || !IsInventoryWidgetOpen())
	{
		return;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetController());
	if (!PlayerController || !PlayerController->TransferHoveredLootDropItemToInventory())
	{
		return;
	}

	if (PlayerController->HasAuthority())
	{
		RebuildInventoryWidgetSlots();
	}
}

void ALSPlayerCharacter::OnToggleInventory()
{
	if (!IsLocallyControlled()) return;

	// 칩스테이션 등 모달이 열려 있으면 그걸 닫고, 닫을 게 없을 때만 단독 인벤토리를 연다.
	if (TryCloseOpenModalPanel())
	{
		return;
	}

	ShowInventoryWidgetStandalone();
}

void ALSPlayerCharacter::OnMenu()
{
	if (!IsLocallyControlled()) return;

	// ESC(메뉴/백): 열린 모달이 있으면 먼저 닫는다.
	if (TryCloseOpenModalPanel())
	{
		return;
	}

	// 닫을 UI가 없으면 설정 메뉴를 연다. (설정 UI 구현 시 이 자리에 연결)
}

bool ALSPlayerCharacter::TryCloseOpenModalPanel()
{
	// 칩스테이션은 인벤토리 위젯과 별개인 컨트롤러 소유 모달이라 먼저 확인한다.
	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetController()))
	{
		if (PlayerController->IsChipStationWidgetOpen())
		{
			PlayerController->HideChipStationWidget();
			return true;
		}
	}

	// 인벤토리(컨테이너로 열렸든 단독이든) — 함께 떠 있던 룻드랍/로비 창고도 같이 닫힌다.
	if (IsInventoryWidgetOpen())
	{
		HideInventoryWidget();
		return true;
	}

	return false;
}

void ALSPlayerCharacter::RebuildInventoryWidgetSlots()
{
	if (ULSInventoryWidget* LSInventoryWidget = Cast<ULSInventoryWidget>(InventoryWidget))
	{
		LSInventoryWidget->RebuildInventorySlots();
		LSInventoryWidget->RebuildConfirmedStorageSlots();
	}
}

bool ALSPlayerCharacter::ShowInventoryWidgetInternal(bool bShowStoreAllButton)
{
	if (!IsLocallyControlled())
	{
		return false;
	}

	if (!InventoryWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("InventoryWidgetClass is not set on %s."), *GetNameSafe(this));
		return false;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot show inventory widget because player controller is missing on %s."), *GetNameSafe(this));
		return false;
	}

	if (!InventoryWidget)
	{
		InventoryWidget = CreateWidget<UUserWidget>(PlayerController, InventoryWidgetClass);
		if (!InventoryWidget)
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create inventory widget on %s."), *GetNameSafe(this));
			return false;
		}
	}

	if (!InventoryWidget->IsInViewport())
	{
		InventoryWidget->AddToViewport(LSUILayer::ModalPanelInventory);
	}

	if (ULSInventoryWidget* LSInventoryWidget = Cast<ULSInventoryWidget>(InventoryWidget))
	{
		LSInventoryWidget->RebuildInventorySlots();
		LSInventoryWidget->RebuildConfirmedStorageSlots();
		LSInventoryWidget->SetStoreAllButtonVisible(bShowStoreAllButton);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("InventoryWidgetClass is not derived from ULSInventoryWidget on %s."), *GetNameSafe(this));
	}

	InventoryWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// 인벤토리가 뜨면 진행 중이던 스킬 프리뷰는 취소한다(모달 아래 프리뷰 잔류 방지).
	CancelActiveSkillPreview();

	if (ALSPlayerControllerBase* LSPlayerController = Cast<ALSPlayerControllerBase>(PlayerController))
	{
		LSPlayerController->UpdateBackgroundBlurVisibility();
	}

	return true;
}

void ALSPlayerCharacter::ShowInventoryWidgetForTarget(AActor* Target)
{
	if (!Target)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot show inventory widget because target is missing on %s."), *GetNameSafe(this));
		return;
	}

	if (!ShowInventoryWidgetInternal(Target->IsA<ALSLobbyStorageActor>()))
	{
		return;
	}

	ActiveInventoryTarget = Target;
	bIsStandaloneInventoryOpen = false;
}

void ALSPlayerCharacter::ShowInventoryWidgetStandalone()
{
	if (!IsLocallyControlled() || IsInventoryWidgetOpen())
	{
		return;
	}

	// 단독 인벤토리는 컨테이너가 없으므로 전부 보관 버튼을 숨긴다.
	if (!ShowInventoryWidgetInternal(false))
	{
		return;
	}

	ActiveInventoryTarget.Reset();
	bIsStandaloneInventoryOpen = true;
}

void ALSPlayerCharacter::HideInventoryWidget()
{
	if (InventoryWidget)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetController()))
	{
		PlayerController->HideLootDropWidget();
		PlayerController->HideLobbyStorageWidget();
		PlayerController->UpdateBackgroundBlurVisibility();
	}

	ActiveInventoryTarget.Reset();
	bIsStandaloneInventoryOpen = false;
}

void ALSPlayerCharacter::UpdateInventoryWidgetDistance()
{
	if (!IsInventoryWidgetOpen())
	{
		return;
	}

	// 단독(Tab) 인벤토리는 연동된 컨테이너가 없으므로 거리 기반 자동 닫기 대상이 아니다.
	if (bIsStandaloneInventoryOpen)
	{
		return;
	}

	AActor* Target = ActiveInventoryTarget.Get();
	if (!Target)
	{
		UE_LOG(LogLS, Warning, TEXT("Closing inventory widget because active inventory target is missing on %s."), *GetNameSafe(this));
		HideInventoryWidget();
		return;
	}

	const float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	if (Distance > MaxInteractRange)
	{
		HideInventoryWidget();
	}
}

bool ALSPlayerCharacter::IsInventoryWidgetOpen() const
{
	return InventoryWidget && InventoryWidget->IsVisible();
}

ELSSkillCastMode ALSPlayerCharacter::ResolveSlotCastMode(ELSPlayerSkillSlot Slot) const
{
	// 디버그 오버라이드 우선 적용을 위해 컴포넌트로 해석을 위임한다(설정 저장소 조회 포함).
	return PlayerSkillComponent ? PlayerSkillComponent->GetEffectiveCastMode(Slot) : ELSSkillCastMode::QuickCastWithIndicator;
}

void ALSPlayerCharacter::HandleSkillInputPressed(ELSPlayerSkillSlot Slot)
{
	// 즉시 퀵캐스트는 프리뷰 없이 바로 발동, 그 외 두 모드는 프리뷰 진입(확정 트리거만 다름).
	if (ResolveSlotCastMode(Slot) == ELSSkillCastMode::QuickCast)
	{
		ActivateSkillInstant(Slot);
		return;
	}

	BeginSkillPreview(Slot);
}

void ALSPlayerCharacter::HandleSkillInputReleased(ELSPlayerSkillSlot Slot)
{
	// 홀드-프리뷰 모드에서만 키를 뗄 때 커서 위치로 확정 발동한다.
	if (ResolveSlotCastMode(Slot) != ELSSkillCastMode::QuickCastWithIndicator)
	{
		return;
	}

	if (!IsLocallyControlled() || !PlayerSkillComponent || PlayerSkillComponent->GetActiveSlot() != Slot || !PlayerSkillComponent->IsPreviewingSkill())
	{
		return;
	}

	ConfirmActiveSkillPreview();
}

void ALSPlayerCharacter::BeginSkillPreview(ELSPlayerSkillSlot Slot)
{
	// 스킬 시전(몽타주) 중이거나 모달 UI가 열려 있으면 새 스킬 프리뷰 진입을 막는다(Skill1~3 공통 게이트).
	if (IsInputBlocked() || IsModalUIBlockingInput())
	{
		return;
	}

	if (!IsLocallyControlled() || !PlayerSkillComponent)
	{
		return;
	}

	if (PlayerSkillComponent->BeginSkillPreview(Slot))
	{
		UpdateActiveSkillPreview();
	}
}

void ALSPlayerCharacter::ActivateSkillInstant(ELSPlayerSkillSlot Slot)
{
	// 프리뷰 진입과 동일한 입력 게이트.
	if (IsInputBlocked() || IsModalUIBlockingInput())
	{
		return;
	}

	if (!IsLocallyControlled() || !PlayerSkillComponent)
	{
		return;
	}

	FVector MouseWorldPoint = FVector::ZeroVector;
	if (!ResolveMouseWorldPoint(MouseWorldPoint))
	{
		return;
	}

	FVector AimDirection = MouseWorldPoint - GetActorLocation();
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = GetActorForwardVector();
	}

	PlayerSkillComponent->ActivateSkillInstant(Slot, MouseWorldPoint, AimDirection.Rotation());
}

void ALSPlayerCharacter::UpdateActiveSkillPreview()
{
	if (!IsLocallyControlled() || !PlayerSkillComponent || !PlayerSkillComponent->IsPreviewingSkill())
	{
		return;
	}

	FVector MouseWorldPoint = FVector::ZeroVector;
	if (!ResolveMouseWorldPoint(MouseWorldPoint))
	{
		return;
	}

	FVector AimDirection = MouseWorldPoint - GetActorLocation();
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = GetActorForwardVector();
	}

	FLSSkillAreaPreviewSpec PreviewSpec;
	const bool bHasPreviewSpec = PlayerSkillComponent->GetActivePreviewSpec(PreviewSpec);
	FVector PreviewLocation =
		bHasPreviewSpec && PreviewSpec.LocationMode == ELSSkillPreviewLocationMode::CasterOrigin
			? GetActorLocation()
			: MouseWorldPoint;

	if (bHasPreviewSpec && !PreviewSpec.LocationOffset.IsNearlyZero())
	{
		const FVector Forward = AimDirection.GetSafeNormal2D();
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
		PreviewLocation += (Forward * PreviewSpec.LocationOffset.X) + (Right * PreviewSpec.LocationOffset.Y);
	}

	PlayerSkillComponent->UpdateActiveSkillPreview(PreviewLocation, AimDirection.Rotation());
}

bool ALSPlayerCharacter::ConfirmActiveSkillPreview()
{
	if (!IsLocallyControlled())
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot confirm skill preview because it is not locally controlled."),
			*GetNameSafe(this));
		return false;
	}

	if (!PlayerSkillComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot confirm skill preview because PlayerSkillComponent is missing."),
			*GetNameSafe(this));
		return false;
	}

	if (!PlayerSkillComponent->IsPreviewingSkill())
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot confirm skill preview because no skill preview is active."),
			*GetNameSafe(this));
		return false;
	}

	FVector MouseWorldPoint = FVector::ZeroVector;
	if (!ResolveMouseWorldPoint(MouseWorldPoint))
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot confirm skill preview because mouse world point could not be resolved."),
			*GetNameSafe(this));
		return false;
	}

	FVector AimDirection = MouseWorldPoint - GetActorLocation();
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = GetActorForwardVector();
	}

	return PlayerSkillComponent->ConfirmAnyActiveSkillPreview(MouseWorldPoint, AimDirection.Rotation());
}

bool ALSPlayerCharacter::CancelActiveSkillPreview()
{
	if (!IsLocallyControlled() || !PlayerSkillComponent || !PlayerSkillComponent->IsPreviewingSkill())
	{
		return false;
	}

	PlayerSkillComponent->CancelAnyActiveSkillPreview();
	return true;
}

void ALSPlayerCharacter::ServerRequestInteract_Implementation(AActor* Target)
{
	if (!Target || !Target->Implements<ULSInteractable>()) return;

	const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	if (Dist > MaxInteractRange * 1.5f) return;

	if (!ILSInteractable::Execute_CanInteract(Target, this)) return;

	ILSInteractable::Execute_Interact(Target, this);

	if (NoiseEmitterComponent)
	{
		NoiseEmitterComponent->EmitInteractNoise();
	}
}

void ALSPlayerCharacter::OnRunStart()
{
	if (!CanStartRunning())
	{
		ApplyRunState(false);
		return;
	}

	ApplyRunState(true);

	if (!HasAuthority())
	{
		ServerSetRunState(true);
	}
}

void ALSPlayerCharacter::OnRunEnd()
{
	ApplyRunState(false);

	if (!HasAuthority())
	{
		ServerSetRunState(false);
	}
}

bool ALSPlayerCharacter::IsInputBlocked() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC && ASC->HasMatchingGameplayTag(LSGameplayTags::State_InputBlocked);
}

bool ALSPlayerCharacter::IsModalUIBlockingInput() const
{
	const ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetController());
	return PlayerController && PlayerController->IsAnyModalPanelOpen();
}

bool ALSPlayerCharacter::IsFacingRotationLocked() const
{
	// 스킬 시전(LS.Combat.SkillCasting)은 전 구간 회전 잠금.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC && ASC->HasMatchingGameplayTag(LSGameplayTags::Combat_SkillCasting))
	{
		return true;
	}

	// 기본공격은 히트 판정 프레임(LSAN_PlayerMeleeHit)까지만 잠근다. 이후엔 다음 콤보 조준용 회전을 허용.
	return PlayerCombatComponent
		&& PlayerCombatComponent->IsAttackInProgress()
		&& !PlayerCombatComponent->IsBasicAttackHitConsumed();
}

void ALSPlayerCharacter::Move(const FInputActionValue& Value)
{
	// 스킬 몽타주 재생 중이면 입력 이동을 무시한다(루트모션 이동은 별도 경로라 영향 없음).
	if (IsInputBlocked())
	{
		LastMoveWorldDirection = FVector::ZeroVector;
		return;
	}

	const FVector2D Input = Value.Get<FVector2D>();

	FVector ForwardDirection = FollowCamera->GetForwardVector();
	ForwardDirection.Z = 0.0f;
	ForwardDirection.Normalize();

	FVector RightDirection = FollowCamera->GetRightVector();
	RightDirection.Z = 0.0f;
	RightDirection.Normalize();

	FVector MoveDirection = (RightDirection * Input.X) + (ForwardDirection * Input.Y);
	MoveDirection.Z = 0.0f;

	if (MoveDirection.IsNearlyZero())
	{
		LastMoveWorldDirection = FVector::ZeroVector;
		return;
	}

	LastMoveWorldDirection = MoveDirection.GetSafeNormal();
	AddMovementInput(LastMoveWorldDirection, FMath::Clamp(MoveDirection.Size(), 0.0f, 1.0f));
}

void ALSPlayerCharacter::FaceMovementDirection(float DeltaSeconds)
{
	FVector MoveDirection = GetVelocity();
	MoveDirection.Z = 0.0f;
	if (MoveDirection.IsNearlyZero())
	{
		return;
	}

	if (CameraBoom)
	{
		const FVector OffsetTarget = bEnableMouseCameraLead
			? MoveDirection.GetSafeNormal() * (MouseCameraLeadDistance * 0.6f)
			: FVector::ZeroVector;
		CameraBoom->TargetOffset = FMath::VInterpTo(CameraBoom->TargetOffset, OffsetTarget, DeltaSeconds, MouseCameraLeadInterpSpeed);
	}

	const FRotator TargetRotation = MoveDirection.Rotation();
	const FRotator NewRotation = FMath::RInterpTo(
		GetActorRotation(),
		FRotator(0.0f, TargetRotation.Yaw, 0.0f),
		DeltaSeconds,
		RunFacingInterpSpeed);

	ApplyFacingRotation(NewRotation);
}

bool ALSPlayerCharacter::ShouldSyncFacingRotation(float NewYaw) const
{
	if (!bHasSentFacingRotation)
	{
		return true;
	}

	if (!GetWorld())
	{
		return false;
	}

	const float TimeSinceLastSync = GetWorld()->GetTimeSeconds() - LastFacingSyncTime;
	if (TimeSinceLastSync < FacingSyncInterval)
	{
		return false;
	}

	const float YawDelta = FMath::Abs(FRotator::NormalizeAxis(NewYaw - LastSentFacingYaw));
	return YawDelta >= FacingSyncYawTolerance;
}

void ALSPlayerCharacter::ApplyRunState(bool bNewIsRunning)
{
	if (bNewIsRunning && !CanStartRunning())
	{
		bNewIsRunning = false;
	}

	bIsRunning = bNewIsRunning;

	RefreshMaxWalkSpeed();
}

void ALSPlayerCharacter::RefreshMaxWalkSpeed()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	// MoveSpeed 어트리뷰트 = 이동속도 배수(기본 1.0, 칩/장비로 가산). 0 이하 방지로 하한 클램프.
	const float Multiplier = PlayerAttributeSet ? FMath::Max(0.01f, PlayerAttributeSet->GetMoveSpeed()) : 1.0f;
	MovementComponent->MaxWalkSpeed = (bIsRunning ? RunSpeed : WalkSpeed) * Multiplier;
}

void ALSPlayerCharacter::HandleMoveSpeedChanged(const FOnAttributeChangeData& ChangeData)
{
	// 칩 장착·신호 게이지 변화 등으로 MoveSpeed 어트리뷰트가 바뀌면 이동속도를 즉시 재계산한다.
	RefreshMaxWalkSpeed();
}

bool ALSPlayerCharacter::CanStartRunning() const
{
	return HasStamina(KINDA_SMALL_NUMBER);
}

bool ALSPlayerCharacter::IsMovingForRunStaminaDrain() const
{
	FVector HorizontalVelocity = GetVelocity();
	HorizontalVelocity.Z = 0.0f;
	return !HorizontalVelocity.IsNearlyZero(1.0f);
}

void ALSPlayerCharacter::UpdateRunStamina(float DeltaSeconds)
{
	if (!HasAuthority() || !bIsRunning || RunStaminaDrainPerSecond <= 0.0f)
	{
		return;
	}

	if (!CanStartRunning())
	{
		ApplyRunState(false);
		ClientSetRunState(false);
		return;
	}

	if (!IsMovingForRunStaminaDrain())
	{
		return;
	}

	if (!TrySpendRunStamina(RunStaminaDrainPerSecond * DeltaSeconds))
	{
		ApplyRunState(false);
		ClientSetRunState(false);
	}
}

void ALSPlayerCharacter::UpdateStaminaRecovery(float DeltaSeconds)
{
	if (!HasAuthority() || !PlayerAttributeSet || StaminaRecoveryPerSecond <= 0.0f)
	{
		return;
	}

	if (bIsRunning || PlayerAttributeSet->GetCurrentStamina() >= PlayerAttributeSet->GetMaxStamina())
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() - LastStaminaSpendTime < StaminaRecoveryDelay)
	{
		return;
	}

	ApplyStaminaChange(StaminaRecoveryPerSecond * DeltaSeconds);
}

void ALSPlayerCharacter::UpdateHealthRecovery(float DeltaSeconds)
{
	if (!HasAuthority() || !PlayerAttributeSet)
	{
		return;
	}

	// Recovery 어트리뷰트 = 초당 회복 체력(HP/s). 칩/장비 합산으로 올라간다.
	const float RecoveryPerSecond = PlayerAttributeSet->GetRecovery();
	if (RecoveryPerSecond <= 0.0f)
	{
		return;
	}

	const ULSCombatAttributeSet* CombatSet = GetCombatAttributeSet();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!CombatSet || !ASC)
	{
		return;
	}

	// 사망 상태면 회복하지 않는다(음수/0 체력을 되살리지 않도록).
	if (const ULSCharacterCombatComponent* CombatComponent = GetCharacterCombatComponent())
	{
		if (CombatComponent->IsDead())
		{
			return;
		}
	}

	const float CurrentHealth = CombatSet->GetCurrentHealth();
	const float MaxHealth = CombatSet->GetMaxHealth();
	if (CurrentHealth >= MaxHealth)
	{
		return;
	}

	const float NewHealth = FMath::Min(CurrentHealth + RecoveryPerSecond * DeltaSeconds, MaxHealth);
	ASC->SetNumericAttributeBase(ULSCombatAttributeSet::GetCurrentHealthAttribute(), NewHealth);
}

bool ALSPlayerCharacter::HasStamina(float RequiredAmount) const
{
	return PlayerAttributeSet && PlayerAttributeSet->GetCurrentStamina() >= FMath::Max(RequiredAmount, 0.0f);
}

bool ALSPlayerCharacter::TrySpendStamina(float Amount)
{
	if (!HasAuthority() || Amount <= 0.0f || !HasStamina(Amount))
	{
		return false;
	}

	ApplyStaminaChange(-Amount);
	LastStaminaSpendTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	return true;
}

bool ALSPlayerCharacter::TrySpendRunStamina(const float Amount)
{
	if (!HasAuthority() || Amount <= 0.0f || !PlayerAttributeSet)
	{
		return false;
	}

	const float CurrentStamina = PlayerAttributeSet->GetCurrentStamina();
	const float SpendAmount = FMath::Min(CurrentStamina, Amount);
	if (SpendAmount <= 0.0f)
	{
		return false;
	}

	ApplyStaminaChange(-SpendAmount);
	LastStaminaSpendTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	return CurrentStamina > SpendAmount + KINDA_SMALL_NUMBER;
}

void ALSPlayerCharacter::ApplyStaminaChange(float Amount)
{
	if (!HasAuthority() || FMath::IsNearlyZero(Amount) || !StaminaChangeEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(StaminaChangeEffectClass, 1.0f, EffectContext);
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		UE_LOG(LogLS, Warning, TEXT("%s failed to create stamina change effect spec."), *GetNameSafe(this));
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(LSGameplayTags::Data_Stamina_Amount, Amount);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void ALSPlayerCharacter::InitializeSurvivalOverheadWidget()
{
	if (GetNetMode() == NM_DedicatedServer || !SurvivalOverheadWidgetComponent)
	{
		return;
	}

	SurvivalOverheadWidgetComponent->SetRelativeLocation(SurvivalOverheadWidgetOffset);
	SurvivalOverheadWidgetComponent->SetDrawSize(SurvivalOverheadDrawSize);

	if (SurvivalOverheadWidgetClass)
	{
		SurvivalOverheadWidgetComponent->SetWidgetClass(SurvivalOverheadWidgetClass);
	}

	if (!SurvivalOverheadWidgetComponent->GetWidgetClass())
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize survival overhead widget because SurvivalOverheadWidgetClass is not set."), *GetNameSafe(this));
		return;
	}

	SurvivalOverheadWidgetComponent->InitWidget();
	ULSSurvivalOverheadWidget* OverheadWidget = Cast<ULSSurvivalOverheadWidget>(SurvivalOverheadWidgetComponent->GetWidget());
	if (!OverheadWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("%s survival overhead widget is not derived from ULSSurvivalOverheadWidget."), *GetNameSafe(this));
		return;
	}

	OverheadWidget->InitializeSurvivalOverheadForCharacter(this);
}

void ALSPlayerCharacter::ServerSetRunState_Implementation(bool bNewIsRunning)
{
	ApplyRunState(bNewIsRunning);
	if (bNewIsRunning && !bIsRunning)
	{
		ClientSetRunState(false);
	}
}

void ALSPlayerCharacter::ClientSetRunState_Implementation(bool bNewIsRunning)
{
	ApplyRunState(bNewIsRunning);
}

void ALSPlayerCharacter::ServerRequestDash_Implementation(FVector_NetQuantizeNormal DashDirection)
{
	if (PlayerCombatComponent && HasStamina(DashStaminaCost) && PlayerCombatComponent->RequestDash(FVector(DashDirection)))
	{
		TrySpendStamina(DashStaminaCost);
	}
}

void ALSPlayerCharacter::ServerSyncFacingRotation_Implementation(float NewYaw)
{
	SetActorRotation(FRotator(0.0f, FRotator::NormalizeAxis(NewYaw), 0.0f));
}

FVector ALSPlayerCharacter::GetDashDirection() const
{
	FVector DashDirection = LastMoveWorldDirection;
	if (DashDirection.IsNearlyZero() && GetCharacterMovement())
	{
		DashDirection = GetCharacterMovement()->GetLastInputVector();
	}

	if (DashDirection.IsNearlyZero())
	{
		DashDirection = GetActorForwardVector();
	}

	DashDirection.Z = 0.0f;
	return DashDirection.IsNearlyZero() ? GetActorForwardVector() : DashDirection.GetSafeNormal();
}

bool ALSPlayerCharacter::ResolveMouseWorldPoint(FVector& OutMouseWorldPoint) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->IsLocalPlayerController())
	{
		return false;
	}

	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;
	if (!PlayerController->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return false;
	}

	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}

	const float TargetPlaneZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float RayDistance = (TargetPlaneZ - WorldOrigin.Z) / WorldDirection.Z;
	if (RayDistance < 0.0f)
	{
		return false;
	}

	OutMouseWorldPoint = WorldOrigin + (WorldDirection * RayDistance);
	return true;
}
