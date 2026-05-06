// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/LSPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Combat/LSAimComponent.h"
#include "Combat/LSPlayerCombatComponent.h"
#include "EnhancedInputComponent.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "LostSignal.h"
#include "Vision/LSMPCVisionSourceComponent.h"
#include "Vision/LSPlayerXRayComponent.h"
#include "Vision/LSVisionComponent.h"

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
	if (Skill1Action) { EnhancedInput->BindAction(Skill1Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill1); }
	if (Skill2Action) { EnhancedInput->BindAction(Skill2Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill2); }
	if (Skill3Action) { EnhancedInput->BindAction(Skill3Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill3); }
	if (Item1Action) { EnhancedInput->BindAction(Item1Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem1); }
	if (Item2Action) { EnhancedInput->BindAction(Item2Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem2); }
	if (Item3Action) { EnhancedInput->BindAction(Item3Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem3); }
	if (Item4Action) { EnhancedInput->BindAction(Item4Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem4); }
	if (Item5Action) { EnhancedInput->BindAction(Item5Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem5); }
	if (Item6Action) { EnhancedInput->BindAction(Item6Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem6); }
	if (InteractAction) { EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnInteract); }

	PlayerInputComponent->BindKey(EKeys::T, IE_Pressed, this, &ALSPlayerCharacter::OnAttack);
}

void ALSPlayerCharacter::OnAttack()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->RequestBasicAttack();
	}
}

void ALSPlayerCharacter::OnDash()
{
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->RequestDash();
	}
}

void ALSPlayerCharacter::OnSkill1() {}
void ALSPlayerCharacter::OnSkill2() {}
void ALSPlayerCharacter::OnSkill3() {}
void ALSPlayerCharacter::OnItem1() {}
void ALSPlayerCharacter::OnItem2() {}
void ALSPlayerCharacter::OnItem3() {}
void ALSPlayerCharacter::OnItem4() {}
void ALSPlayerCharacter::OnItem5() {}
void ALSPlayerCharacter::OnItem6() {}
void ALSPlayerCharacter::OnInteract() {}

void ALSPlayerCharacter::OnRunStart()
{
	bIsRunning = true;
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}

void ALSPlayerCharacter::OnRunEnd()
{
	bIsRunning = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
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

	AddMovementInput(RightDirection, Input.X);
	AddMovementInput(ForwardDirection, Input.Y);
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
		const FVector OffsetTarget = MoveDirection.GetSafeNormal() * (MouseCameraLeadDistance * 0.6f);
		CameraBoom->TargetOffset = FMath::VInterpTo(CameraBoom->TargetOffset, OffsetTarget, DeltaSeconds, MouseCameraLeadInterpSpeed);
	}

	const FRotator TargetRotation = MoveDirection.Rotation();
	const FRotator NewRotation = FMath::RInterpTo(
		GetActorRotation(),
		FRotator(0.0f, TargetRotation.Yaw, 0.0f),
		DeltaSeconds,
		RunFacingInterpSpeed);

	SetActorRotation(NewRotation);
}
