// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/LSPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "GAS/LSGameplayTags.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "LostSignal.h"
#include "InputCoreTypes.h"
#include "Vision/LSMPCVisionSourceComponent.h"
#include "Vision/LSVisionComponent.h"

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
	CameraBoom->TargetArmLength = 1300;

	// 카메라 생성
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	MPCVisionSourceComponent = CreateDefaultSubobject<ULSMPCVisionSourceComponent>(TEXT("MPCVisionSourceComponent"));
	VisionComponent = CreateDefaultSubobject<ULSVisionComponent>(TEXT("VisionComponent"));
}

void ALSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay(); // LSCharacterBase::BeginPlay에서 InitAbilityActorInfo 호출

	// BP_PlayerCharacter Details에서 할당한 어빌리티 클래스를 부여
	if (DashAbilityClass)
	{
		GrantAbility(DashAbilityClass);
	}
}

void ALSPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FaceMouseCursor(DeltaSeconds);
}

void ALSPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogLS, Error, TEXT("'%s' Enhanced Input component를 찾지 못했습니다."), *GetNameSafe(this));
		return;
	}

	// 이동 (Triggered = 누르는 동안 매 프레임)
	if (MoveAction)    EIC->BindAction(MoveAction,    ETriggerEvent::Triggered, this, &ALSPlayerCharacter::Move);

	// 전투 / 스킬 (Started = 누른 순간 1회)
	if (AttackAction)  EIC->BindAction(AttackAction,  ETriggerEvent::Started, this, &ALSPlayerCharacter::OnAttack);
	if (DashAction)    EIC->BindAction(DashAction,    ETriggerEvent::Started, this, &ALSPlayerCharacter::OnDash);
	if (Skill1Action)  EIC->BindAction(Skill1Action,  ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill1);
	if (Skill2Action)  EIC->BindAction(Skill2Action,  ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill2);
	if (Skill3Action)  EIC->BindAction(Skill3Action,  ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill3);

	// 아이템 슬롯 (Started = 누른 순간 1회)
	if (Item1Action)   EIC->BindAction(Item1Action,   ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem1);
	if (Item2Action)   EIC->BindAction(Item2Action,   ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem2);
	if (Item3Action)   EIC->BindAction(Item3Action,   ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem3);
	if (Item4Action)   EIC->BindAction(Item4Action,   ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem4);
	if (Item5Action)   EIC->BindAction(Item5Action,   ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem5);
	if (Item6Action)   EIC->BindAction(Item6Action,   ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem6);

	// 상호작용
	if (InteractAction) EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnInteract);


	//테스트 코드
	//PlayerInputComponent->BindKey(EKeys::T, IE_Pressed, this, &ALSPlayerCharacter::Attack);

}

// ── 입력 핸들러 스텁 — 나중에 GAS 어빌리티 활성화로 교체 예정 ──────────

void ALSPlayerCharacter::OnAttack()  {}
void ALSPlayerCharacter::OnDash()
{
	if (AbilitySystemComponent)
	{
		// LS.Ability.Dash 태그가 달린 어빌리티를 발동
		// LS.State.Dodging 태그가 이미 있으면 LSGA_Dash의 ActivationBlockedTags가 차단
		AbilitySystemComponent->TryActivateAbilitiesByTag(
			FGameplayTagContainer(LSGameplayTags::Ability_Dash)
		);
	}
}
void ALSPlayerCharacter::OnSkill1()  {}
void ALSPlayerCharacter::OnSkill2()  {}
void ALSPlayerCharacter::OnSkill3()  {}
void ALSPlayerCharacter::OnItem1()   {}
void ALSPlayerCharacter::OnItem2()   {}
void ALSPlayerCharacter::OnItem3()   {}
void ALSPlayerCharacter::OnItem4()   {}
void ALSPlayerCharacter::OnItem5()   {}
void ALSPlayerCharacter::OnItem6()   {}
void ALSPlayerCharacter::OnInteract() {}


//void ALSPlayerCharacter::Attack()
//{
//	if (AttackMontage)
//	{
//		PlayAnimMontage(AttackMontage);
//	}
//}

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
