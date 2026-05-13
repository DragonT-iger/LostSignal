// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/LSPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Combat/LSAimComponent.h"
#include "Combat/LSPlayerCombatComponent.h"
#include "Core/LSPlayerControllerBase.h"
#include "EnhancedInputComponent.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "EngineUtils.h"
#include "Gameplay/LSInteractable.h"
#include "Gameplay/LSLootBox.h"
#include "InputActionValue.h"
#include "LostSignal.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "Skills/Preview/LSSkillPreviewComponent.h"
#include "Blueprint/UserWidget.h"
#include "UI/Inventory/LSInventoryWidget.h"
#include "Vision/LSMPCVisionSourceComponent.h"
#include "Vision/LSPlayerXRayComponent.h"
#include "Vision/LSVisionComponent.h"
#include "Components/CapsuleComponent.h"

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
	AimComponent = CreateDefaultSubobject<ULSAimComponent>(TEXT("AimComponent"));
	PlayerCombatComponent = CreateDefaultSubobject<ULSPlayerCombatComponent>(TEXT("PlayerCombatComponent"));
	SkillPreviewComponent = CreateDefaultSubobject<ULSSkillPreviewComponent>(TEXT("SkillPreviewComponent"));
	PlayerSkillComponent = CreateDefaultSubobject<ULSPlayerSkillComponent>(TEXT("PlayerSkillComponent"));
	PlayerAttributeSet = CreateDefaultSubobject<ULSCharacterAttributeSet>(TEXT("PlayerAttributeSet"));

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ALSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ALSPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsRunning)
	{
		FaceMovementDirection(DeltaSeconds);
	}
	else if (AimComponent)
	{
		AimComponent->UpdateFacing(DeltaSeconds);
	}

	UpdateInventoryWidgetDistance();
	UpdateActiveSkillPreview();
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

	if (AttackAction) { EnhancedInput->BindAction(AttackAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnAttack); }
	if (DashAction) { EnhancedInput->BindAction(DashAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnDash); }
	if (SkillCancelAction) { EnhancedInput->BindAction(SkillCancelAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkillPreviewCancelInput); }
	if (Skill1Action)
	{
		EnhancedInput->BindAction(Skill1Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill1);
	}
	if (Skill2Action)
	{
		EnhancedInput->BindAction(Skill2Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill2);
	}
	if (Skill3Action)
	{
		EnhancedInput->BindAction(Skill3Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill3);
	}
	if (Skill4Action)
	{
		EnhancedInput->BindAction(Skill4Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill4);
	}
	if (Ultimatection)
	{
		EnhancedInput->BindAction(Ultimatection, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnUltimate);
	}
	if (Item1Action) { EnhancedInput->BindAction(Item1Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem1); }
	if (Item2Action) { EnhancedInput->BindAction(Item2Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem2); }
	if (Item3Action) { EnhancedInput->BindAction(Item3Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem3); }
	if (Item4Action) { EnhancedInput->BindAction(Item4Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem4); }
	if (Item5Action) { EnhancedInput->BindAction(Item5Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem5); }
	if (Item6Action) { EnhancedInput->BindAction(Item6Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem6); }
	if (InteractAction) { EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnInteract); }
	if (LootTransferAction) { EnhancedInput->BindAction(LootTransferAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnLootTransfer); }
}

void ALSPlayerCharacter::OnAttack()
{
	if (ConfirmActiveSkillPreview())
	{
		return;
	}

	if (!PlayerCombatComponent)
	{
		return;
	}

	if (!HasAuthority())
	{
		PlayerCombatComponent->RequestBasicAttack();
		ServerRequestBasicAttack();
		return;
	}

	PlayerCombatComponent->RequestBasicAttack();
}

void ALSPlayerCharacter::OnDash()
{
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

	PlayerCombatComponent->RequestDash(DashDirection);
}

void ALSPlayerCharacter::OnSkillPreviewCancelInput()
{
	CancelActiveSkillPreview();
}

void ALSPlayerCharacter::OnSkill1() { BeginSkillPreview(ELSPlayerSkillSlot::Skill1); }
void ALSPlayerCharacter::OnSkill2() { BeginSkillPreview(ELSPlayerSkillSlot::Skill2); }
void ALSPlayerCharacter::OnSkill3() { BeginSkillPreview(ELSPlayerSkillSlot::Skill3); }
void ALSPlayerCharacter::OnSkill4() { BeginSkillPreview(ELSPlayerSkillSlot::Skill4); }
void ALSPlayerCharacter::OnUltimate() { BeginSkillPreview(ELSPlayerSkillSlot::Ultimate); }
void ALSPlayerCharacter::OnItem1() {}
void ALSPlayerCharacter::OnItem2() {}
void ALSPlayerCharacter::OnItem3() {}
void ALSPlayerCharacter::OnItem4() {}
void ALSPlayerCharacter::OnItem5() {}
void ALSPlayerCharacter::OnItem6() {}

