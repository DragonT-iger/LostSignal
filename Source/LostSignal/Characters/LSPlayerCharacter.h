// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/LSCharacterBase.h"
#include "LSPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

/**
 * Abstract player character for LostSignal.
 * Top-down 쿼터뷰 카메라 + 마우스 방향 추적을 담당.
 * 실제 메시·에셋은 파생 Blueprint(BP_PlayerCharacter)에서 할당.
 *
 * AnimationTest의 ATP_ThirdPersonCharacter 기능을 이식한 클래스.
 */
UCLASS(Abstract)
class ALSPlayerCharacter : public ALSCharacterBase
{
	GENERATED_BODY()

	/** 카메라를 캐릭터 위에 위치시키는 붐. Unity의 카메라 피벗 오브젝트에 해당. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** 실제 렌더링 카메라. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> FollowCamera;

protected:

	/** 이동 Input Action (IA_Move). BP 또는 에디터에서 할당. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Top-down 카메라 피치 각도. */
	UPROPERTY(EditAnywhere, Category="Camera")
	float TopDownCameraPitch = -60.0f;

	/** Top-down 카메라 야우 각도. 약간 비틀어서 쿼터뷰 느낌. */
	UPROPERTY(EditAnywhere, Category="Camera")
	float TopDownCameraYaw = -45.0f;

	/** 카메라 붐 길이 (캐릭터~카메라 거리). */
	UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="0.0"))
	float TopDownCameraDistance = 900.0f;

	/** 마우스 방향으로 회전할 때 보간 속도. */
	UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="0.0"))
	float MouseFacingInterpSpeed = 15.0f;

	/** 마우스 위치 쪽으로 카메라가 이동하는 최대 거리. */
	UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="0.0"))
	float MouseCameraLeadDistance = 250.0f;

	/** 이 범위 안에서는 카메라 리드가 작동하지 않는 데드존 반경. */
	UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="0.0"))
	float MouseCameraLeadDeadZone = 120.0f;

	/** 카메라 리드 이동 보간 속도. */
	UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="0.0"))
	float MouseCameraLeadInterpSpeed = 8.0f;

public:

	ALSPlayerCharacter();

	virtual void Tick(float DeltaSeconds) override;

protected:

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:

	void Move(const FInputActionValue& Value);
	void FaceMouseCursor(float DeltaSeconds);
};
