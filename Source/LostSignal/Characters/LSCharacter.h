// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LSCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

/**
 * Base top-down player character for LostSignal.
 * Camera asset references should be assigned in a derived Blueprint.
 */
UCLASS(Abstract)
class ALSCharacter : public ACharacter
{
	GENERATED_BODY()

private:

	/** Camera boom positioning the camera above the character. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Top-down gameplay camera. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> FollowCamera;

public:

	/** Constructor */
	ALSCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UCameraComponent* GetFollowCamera() const { return FollowCamera.Get(); }
	USpringArmComponent* GetCameraBoom() const { return CameraBoom.Get(); }
};
