// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Characters/LSCharacterBase.h"
#include "CoreMinimal.h"
#include "Skills/LSSkillTypes.h"
#include "LSPlayerCharacter.generated.h"

class UCameraComponent;
class UGameplayEffect;
class UInputAction;
class ULSAimComponent;
class ULSCharacterLightingComponent;
class ULSChipStatComponent;
class ULSEquipmentStatComponent;
class ULSMPCVisionSourceComponent;
class ULSNoiseEmitterComponent;
class ULSPlayerCombatComponent;
class ULSPlayerXRayComponent;
class ULSPlayerSkillComponent;
class ULSSkillPreviewComponent;
class ULSVisionComponent;
class ULSCharacterAttributeSet;
class ULSSurvivalOverheadWidget;
class UUserWidget;
class UWidgetComponent;
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

	// 사망 시 서버에서 FarmingGameMode에 레이드 종료(Dead)를 알린다. 파밍 외 레벨에서는 아무것도 하지 않는다.
	virtual void OnDeathStateChanged(bool bIsDead) override;

	void ApplyFacingRotation(const FRotator& NewRotation);
	void RebuildInventoryWidgetSlots();

	UFUNCTION(BlueprintPure, Category="LS/UI")
	bool IsInventoryWidgetOpen() const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	ULSAimComponent* GetAimComponent() const { return AimComponent; }

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	ULSPlayerCombatComponent* GetPlayerCombatComponent() const { return PlayerCombatComponent; }

	UFUNCTION(BlueprintPure, Category="LS/Noise")
	ULSNoiseEmitterComponent* GetNoiseEmitterComponent() const { return NoiseEmitterComponent; }

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	ULSPlayerSkillComponent* GetPlayerSkillComponent() const { return PlayerSkillComponent; }

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	ULSSkillPreviewComponent* GetSkillPreviewComponent() const { return SkillPreviewComponent; }

	UFUNCTION(BlueprintPure, Category="LS/GAS")
	ULSCharacterAttributeSet* GetPlayerAttributeSet() const { return PlayerAttributeSet; }

	UFUNCTION(BlueprintPure, Category="LS/Input")
	UInputAction* GetInteractAction() const { return InteractAction; }

	UInputAction* GetSkillInputAction(ELSPlayerSkillSlot Slot) const;

	UFUNCTION(BlueprintPure, Category="LS/Movement")
	bool IsRunning() const { return bIsRunning; }

	AActor* ResolveBestInteractTarget();

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// 파생 클래스가 칩 적용 전에 베이스 어트리뷰트를 초기화할 수 있는 훅. 기본 동작 없음.
	virtual void InitializeBaseAttributes() {}

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSCharacterLightingComponent> CharacterLightingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Noise", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSNoiseEmitterComponent> NoiseEmitterComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSAimComponent> AimComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSPlayerCombatComponent> PlayerCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Skill", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSSkillPreviewComponent> SkillPreviewComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Skill", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSPlayerSkillComponent> PlayerSkillComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/GAS", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSCharacterAttributeSet> PlayerAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Chip", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSChipStatComponent> ChipStatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Equipment", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULSEquipmentStatComponent> EquipmentStatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/UI", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UWidgetComponent> SurvivalOverheadWidgetComponent;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	TSubclassOf<ULSSurvivalOverheadWidget> SurvivalOverheadWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	FVector SurvivalOverheadWidgetOffset = FVector(0.0f, 0.0f, 120.0f);

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	FVector2D SurvivalOverheadDrawSize = FVector2D(160.0f, 48.0f);

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> SkillCancelAction;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Skill1Action;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Skill2Action;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> Skill3Action;

	UPROPERTY(EditAnywhere, Category = "LS/Input")
	TObjectPtr<UInputAction> Skill4Action;

	UPROPERTY(EditAnywhere, Category = "LS/Input")
	TObjectPtr<UInputAction> Ultimatection;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Input")
	TObjectPtr<UInputAction> LootTransferAction;

	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> RunAction;

	// Tab: 인벤토리 단독 열기/닫기 토글
	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> ToggleInventoryAction;

	// ESC(메뉴/백): 열린 UI가 있으면 닫고, 닫을 게 없으면 설정 메뉴를 연다
	UPROPERTY(EditAnywhere, Category="LS/Input")
	TObjectPtr<UInputAction> MenuAction;

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

	UPROPERTY(EditDefaultsOnly, Category="LS/Movement")
	TSubclassOf<UGameplayEffect> StaminaChangeEffectClass;

	UPROPERTY(EditAnywhere, Category="LS/Movement", meta=(ClampMin="0.0"))
	float RunStaminaDrainPerSecond = 5.0f;

	UPROPERTY(EditAnywhere, Category="LS/Movement", meta=(ClampMin="0.0"))
	float DashStaminaCost = 10.0f;

	UPROPERTY(EditAnywhere, Category="LS/Movement", meta=(ClampMin="0.0"))
	float StaminaRecoveryDelay = 2.0f;

	UPROPERTY(EditAnywhere, Category="LS/Movement", meta=(ClampMin="0.0"))
	float StaminaRecoveryPerSecond = 20.0f;

	UPROPERTY(EditAnywhere, Category="LS/Movement", meta=(ClampMin="0.0"))
	float MaxAllowedStepHeight = 50.0f;

	// 시작(첫 착지) 위치 Z로 1회 고정되는 기준 바닥. climb 천장 = BaseFloorZ + MaxAllowedStepHeight
	float BaseFloorZ = 0.0f;
	bool bBaseFloorZInitialized = false;

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
	// 컨테이너 없이 Tab으로 연 단독 인벤토리인지. 단독이면 거리 기반 자동 닫기를 건너뛴다.
	bool bIsStandaloneInventoryOpen = false;
	TWeakObjectPtr<AActor> ActiveInventoryTarget;

	UPROPERTY(EditAnywhere, Category="LS/Combat", meta=(ClampMin="0.0"))
	float FacingSyncInterval = 0.05f;

	UPROPERTY(EditAnywhere, Category="LS/Combat", meta=(ClampMin="0.0"))
	float FacingSyncYawTolerance = 1.0f;

	UPROPERTY(VisibleInstanceOnly, Transient, Category="LS/UI")
	TObjectPtr<UUserWidget> InventoryWidget;

	float LastSentFacingYaw = 0.0f;
	float LastFacingSyncTime = 0.0f;
	float LastStaminaSpendTime = -FLT_MAX;
	FVector LastMoveWorldDirection = FVector::ZeroVector;

	// 스킬 몽타주 재생 중(LS.State.InputBlocked 태그) 전투 입력을 무시할지. 이동·대시·스킬 입력 공통 게이트.
	bool IsInputBlocked() const;

	// 모달 UI(인벤토리/룻드랍/로비창고/칩스테이션) 열림 중 전투 입력(공격·스킬·대시)을 무시할지. 이동은 허용.
	bool IsModalUIBlockingInput() const;

	// 마우스 조준·이동 방향 회전을 잠글지. 스킬 시전은 전 구간, 기본공격은 히트 판정 프레임까지 잠근다.
	bool IsFacingRotationLocked() const;

	void Move(const FInputActionValue& Value);
	void FaceMovementDirection(float DeltaSeconds);
	void OnAttack();
	void OnAttackReleased();
	void OnDash();
	void OnSkillPreviewCancelInput();
	void OnRunStart();
	void OnRunEnd();
	void OnSkill1();
	void OnSkill2();
	void OnSkill3();
	void OnSkill4();
	void OnUltimate();
	void OnSkill1Released();
	void OnSkill2Released();
	void OnSkill3Released();
	void OnSkill4Released();
	void OnUltimateReleased();
	void OnItem1();
	void OnItem2();
	void OnItem3();
	void OnItem4();
	void OnItem5();
	void OnItem6();
	void OnInteract();
	void OnLootTransfer();
	void OnToggleInventory();
	void OnMenu();
	// 열려 있는 모달 패널(칩스테이션/인벤토리)을 우선순위대로 하나 닫는다. 닫았으면 true.
	bool TryCloseOpenModalPanel();
	bool ShowInventoryWidgetInternal(bool bShowStoreAllButton);
	void ShowInventoryWidgetForTarget(AActor* Target);
	void ShowInventoryWidgetStandalone();
	void HideInventoryWidget();
	void UpdateInventoryWidgetDistance();

	// 슬롯 캐스트 모드에 따라 누름/뗌 입력을 프리뷰 진입·즉발·릴리즈 확정으로 분기한다.
	ELSSkillCastMode ResolveSlotCastMode(ELSPlayerSkillSlot Slot) const;
	void HandleSkillInputPressed(ELSPlayerSkillSlot Slot);
	void HandleSkillInputReleased(ELSPlayerSkillSlot Slot);
	void BeginSkillPreview(ELSPlayerSkillSlot Slot);
	void ActivateSkillInstant(ELSPlayerSkillSlot Slot);
	void UpdateActiveSkillPreview();
	bool ConfirmActiveSkillPreview();
	bool CancelActiveSkillPreview();

	void ApplyRunState(bool bNewIsRunning);
	bool CanStartRunning() const;
	bool IsMovingForRunStaminaDrain() const;
	void UpdateRunStamina(float DeltaSeconds);
	void UpdateStaminaRecovery(float DeltaSeconds);
	void UpdateHealthRecovery(float DeltaSeconds);
	void RefreshMaxWalkSpeed();
	void HandleMoveSpeedChanged(const struct FOnAttributeChangeData& ChangeData);
	void UpdateClimbCeiling();
	bool HasStamina(float RequiredAmount) const;
	bool TrySpendStamina(float Amount);
	bool TrySpendRunStamina(float Amount);
	void ApplyStaminaChange(float Amount);
	void InitializeSurvivalOverheadWidget();
	bool ShouldSyncFacingRotation(float NewYaw) const;

	UFUNCTION(Server, Reliable)
	void ServerSetRunState(bool bNewIsRunning);

	UFUNCTION(Client, Reliable)
	void ClientSetRunState(bool bNewIsRunning);

	UFUNCTION(Server, Reliable)
	void ServerRequestDash(FVector_NetQuantizeNormal DashDirection);

	UFUNCTION(Server, Unreliable)
	void ServerSyncFacingRotation(float NewYaw);

	UFUNCTION(Server, Reliable)
	void ServerRequestInteract(AActor* Target);

	FVector GetDashDirection() const;
	bool ResolveMouseWorldPoint(FVector& OutMouseWorldPoint) const;
};
