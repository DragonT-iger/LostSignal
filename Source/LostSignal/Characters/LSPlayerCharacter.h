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
class UUserWidget;
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

	void ApplyFacingRotation(const FRotator& NewRotation);

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	ULSAimComponent* GetAimComponent() const { return AimComponent; }

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	ULSPlayerCombatComponent* GetPlayerCombatComponent() const { return PlayerCombatComponent; }

	UFUNCTION(BlueprintPure, Category="LS/GAS")
	ULSCharacterAttributeSet* GetPlayerAttributeSet() const { return PlayerAttributeSet; }

	UFUNCTION(BlueprintPure, Category="LS/Input")
	UInputAction* GetInteractAction() const { return InteractAction; }

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> RunAction;

	// 상호작용 감지 최대 거리 (cm)
	UPROPERTY(EditAnywhere, Category="LS/Interact", meta=(ClampMin="50.0"))
	float MaxInteractRange = 250.0f;

	// 플레이어 정면과 오브젝트 방향의 내적 최소값 (0=90도, 0.5=60도, -1=뒤)
	UPROPERTY(EditAnywhere, Category="LS/Interact", meta=(ClampMin="0.0"))
	float InteractDistanceWeight = 0.55f;

	UPROPERTY(EditAnywhere, Category="LS/Interact", meta=(ClampMin="0.0"))
	float InteractAngleWeight = 0.45f;

	UPROPERTY(EditAnywhere, Category="LS/Interact", meta=(ClampMin="0.0"))
	float InteractScoreThreshold = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

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

	UPROPERTY(EditAnywhere, Category="LS/Camera")
	bool bEnableMouseCameraLead = false;

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
	bool bHasSentFacingRotation = false;
	TWeakObjectPtr<AActor> ActiveInventoryTarget;

	UPROPERTY(EditAnywhere, Category="LS/Combat", meta=(ClampMin="0.0"))
	float FacingSyncInterval = 0.05f;

	UPROPERTY(EditAnywhere, Category="LS/Combat", meta=(ClampMin="0.0"))
	float FacingSyncYawTolerance = 1.0f;

	UPROPERTY(VisibleInstanceOnly, Transient, Category="LS/UI")
	TObjectPtr<UUserWidget> InventoryWidget;

	float LastSentFacingYaw = 0.0f;
	float LastFacingSyncTime = 0.0f;
	FVector LastMoveWorldDirection = FVector::ZeroVector;

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
	void ShowInventoryWidgetForTarget(AActor* Target);
	void HideInventoryWidget();
	void UpdateInventoryWidgetDistance();
	bool IsInventoryWidgetOpen() const;

	void ApplyRunState(bool bNewIsRunning);
	bool ShouldSyncFacingRotation(float NewYaw) const;

	UFUNCTION(Server, Reliable)
	void ServerSetRunState(bool bNewIsRunning);

	UFUNCTION(Server, Reliable)
	void ServerRequestBasicAttack();

	UFUNCTION(Server, Reliable)
	void ServerRequestDash(FVector_NetQuantizeNormal DashDirection);

	UFUNCTION(Server, Unreliable)
	void ServerSyncFacingRotation(float NewYaw);

	UFUNCTION(Server, Reliable)
	void ServerRequestInteract(AActor* Target);

	FVector GetDashDirection() const;
	bool ResolveMouseWorldPoint(FVector& OutMouseWorldPoint) const;
};