void ALSPlayerCharacter::OnInteract()
{
	if (!IsLocallyControlled()) return;

	if (IsInventoryWidgetOpen())
	{
		HideInventoryWidget();
		return;
	}

	const FVector MyLocation = GetActorLocation();

	FVector MouseWorldPoint = FVector::ZeroVector;
	if (!ResolveMouseWorldPoint(MouseWorldPoint))
	{
		return;
	}

	FVector AimDirection = MouseWorldPoint - MyLocation;
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		return;
	}

	AimDirection.Normalize();

	AActor* BestTarget = nullptr;
	float BestScore = -FLT_MAX;

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor == this || !Actor->Implements<ULSInteractable>()) continue;

		const FVector ToActor = Actor->GetActorLocation() - MyLocation;
		const float DistSq = ToActor.SizeSquared();
		if (DistSq > FMath::Square(MaxInteractRange)) continue;

		FVector TargetDirection = ToActor;
		TargetDirection.Z = 0.0f;
		if (TargetDirection.IsNearlyZero()) continue;

		const float DistanceScore = 1.0f - FMath::Clamp(FMath::Sqrt(DistSq) / FMath::Max(MaxInteractRange, 1.0f), 0.0f, 1.0f);
		const float Dot = FVector::DotProduct(AimDirection, TargetDirection.GetSafeNormal());
		const float AngleScore = (FMath::Clamp(Dot, -1.0f, 1.0f) + 1.0f) * 0.5f;
		const float Score = (DistanceScore * InteractDistanceWeight) + (AngleScore * InteractAngleWeight);
		if (Score < InteractScoreThreshold) continue;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Actor;
		}
	}

	if (BestTarget)
	{
		ServerRequestInteract(BestTarget);

		if (BestTarget->IsA<ALSLootBox>())
		{
			ShowInventoryWidgetForTarget(BestTarget);
		}
	}
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

	RebuildInventoryWidgetSlots();
}

void ALSPlayerCharacter::RebuildInventoryWidgetSlots()
{
	if (ULSInventoryWidget* LSInventoryWidget = Cast<ULSInventoryWidget>(InventoryWidget))
	{
		LSInventoryWidget->RebuildInventorySlots();
		LSInventoryWidget->RebuildConfirmedStorageSlots();
	}
}

void ALSPlayerCharacter::ShowInventoryWidgetForTarget(AActor* Target)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	if (!Target)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot show inventory widget because target is missing on %s."), *GetNameSafe(this));
		return;
	}

	if (!InventoryWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("InventoryWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot show inventory widget because player controller is missing on %s."), *GetNameSafe(this));
		return;
	}

	if (!InventoryWidget)
	{
		InventoryWidget = CreateWidget<UUserWidget>(PlayerController, InventoryWidgetClass);
		if (!InventoryWidget)
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create inventory widget on %s."), *GetNameSafe(this));
			return;
		}
	}

	if (!InventoryWidget->IsInViewport())
	{
		InventoryWidget->AddToViewport();
	}

	if (ULSInventoryWidget* LSInventoryWidget = Cast<ULSInventoryWidget>(InventoryWidget))
	{
		LSInventoryWidget->RebuildInventorySlots();
		LSInventoryWidget->RebuildConfirmedStorageSlots();
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("InventoryWidgetClass is not derived from ULSInventoryWidget on %s."), *GetNameSafe(this));
	}

	InventoryWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ActiveInventoryTarget = Target;
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
	}

	ActiveInventoryTarget.Reset();
}

void ALSPlayerCharacter::UpdateInventoryWidgetDistance()
{
	if (!IsInventoryWidgetOpen())
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

void ALSPlayerCharacter::BeginSkillPreview(ELSPlayerSkillSlot Slot)
{
	if (!IsLocallyControlled() || !PlayerSkillComponent)
	{
		return;
	}

	if (PlayerSkillComponent->BeginSkillPreview(Slot))
	{
		UpdateActiveSkillPreview();
	}
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
	if (!IsLocallyControlled() || !PlayerSkillComponent || !PlayerSkillComponent->IsPreviewingSkill())
	{
		return false;
	}

	FVector MouseWorldPoint = FVector::ZeroVector;
	if (!ResolveMouseWorldPoint(MouseWorldPoint))
	{
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
}

void ALSPlayerCharacter::OnRunStart()
{
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

void ALSPlayerCharacter::Move(const FInputActionValue& Value)
{
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
	bIsRunning = bNewIsRunning;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = bIsRunning ? RunSpeed : WalkSpeed;
	}
}

void ALSPlayerCharacter::ServerSetRunState_Implementation(bool bNewIsRunning)
{
	ApplyRunState(bNewIsRunning);
}

void ALSPlayerCharacter::ServerRequestBasicAttack_Implementation()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->RequestBasicAttack();
	}
}

void ALSPlayerCharacter::ServerRequestDash_Implementation(FVector_NetQuantizeNormal DashDirection)
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->RequestDash(FVector(DashDirection));
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
