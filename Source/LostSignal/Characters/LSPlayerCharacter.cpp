// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/LSPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "LostSignal.h"

ALSPlayerCharacter::ALSPlayerCharacter()
{
	// 카메라 붐 생성 — SpringArm은 Unity의 카메라 피벗 오브젝트에 해당
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

	// 카메라 생성
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void ALSPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FaceMouseCursor(DeltaSeconds);
}

void ALSPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALSPlayerCharacter::Move);
		}
	}
	else
	{
		UE_LOG(LogLS, Error, TEXT("'%s' Enhanced Input component를 찾지 못했습니다."), *GetNameSafe(this));
	}
}

void ALSPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();

	// 카메라 기준 전후좌우 방향 계산 (Z 무시, 평면 이동)
	FVector ForwardDir = FollowCamera->GetForwardVector();
	ForwardDir.Z = 0.0f;
	ForwardDir.Normalize();

	FVector RightDir = FollowCamera->GetRightVector();
	RightDir.Z = 0.0f;
	RightDir.Normalize();

	AddMovementInput(RightDir, Input.X);
	AddMovementInput(ForwardDir, Input.Y);
}

void ALSPlayerCharacter::FaceMouseCursor(float DeltaSeconds)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// 마우스 2D 위치 → 월드 레이 변환 (Unity의 Camera.ScreenPointToRay에 해당)
	FVector WorldOrigin;
	FVector WorldDirection;
	if (!PC->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return;
	}

	// Z축이 0에 가까우면 수평 레이 — 교점 계산 불안정하므로 스킵
	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return;
	}

	// 레이와 캐릭터 높이 평면의 교점 계산
	const float TraceDistance = (GetActorLocation().Z - WorldOrigin.Z) / WorldDirection.Z;
	if (TraceDistance <= 0.0f)
	{
		return;
	}

	const FVector CursorWorldPoint = WorldOrigin + (WorldDirection * TraceDistance);
	FVector LookDir = CursorWorldPoint - GetActorLocation();
	LookDir.Z = 0.0f;

	if (LookDir.IsNearlyZero())
	{
		return;
	}

	// 마우스 방향으로 카메라 오프셋 이동 (데드존 안쪽은 이동 없음)
	if (CameraBoom)
	{
		FVector OffsetTarget = FVector::ZeroVector;
		const float CursorDist = LookDir.Length();
		if (CursorDist > MouseCameraLeadDeadZone)
		{
			OffsetTarget = LookDir.GetSafeNormal() * FMath::Min(CursorDist - MouseCameraLeadDeadZone, MouseCameraLeadDistance);
		}
		CameraBoom->TargetOffset = FMath::VInterpTo(CameraBoom->TargetOffset, OffsetTarget, DeltaSeconds, MouseCameraLeadInterpSpeed);
	}

	// 마우스 방향으로 캐릭터 회전 보간
	const FRotator TargetRot = LookDir.Rotation();
	const FRotator NewRot = FMath::RInterpTo(GetActorRotation(), FRotator(0.0f, TargetRot.Yaw, 0.0f), DeltaSeconds, MouseFacingInterpSpeed);
	SetActorRotation(NewRot);
}
