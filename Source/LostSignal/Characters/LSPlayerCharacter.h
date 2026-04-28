// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/LSCharacterBase.h"
#include "LSPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UGameplayAbility;
class ULSMPCVisionSourceComponent;
class ULSVisionComponent;
class ULSPlayerXRayComponent;
struct FInputActionValue;

/**
 * Abstract player character for LostSignal.
 * Top-down 쿼터뷰 카메라 + 마우스 방향 추적을 담당.
 * 실제 메시·에셋은 파생 Blueprint(BP_PlayerCharacter)에서 할당.
 */
UCLASS(Abstract)
class ALSPlayerCharacter : public ALSCharacterBase
{
	GENERATED_BODY()

	/** 카메라를 캐릭터 위에 위치시키는 붐. Unity의 카메라 피벗 오브젝트에 해당. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** 실제 렌더링 카메라. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSMPCVisionSourceComponent> MPCVisionSourceComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSVisionComponent> VisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSPlayerXRayComponent> PlayerXRayComponent;

protected:

	// ── Input Actions ────────────────────────────────────
	// BP_PlayerCharacter Details 패널에서 각 슬롯에 에셋 할당

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> MoveAction;      // IA_Move      (WASD)

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> AttackAction;    // IA_Attack    (마우스 좌클릭)

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> DashAction;      // IA_Dash      (Space)

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Skill1Action;    // IA_Skill1    (Q)

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Skill2Action;    // IA_Skill2    (E)

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Skill3Action;    // IA_Skill3    (R)

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Item1Action;     // IA_Item1     (1)

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Item2Action;     // IA_Item2     (2)

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Item3Action;     // IA_Item3     (3)

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Item4Action;     // IA_Item4     (4)

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Item5Action;     // IA_Item5     (5)

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Item6Action;     // IA_Item6     (6)

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> InteractAction;  // IA_Interact  (F 등)

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> RunAction;       // IA_Run       (Shift)

	// ── 이동 파라미터 ────────────────────────────────────

	UPROPERTY(EditAnywhere, Category="LS/Movement", meta=(ClampMin="0.0"))
	float WalkSpeed = 300.0f;

	UPROPERTY(EditAnywhere, Category="LS/Movement", meta=(ClampMin="0.0"))
	float RunSpeed = 600.0f;

	// ── 카메라 파라미터 ──────────────────────────────────

	/** Top-down 카메라 피치 각도. */
	UPROPERTY(EditAnywhere, Category="LS/Camera")
	float TopDownCameraPitch = -60.0f;

	/** Top-down 카메라 야우 각도. 약간 비틀어서 쿼터뷰 느낌. */
	UPROPERTY(EditAnywhere, Category="LS/Camera")
	float TopDownCameraYaw = -45.0f;

	/** 카메라 붐 길이 (캐릭터~카메라 거리). */
	UPROPERTY(EditAnywhere, Category="LS/Camera", meta=(ClampMin="0.0"))
	float TopDownCameraDistance = 900.0f;

	/** 마우스 방향으로 회전할 때 보간 속도. */
	UPROPERTY(EditAnywhere, Category="LS/Camera", meta=(ClampMin="0.0"))
	float MouseFacingInterpSpeed = 15.0f;

	/** 달리는 동안 이동 방향으로 천천히 돌아가는 보간 속도. */
	UPROPERTY(EditAnywhere, Category="LS/Camera", meta=(ClampMin="0.0"))
	float RunFacingInterpSpeed = 4.0f;

	/** 마우스 위치 쪽으로 카메라가 이동하는 최대 거리. */
	UPROPERTY(EditAnywhere, Category="LS/Camera", meta=(ClampMin="0.0"))
	float MouseCameraLeadDistance = 250.0f;

	/** 이 범위 안에서는 카메라 리드가 작동하지 않는 데드존 반경. */
	UPROPERTY(EditAnywhere, Category="LS/Camera", meta=(ClampMin="0.0"))
	float MouseCameraLeadDeadZone = 120.0f;

	/** 카메라 리드 이동 보간 속도. */
	UPROPERTY(EditAnywhere, Category="LS/Camera", meta=(ClampMin="0.0"))
	float MouseCameraLeadInterpSpeed = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "LS/Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	/** 대쉬 어빌리티 클래스. C++ 생성자에서 ULSGA_Dash로 초기화됨 */
	UPROPERTY(VisibleDefaultsOnly, Category="LS/GAS|Abilities")
	TSubclassOf<UGameplayAbility> DashAbilityClass;

public:

	//void Attack();

	ALSPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:

	bool bIsRunning = false;

	void Move(const FInputActionValue& Value);
	void FaceMouseCursor(float DeltaSeconds);
	void FaceMovementDirection(float DeltaSeconds);

	// ── 입력 핸들러 ──
	void OnAttack();
	void OnDash();
	void OnRunStart();
	void OnRunEnd();
	void OnSkill1();
	void OnSkill2();
	void OnSkill3();
	void OnItem1();
	void OnItem2();
	void OnItem3();
	void OnItem4();
	void OnItem5();
	void OnItem6();
	void OnInteract();
};
