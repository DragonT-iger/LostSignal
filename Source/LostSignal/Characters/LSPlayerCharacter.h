// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Characters/LSCharacterBase.h"
#include "CoreMinimal.h"
#include "LSPlayerCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class ULSAimComponent;
class ULSMPCVisionSourceComponent;
class ULSPlayerCombatComponent;
class ULSPlayerXRayComponent;
class ULSVisionComponent;
class ULSCharacterAttributeSet;
class USpringArmComponent;
struct FInputActionValue;

UCLASS(Abstract)
class LOSTSIGNAL_API ALSPlayerCharacter : public ALSCharacterBase
{
	GENERATED_BODY()

public:
	ALSPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	ULSAimComponent* GetAimComponent() const { return AimComponent; }

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	ULSPlayerCombatComponent* GetPlayerCombatComponent() const { return PlayerCombatComponent; }

	UFUNCTION(BlueprintPure, Category="LS/GAS")
	ULSCharacterAttributeSet* GetPlayerAttributeSet() const { return PlayerAttributeSet; }

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSMPCVisionSourceComponent> MPCVisionSourceComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSVisionComponent> VisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSPlayerXRayComponent> PlayerXRayComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSAimComponent> AimComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSPlayerCombatComponent> PlayerCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/GAS", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSCharacterAttributeSet> PlayerAttributeSet;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Skill1Action;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Skill2Action;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Skill3Action;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Item1Action;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Item2Action;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Item3Action;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Item4Action;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Item5Action;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Item6Action;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> RunAction;

	UPROPERTY(EditAnywhere, Category="LS/Movement", meta=(ClampMin="0.0"))
	float WalkSpeed = 300.0f;

	UPROPERTY(EditAnywhere, Category="LS/Movement", meta=(ClampMin="0.0"))
	float RunSpeed = 600.0f;

	UPROPERTY(EditAnywhere, Category="LS/Camera")
	float TopDownCameraPitch = -60.0f;

	UPROPERTY(EditAnywhere, Category="LS/Camera")
	float TopDownCameraYaw = -45.0f;

	UPROPERTY(EditAnywhere, Category="LS/Camera", meta=(ClampMin="0.0"))
	float TopDownCameraDistance = 900.0f;

	UPROPERTY(EditAnywhere, Category="LS/Camera", meta=(ClampMin="0.0"))
	float RunFacingInterpSpeed = 4.0f;

	UPROPERTY(EditAnywhere, Category="LS/Camera", meta=(ClampMin="0.0"))
	float MouseCameraLeadDistance = 250.0f;

	UPROPERTY(EditAnywhere, Category="LS/Camera", meta=(ClampMin="0.0"))
	float MouseCameraLeadDeadZone = 120.0f;

	UPROPERTY(EditAnywhere, Category="LS/Camera", meta=(ClampMin="0.0"))
	float MouseCameraLeadInterpSpeed = 8.0f;

private:
	bool bIsRunning = false;

	void Move(const FInputActionValue& Value);
	void FaceMovementDirection(float DeltaSeconds);
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
