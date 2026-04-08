// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/LSCharacterBase.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ALSCharacterBase::ALSCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	// 컨트롤러 회전을 따르지 않음 — 마우스 방향 또는 AI가 직접 회전 제어
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 이동 방향으로 자동 회전하지 않음 — 플레이어는 마우스, 적은 AI가 회전 담당
	GetCharacterMovement()->bOrientRotationToMovement = false;
	//GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);

	// 메시 원점(발바닥)과 캡슐 원점(중심)의 차이 보정
	// Z: 캡슐 절반 높이(96)만큼 내림, Yaw: UE 기본 메시 방향 보정
	GetMesh()->SetRelativeLocationAndRotation(
		FVector(0.0f, 0.0f, -96.0f),
		FRotator(0.0f, -90.0f, 0.0f)
	);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	//GetCharacterMovement()->JumpZVelocity = 500.0f; // 점프는 사용하지 않음
	//GetCharacterMovement()->AirControl = 0.35f;
	//GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	//GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
	//GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;
	//GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
}
