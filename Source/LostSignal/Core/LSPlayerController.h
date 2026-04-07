// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LSPlayerController.generated.h"

class UNiagaraSystem;
class UInputAction;
class UInputMappingContext;
class UPathFollowingComponent;

/**
 * Base PlayerController for LostSignal.
 * Handles top-down click-to-move input and default mapping setup.
 */
UCLASS(Abstract)
class ALSPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** Component used for moving along a NavMesh path. */
	UPROPERTY(VisibleDefaultsOnly, Category="AI")
	TObjectPtr<UPathFollowingComponent> PathFollowingComponent;

	/** Time threshold used to detect short click input. */
	UPROPERTY(EditAnywhere, Category="Input")
	float ShortPressThreshold = 0.25f;

	/** FX class spawned when a short click move is confirmed. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UNiagaraSystem> FXCursor;

	/** Default input mapping context for gameplay. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Mouse click move action. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SetDestinationClickAction;

	/** Touch move action. */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SetDestinationTouchAction;

	/** True while the controller is processing mouse-driven movement input. */
	uint32 bMoveToMouseCursor : 1;

	/** True while the current input path uses touch. */
	uint32 bIsTouch : 1;

	/** Cached movement destination in world space. */
	FVector CachedDestination = FVector::ZeroVector;

	/** Current press duration used for click versus hold behavior. */
	float FollowTime = 0.0f;

public:

	ALSPlayerController();

protected:

	virtual void SetupInputComponent() override;

	void OnInputStarted();
	void OnSetDestinationTriggered();
	void OnSetDestinationReleased();
	void OnTouchTriggered();
	void OnTouchReleased();
	void UpdateCachedDestination();
};
