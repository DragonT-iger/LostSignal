// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/LSPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Characters/Enemys/LSEnemyCharacter.h"
#include "Characters/LSChipStatComponent.h"
#include "Characters/LSEquipmentStatComponent.h"
#include "Combat/LSAimComponent.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Combat/LSPlayerCombatComponent.h"
#include "Core/LSFarmingGameMode.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSConsumableRow.h"
#include "Data/LSGameDataSubsystem.h"
#include "Engine/GameInstance.h"
#include "Inventory/LSCraftingUtils.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "Session/LSSaveSubsystem.h"
#include "TimerManager.h"
#include "EnhancedInputComponent.h"
#include "AbilitySystemComponent.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "GAS/Effects/LSGE_StaminaChange.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "EngineUtils.h"
#include "Gameplay/LSInteractable.h"
#include "Gameplay/LSLobbyStorageActor.h"
#include "Gameplay/LSLootBox.h"
#include "Gameplay/LSNoiseEmitterComponent.h"
#include "InputActionValue.h"
#include "LostSignal.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "Skills/LSSkillAreaTypes.h"
#include "Skills/Preview/LSSkillPreviewComponent.h"
#include "Blueprint/UserWidget.h"
#include "UI/Inventory/LSInventoryWidget.h"
#include "UI/LSUILayer.h"
#include "Vision/LSCharacterLightingComponent.h"
#include "Vision/LSMPCVisionSourceComponent.h"
#include "Vision/LSPlayerXRayComponent.h"
#include "Vision/LSVisionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "UI/Interact/LSDistanceMarkerComponent.h"
#include "UI/Survival/LSSurvivalOverheadWidget.h"

ALSPlayerCharacter::ALSPlayerCharacter()
{
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
	CameraBoom->TargetArmLength = 1100.0f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	MPCVisionSourceComponent = CreateDefaultSubobject<ULSMPCVisionSourceComponent>(TEXT("MPCVisionSourceComponent"));
	VisionComponent = CreateDefaultSubobject<ULSVisionComponent>(TEXT("VisionComponent"));
	PlayerXRayComponent = CreateDefaultSubobject<ULSPlayerXRayComponent>(TEXT("PlayerXRayComponent"));
	CharacterLightingComponent = CreateDefaultSubobject<ULSCharacterLightingComponent>(TEXT("CharacterLightingComponent"));
	NoiseEmitterComponent = CreateDefaultSubobject<ULSNoiseEmitterComponent>(TEXT("NoiseEmitterComponent"));
	AimComponent = CreateDefaultSubobject<ULSAimComponent>(TEXT("AimComponent"));
	PlayerCombatComponent = CreateDefaultSubobject<ULSPlayerCombatComponent>(TEXT("PlayerCombatComponent"));
	SkillPreviewComponent = CreateDefaultSubobject<ULSSkillPreviewComponent>(TEXT("SkillPreviewComponent"));
	PlayerSkillComponent = CreateDefaultSubobject<ULSPlayerSkillComponent>(TEXT("PlayerSkillComponent"));
	PlayerAttributeSet = CreateDefaultSubobject<ULSCharacterAttributeSet>(TEXT("PlayerAttributeSet"));
	ChipStatComponent = CreateDefaultSubobject<ULSChipStatComponent>(TEXT("ChipStatComponent"));
	EquipmentStatComponent = CreateDefaultSubobject<ULSEquipmentStatComponent>(TEXT("EquipmentStatComponent"));
	StaminaChangeEffectClass = ULSGE_StaminaChange::StaticClass();
	SurvivalOverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("SurvivalOverheadWidgetComponent"));
	SurvivalOverheadWidgetComponent->SetupAttachment(RootComponent);
	SurvivalOverheadWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SurvivalOverheadWidgetComponent->SetGenerateOverlapEvents(false);
	SurvivalOverheadWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	SurvivalOverheadWidgetComponent->SetDrawSize(SurvivalOverheadDrawSize);
	SurvivalOverheadWidgetComponent->SetRelativeLocation(SurvivalOverheadWidgetOffset);

	MarkerActivationSphere = CreateDefaultSubobject<USphereComponent>(TEXT("MarkerActivationSphere"));
	MarkerActivationSphere->SetupAttachment(RootComponent);
	MarkerActivationSphere->InitSphereRadius(MarkerActivationRadius);
	MarkerActivationSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 로컬 폰에서만 BeginPlay/빙의 시 켠다
	MarkerActivationSphere->SetCollisionObjectType(ECC_WorldDynamic);
	MarkerActivationSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	MarkerActivationSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap); // InteractMarker 채널만 감지
	MarkerActivationSphere->SetGenerateOverlapEvents(false);
	MarkerActivationSphere->CanCharacterStepUpOn = ECB_No;

	WeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMeshComponent"));
	WeaponMeshComponent->SetupAttachment(GetMesh(), WeaponSocketName);
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 외형 전용, 히트 판정은 전투 컴포넌트가 담당
	WeaponMeshComponent->SetRenderCustomDepth(true); // 캐릭터 메쉬와 동일하게 커스텀 뎁스 스텐실 1 사용
	WeaponMeshComponent->SetCustomDepthStencilValue(1);

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = WalkSpeed;
		MovementComponent->MaxStepHeight = MaxAllowedStepHeight;
	}
}

void ALSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitializeSurvivalOverheadWidget();

	// 생성자 SetupAttachment는 멤버 기본값 시점에 소켓을 굽는다. BP에서 바꾼 WeaponSocketName을
	// 반영하려면 실제 값으로 다시 붙인다. 소켓이 없으면 여기서 경고를 남긴다(런타임 1회).
	AttachWeaponMeshToSocket();
	if (WeaponMeshComponent && GetMesh() && !GetMesh()->DoesSocketExist(WeaponSocketName))
	{
		UE_LOG(LogLS, Warning, TEXT("%s: 무기 소켓 '%s'가 스켈레톤에 없음 — 무기 위치 어긋남"),
			*GetNameSafe(this), *WeaponSocketName.ToString());
	}

	// 칩은 베이스 스탯 위에 얹는 GE이므로, 칩 적용 전에 파생 클래스의 베이스 어트리뷰트를 먼저 초기화한다.
	InitializeBaseAttributes();

	// Super::BeginPlay에서 ASC(InitAbilityActorInfo)가 준비된 뒤 칩 전투 스탯을 최초 적용한다.
	if (ChipStatComponent)
	{
		ChipStatComponent->RefreshChipStats(/*bRestoreFullHealth=*/true);
	}

	// 칩 적용 후 장비(무기/방어구) 전투 스탯을 얹는다. (장비 체력 보정까지 반영한 뒤 현재 체력을 최대치로 맞춘다)
	if (EquipmentStatComponent)
	{
		EquipmentStatComponent->RefreshEquipmentStats(/*bRestoreFullHealth=*/true);
	}

	// MoveSpeed 어트리뷰트 변화(칩·장비·신호 게이지)를 구독해 이동속도를 재계산하고, 현재 값으로 최초 반영한다.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(ULSCharacterAttributeSet::GetMoveSpeedAttribute())
			.AddUObject(this, &ALSPlayerCharacter::HandleMoveSpeedChanged);
	}
	RefreshMaxWalkSpeed();

	if (MarkerActivationSphere)
	{
		MarkerActivationSphere->OnComponentBeginOverlap.AddDynamic(this, &ALSPlayerCharacter::OnMarkerActivationBeginOverlap);
		MarkerActivationSphere->OnComponentEndOverlap.AddDynamic(this, &ALSPlayerCharacter::OnMarkerActivationEndOverlap);
		MarkerActivationSphere->SetSphereRadius(MarkerActivationRadius);
		UpdateMarkerActivationEnabled();
	}
}

void ALSPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	UpdateMarkerActivationEnabled();
}

void ALSPlayerCharacter::UpdateMarkerActivationEnabled()
{
	if (!MarkerActivationSphere)
	{
		return;
	}

	// 로컬 폰만 마커를 켠다. 시뮬레이션 프록시(타 플레이어)에서 켜지면 그 주변 마커가 잘못 뜬다.
	const bool bEnable = IsLocallyControlled();
	MarkerActivationSphere->SetGenerateOverlapEvents(bEnable);
	MarkerActivationSphere->SetCollisionEnabled(bEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	if (bEnable)
	{
		MarkerActivationSphere->UpdateOverlaps();
		ActivateOverlappingMarkers();
	}
}

void ALSPlayerCharacter::ActivateOverlappingMarkers()
{
	if (!MarkerActivationSphere)
	{
		return;
	}

	// 스피어를 켜는 시점에 이미 겹쳐 있던 마커는 BeginOverlap이 (델리게이트 바인딩 전에 소진돼) 안 온다.
	// 스폰 시 이미 반경 안이던 박스가 여기 해당하므로, 현재 겹친 마커를 직접 활성화해 유실을 막는다.
	TArray<UPrimitiveComponent*> OverlappingComponents;
	MarkerActivationSphere->GetOverlappingComponents(OverlappingComponents);
	for (UPrimitiveComponent* OverlappingComponent : OverlappingComponents)
	{
		if (ULSDistanceMarkerComponent* Marker = Cast<ULSDistanceMarkerComponent>(OverlappingComponent))
		{
			Marker->SetActivatedByProximity(true);
		}
	}
}

void ALSPlayerCharacter::OnMarkerActivationBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ULSDistanceMarkerComponent* Marker = Cast<ULSDistanceMarkerComponent>(OtherComp))
	{
		Marker->SetActivatedByProximity(true);
	}
}

void ALSPlayerCharacter::OnMarkerActivationEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ULSDistanceMarkerComponent* Marker = Cast<ULSDistanceMarkerComponent>(OtherComp))
	{
		Marker->SetActivatedByProximity(false);
	}
}

void ALSPlayerCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터에서 BP를 컴파일/이동할 때마다 실행된다. 여기서 붙여야 뷰포트에서 소켓에 붙어 보여 오프셋을 잡을 수 있다.
	AttachWeaponMeshToSocket();
}

void ALSPlayerCharacter::AttachWeaponMeshToSocket()
{
	if (!WeaponMeshComponent || !GetMesh() || !GetMesh()->DoesSocketExist(WeaponSocketName))
	{
		return;
	}

	WeaponMeshComponent->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::KeepRelativeTransform,
		WeaponSocketName);
}

void ALSPlayerCharacter::OnDeathStateChanged(bool bIsDead)
{
	Super::OnDeathStateChanged(bIsDead);

	// 훅은 모든 머신에서 호출되지만 레이드 종료 판정은 서버 권한에서만 시작한다.
	if (!bIsDead || !HasAuthority())
	{
		return;
	}

	// 파밍 레벨 판별을 GameMode 캐스트가 겸한다 — 로비 등에서는 캐스트 실패로 무시된다.
	if (ALSFarmingGameMode* FarmingGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALSFarmingGameMode>() : nullptr)
	{
		FarmingGameMode->OnPlayerDied();
	}
}

void ALSPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsFacingRotationLocked())
	{
		if (bIsRunning)
		{
			FaceMovementDirection(DeltaSeconds);
		}
		else if (AimComponent)
		{
			AimComponent->UpdateFacing(DeltaSeconds);
		}
	}

	UpdateInventoryWidgetDistance();
	UpdateActiveSkillPreview();
	UpdateThrowAim();
	UpdateRunStamina(DeltaSeconds);
	UpdateStaminaRecovery(DeltaSeconds);
	UpdateHealthRecovery(DeltaSeconds);
	UpdateClimbCeiling();
}

void ALSPlayerCharacter::UpdateClimbCeiling()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float CurrentBottomZ = GetActorLocation().Z - HalfHeight;

	// 시작(첫 착지) 위치의 Z를 기준 바닥(최저지면)으로 1회 고정
	if (!bBaseFloorZInitialized)
	{
		if (!MovementComponent->IsMovingOnGround())
		{
			return; // 아직 착지 전이면 기준 확정을 미룸
		}
		BaseFloorZ = CurrentBottomZ;
		bBaseFloorZInitialized = true;
	}

	const float Ceiling = BaseFloorZ + MaxAllowedStepHeight;

	// 1) 수직 턱(step): StepUp 게이트를 남은 높이만큼으로 조여 차단
	if (MovementComponent->IsMovingOnGround())
	{
		const float Remaining = Ceiling - CurrentBottomZ;
		MovementComponent->MaxStepHeight = FMath::Clamp(Remaining, 0.0f, MaxAllowedStepHeight);
	}

	// 2) 경사면 등 걷기로 올라가는 경로: 천장(기준 +N cm) 초과 시 하드 클램프
	if (CurrentBottomZ > Ceiling)
	{
		FVector NewLocation = GetActorLocation();
		NewLocation.Z = Ceiling + HalfHeight;
		SetActorLocation(NewLocation, false);

		if (MovementComponent->Velocity.Z > 0.0f)
		{
			MovementComponent->Velocity.Z = 0.0f;
		}
	}
}

void ALSPlayerCharacter::ApplyFacingRotation(const FRotator& NewRotation)
{
	const FRotator SanitizedRotation(0.0f, FRotator::NormalizeAxis(NewRotation.Yaw), 0.0f);
	SetActorRotation(SanitizedRotation);

	if (HasAuthority() || !IsLocallyControlled() || !ShouldSyncFacingRotation(SanitizedRotation.Yaw))
	{
		return;
	}

	LastSentFacingYaw = SanitizedRotation.Yaw;
	LastFacingSyncTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	bHasSentFacingRotation = true;
	ServerSyncFacingRotation(SanitizedRotation.Yaw);
}

void ALSPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogLS, Error, TEXT("%s missing EnhancedInputComponent."), *GetNameSafe(this));
		return;
	}

	if (MoveAction) { EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALSPlayerCharacter::Move); }
	if (RunAction)
	{
		EnhancedInput->BindAction(RunAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnRunStart);
		EnhancedInput->BindAction(RunAction, ETriggerEvent::Completed, this, &ALSPlayerCharacter::OnRunEnd);
	}

	if (AttackAction)
	{
		EnhancedInput->BindAction(AttackAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnAttack);
		EnhancedInput->BindAction(AttackAction, ETriggerEvent::Completed, this, &ALSPlayerCharacter::OnAttackReleased);
		// 매핑 컨텍스트 제거 등으로 Completed 대신 Canceled가 올 수 있어 홀드가 눌린 채 남지 않도록 함께 바인딩.
		EnhancedInput->BindAction(AttackAction, ETriggerEvent::Canceled, this, &ALSPlayerCharacter::OnAttackReleased);
	}
	if (DashAction) { EnhancedInput->BindAction(DashAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnDash); }
	if (SkillCancelAction) { EnhancedInput->BindAction(SkillCancelAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkillPreviewCancelInput); }
	// 스킬은 누름(Started)과 뗌(Completed/Canceled)을 함께 바인딩한다. 홀드-프리뷰 모드는 뗌에서 발동하고,
	// 매핑 컨텍스트 제거 등으로 Completed 대신 Canceled가 올 수 있어 릴리즈 처리를 함께 건다.
	if (Skill1Action)
	{
		EnhancedInput->BindAction(Skill1Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill1);
		EnhancedInput->BindAction(Skill1Action, ETriggerEvent::Completed, this, &ALSPlayerCharacter::OnSkill1Released);
		EnhancedInput->BindAction(Skill1Action, ETriggerEvent::Canceled, this, &ALSPlayerCharacter::OnSkill1Released);
	}
	if (Skill2Action)
	{
		EnhancedInput->BindAction(Skill2Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill2);
		EnhancedInput->BindAction(Skill2Action, ETriggerEvent::Completed, this, &ALSPlayerCharacter::OnSkill2Released);
		EnhancedInput->BindAction(Skill2Action, ETriggerEvent::Canceled, this, &ALSPlayerCharacter::OnSkill2Released);
	}
	if (Skill3Action)
	{
		EnhancedInput->BindAction(Skill3Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill3);
		EnhancedInput->BindAction(Skill3Action, ETriggerEvent::Completed, this, &ALSPlayerCharacter::OnSkill3Released);
		EnhancedInput->BindAction(Skill3Action, ETriggerEvent::Canceled, this, &ALSPlayerCharacter::OnSkill3Released);
	}
	if (Skill4Action)
	{
		EnhancedInput->BindAction(Skill4Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnSkill4);
		EnhancedInput->BindAction(Skill4Action, ETriggerEvent::Completed, this, &ALSPlayerCharacter::OnSkill4Released);
		EnhancedInput->BindAction(Skill4Action, ETriggerEvent::Canceled, this, &ALSPlayerCharacter::OnSkill4Released);
	}
	if (Item1Action) { EnhancedInput->BindAction(Item1Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem1); }
	if (Item2Action) { EnhancedInput->BindAction(Item2Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem2); }
	if (Item3Action) { EnhancedInput->BindAction(Item3Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem3); }
	if (Item4Action) { EnhancedInput->BindAction(Item4Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem4); }
	if (Item5Action) { EnhancedInput->BindAction(Item5Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem5); }
	if (Item6Action) { EnhancedInput->BindAction(Item6Action, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnItem6); }
	if (InteractAction) { EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnInteract); }
	if (LootTransferAction) { EnhancedInput->BindAction(LootTransferAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnLootTransfer); }
	if (ToggleInventoryAction) { EnhancedInput->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnToggleInventory); }
	if (MenuAction) { EnhancedInput->BindAction(MenuAction, ETriggerEvent::Started, this, &ALSPlayerCharacter::OnMenu); }
}

UInputAction* ALSPlayerCharacter::GetSkillInputAction(const ELSPlayerSkillSlot Slot) const
{
	switch (Slot)
	{
	case ELSPlayerSkillSlot::Skill1:
		return Skill1Action;
	case ELSPlayerSkillSlot::Skill2:
		return Skill2Action;
	case ELSPlayerSkillSlot::Skill3:
		return Skill3Action;
	case ELSPlayerSkillSlot::Skill4:
		return Skill4Action;
	case ELSPlayerSkillSlot::Dash:
		return DashAction;
	default:
		return nullptr;
	}
}

UInputAction* ALSPlayerCharacter::GetItemInputAction(const int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0:
		return Item1Action;
	case 1:
		return Item2Action;
	case 2:
		return Item3Action;
	case 3:
		return Item4Action;
	case 4:
		return Item5Action;
	case 5:
		return Item6Action;
	default:
		return nullptr;
	}
}

void ALSPlayerCharacter::OnAttack()
{
	if (IsInputBlocked() || IsModalUIBlockingInput())
	{
		return;
	}

	// 투척 조준 중이면 좌클릭으로 착탄 지점을 확정한다(기본 공격보다 우선).
	if (bIsThrowAiming)
	{
		ConfirmThrowAim();
		return;
	}

	if (PlayerSkillComponent && PlayerSkillComponent->IsPreviewingSkill())
	{
		if (!ConfirmActiveSkillPreview())
		{
			UE_LOG(LogLS, Warning, TEXT("%s failed to confirm skill preview. Basic attack input was consumed."),
				*GetNameSafe(this));
		}

		return;
	}

	if (!PlayerCombatComponent)
	{
		return;
	}

	// 달리기 중 기본 공격이 들어오면 걷기로 전환한다. 공격 후 자동 복귀는 없다(달리기 키 재입력 필요).
	if (bIsRunning)
	{
		OnRunEnd();
	}

	PlayerCombatComponent->SetBasicAttackHeld(true);
	PlayerCombatComponent->RequestBasicAttack();
}

void ALSPlayerCharacter::OnAttackReleased()
{
	// 홀드 해제는 입력 블록 상태(모달 UI 등)와 무관하게 항상 서버에 전달한다. 눌린 적 없으면 dedup으로 no-op.
	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->SetBasicAttackHeld(false);
	}
}

void ALSPlayerCharacter::OnDash()
{
	if (IsInputBlocked() || IsModalUIBlockingInput())
	{
		return;
	}

	if (CancelThrowAim() || CancelActiveSkillPreview())
	{
		return;
	}

	if (!PlayerCombatComponent)
	{
		return;
	}

	const FVector DashDirection = GetDashDirection();
	if (!HasAuthority())
	{
		if (!HasStamina(DashStaminaCost))
		{
			return;
		}

		bool bShouldExecuteImmediately = false;
		if (!PlayerCombatComponent->SubmitDashInput(DashDirection, bShouldExecuteImmediately))
		{
			return;
		}

		if (bShouldExecuteImmediately)
		{
			PlayerCombatComponent->PredictDashMovement(DashDirection);
		}

		ServerRequestDash(DashDirection);
		return;
	}

	if (HasStamina(DashStaminaCost) && PlayerCombatComponent->RequestDash(DashDirection))
	{
		TrySpendStamina(DashStaminaCost);
	}
}

void ALSPlayerCharacter::OnSkillPreviewCancelInput()
{
	if (CancelThrowAim())
	{
		return;
	}
	CancelActiveSkillPreview();
}

void ALSPlayerCharacter::OnSkill1()
{
	// 우클릭 겸용: 투척 조준/스킬 프리뷰 중이면 취소, 아니면 스킬1 발동/프리뷰. (우클릭엔 IA_Skill1만 매핑 — IA_SkillCancel 중복 금지)
	if (CancelThrowAim() || CancelActiveSkillPreview())
	{
		return;
	}
	HandleSkillInputPressed(ELSPlayerSkillSlot::Skill1);
}
void ALSPlayerCharacter::OnSkill2() { HandleSkillInputPressed(ELSPlayerSkillSlot::Skill2); }
void ALSPlayerCharacter::OnSkill3() { HandleSkillInputPressed(ELSPlayerSkillSlot::Skill3); }
void ALSPlayerCharacter::OnSkill4() { HandleSkillInputPressed(ELSPlayerSkillSlot::Skill4); }
void ALSPlayerCharacter::OnSkill1Released() { HandleSkillInputReleased(ELSPlayerSkillSlot::Skill1); }
void ALSPlayerCharacter::OnSkill2Released() { HandleSkillInputReleased(ELSPlayerSkillSlot::Skill2); }
void ALSPlayerCharacter::OnSkill3Released() { HandleSkillInputReleased(ELSPlayerSkillSlot::Skill3); }
void ALSPlayerCharacter::OnSkill4Released() { HandleSkillInputReleased(ELSPlayerSkillSlot::Skill4); }
void ALSPlayerCharacter::OnItem1() { TryUseQuickSlot(0); }
void ALSPlayerCharacter::OnItem2() { TryUseQuickSlot(1); }
void ALSPlayerCharacter::OnItem3() { TryUseQuickSlot(2); }
void ALSPlayerCharacter::OnItem4() { TryUseQuickSlot(3); }
void ALSPlayerCharacter::OnItem5() { TryUseQuickSlot(4); }
void ALSPlayerCharacter::OnItem6() { TryUseQuickSlot(5); }

void ALSPlayerCharacter::TryUseQuickSlot(const int32 QuickSlotIndex)
{
	if (!IsLocallyControlled() || bIsConsumableCasting)
	{
		return;
	}

	// 투척 조준 중 아이템 키 재입력은 조준을 취소한다(확정은 좌클릭).
	if (bIsThrowAiming)
	{
		CancelThrowAim();
		return;
	}

	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (!GameInstance)
	{
		return;
	}

	// 1) 퀵슬롯 등록 소모품 RowName 조회.
	const ULSSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<ULSSaveSubsystem>();
	if (!SaveSubsystem || !SaveSubsystem->GetQuickSlots().IsValidIndex(QuickSlotIndex))
	{
		return;
	}

	// 적재 프로토콜로 해금되지 않은 칸은 사용할 수 없다(인덱스 0~5 = 1~6번 칸과 1:1).
	// 컨트롤러 경유로 디버그 패널 오버라이드까지 반영해 UI 표시(가시성)와 항상 같은 값을 쓴다.
	const ALSPlayerControllerBase* QuickSlotOwner = Cast<ALSPlayerControllerBase>(GetController());
	const int32 UnlockedQuickSlotCount = QuickSlotOwner ? QuickSlotOwner->GetUnlockedQuickSlotCount() : SaveSubsystem->GetUnlockedQuickSlotCount();
	if (QuickSlotIndex >= UnlockedQuickSlotCount)
	{
		UE_LOG(LogLS, Verbose, TEXT("[QuickSlot] 잠긴 슬롯 %d 사용 시도 (해금=%d)."), QuickSlotIndex, UnlockedQuickSlotCount);
		return;
	}

	const FName ItemRowName = SaveSubsystem->GetQuickSlots()[QuickSlotIndex];
	if (ItemRowName.IsNone())
	{
		UE_LOG(LogLS, Verbose, TEXT("[Consumable] 사용 실패: 퀵슬롯 %d 비어 있음."), QuickSlotIndex);
		return;
	}

	// 2) 소모품 거동 정의 조회.
	const ULSGameDataSubsystem* GameData = GameInstance->GetSubsystem<ULSGameDataSubsystem>();
	const FLSConsumableRow* ConsumableDef = GameData ? GameData->FindConsumableRow(ItemRowName, TEXT("TryUseQuickSlot")) : nullptr;
	if (!ConsumableDef)
	{
		UE_LOG(LogLS, Warning, TEXT("[QuickSlot] No DT_Consumable row for '%s'."), *ItemRowName.ToString());
		return;
	}

	// 3) 레이드 중 + 보유 수량 확인(클라 미러 기준 선검사).
	const ALSPlayerControllerBase* PlayerController = GetController<ALSPlayerControllerBase>();
	const ULSRaidInventoryComponent* RaidInventory = PlayerController ? PlayerController->GetRaidInventoryComponent() : nullptr;
	if (!RaidInventory || !RaidInventory->IsRaidActive())
	{
		return;
	}
	if (LSCraftingUtils::CountItem(RaidInventory->GetSessionInventory(), ItemRowName) < 1)
	{
		UE_LOG(LogLS, Log, TEXT("[Consumable] 사용 실패: '%s' 보유 수량 없음."), *ItemRowName.ToString());
		return;
	}

	// 투척형은 조준(범위 인디케이터)부터, 그 외는 즉시(시전 시간 반영) 사용한다.
	if (ConsumableDef->Item_Use_Type == ELSConsumableUseType::Throwable)
	{
		BeginThrowAim(ItemRowName, *ConsumableDef);
		return;
	}

	bPendingThrow = false;
	BeginConsumableCast(ItemRowName, *ConsumableDef);
}

void ALSPlayerCharacter::BeginConsumableCast(const FName ItemRowName, const FLSConsumableRow& ConsumableDef)
{
	CastingConsumableRowName = ItemRowName;
	PendingTriggerDelay = FMath::Max(ConsumableDef.Item_Trigger_Delay, 0.0f);

	// 시전 시간이 없으면 게이지 없이 바로 완료 처리로 넘어간다.
	if (ConsumableDef.Item_Cast_Time <= 0.0f)
	{
		bIsConsumableCasting = false;
		HandleConsumableCastComplete();
		return;
	}

	bIsConsumableCasting = true;
	bCastAllowsMove = ConsumableDef.Item_Can_Move;

	if (ALSPlayerControllerBase* PlayerController = GetController<ALSPlayerControllerBase>())
	{
		const LSInventorySlotUtils::FLSItemTradeInfo TradeInfo = LSInventorySlotUtils::ResolveItemTradeInfo(ItemRowName);
		PlayerController->ShowCastGauge(TradeInfo.bValid ? TradeInfo.Name : FText::FromName(ItemRowName), ConsumableDef.Item_Cast_Time);
	}

	GetWorldTimerManager().SetTimer(ConsumableCastCompleteTimer, this, &ALSPlayerCharacter::HandleConsumableCastComplete, ConsumableDef.Item_Cast_Time, false);
}

void ALSPlayerCharacter::HandleConsumableCastComplete()
{
	// 시전 구간 종료(여기부터는 취소 불가). 게이지를 내린다.
	bIsConsumableCasting = false;
	if (ALSPlayerControllerBase* PlayerController = GetController<ALSPlayerControllerBase>())
	{
		PlayerController->HideCastGauge();
	}

	// 모든 소모품은 시전 완료 시점에 수량을 차감한다(효과는 이후 발동 지연 뒤 적용).
	if (!CastingConsumableRowName.IsNone())
	{
		if (HasAuthority())
		{
			ConsumeConsumableAuthoritative(CastingConsumableRowName);
		}
		else
		{
			ServerConsumeConsumable(CastingConsumableRowName);
		}
	}

	if (PendingTriggerDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(ConsumableTriggerDelayTimer, this, &ALSPlayerCharacter::FinishConsumableUse, PendingTriggerDelay, false);
		return;
	}

	FinishConsumableUse();
}

void ALSPlayerCharacter::FinishConsumableUse()
{
	const FName ItemRowName = CastingConsumableRowName;
	CastingConsumableRowName = NAME_None;
	if (ItemRowName.IsNone())
	{
		bPendingThrow = false;
		return;
	}

	// 투척 확정이면 착탄 지점을 함께 서버로 보낸다.
	if (bPendingThrow)
	{
		const FVector TargetLocation = PendingThrowTargetLocation;
		bPendingThrow = false;
		if (HasAuthority())
		{
			UseThrownConsumableAuthoritative(ItemRowName, TargetLocation);
		}
		else
		{
			ServerUseThrownConsumable(ItemRowName, TargetLocation);
		}
		return;
	}

	if (HasAuthority())
	{
		UseConsumableAuthoritative(ItemRowName);
	}
	else
	{
		ServerUseConsumable(ItemRowName);
	}
}

void ALSPlayerCharacter::CancelConsumableCast()
{
	if (!bIsConsumableCasting)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ConsumableCastCompleteTimer);
	bIsConsumableCasting = false;
	CastingConsumableRowName = NAME_None;
	bPendingThrow = false;

	if (ALSPlayerControllerBase* PlayerController = GetController<ALSPlayerControllerBase>())
	{
		PlayerController->HideCastGauge();
	}
}

void ALSPlayerCharacter::ServerUseConsumable_Implementation(const FName ItemRowName)
{
	UseConsumableAuthoritative(ItemRowName);
}

void ALSPlayerCharacter::UseConsumableAuthoritative(const FName ItemRowName)
{
	if (!HasAuthority() || ItemRowName.IsNone())
	{
		return;
	}

	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULSGameDataSubsystem* GameData = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	const FLSConsumableRow* ConsumableDef = GameData ? GameData->FindConsumableRow(ItemRowName, TEXT("UseConsumableAuthoritative")) : nullptr;
	if (!ConsumableDef)
	{
		UE_LOG(LogLS, Warning, TEXT("[QuickSlot] No DT_Consumable row for '%s' on server."), *ItemRowName.ToString());
		return;
	}

	ALSPlayerControllerBase* PlayerController = GetController<ALSPlayerControllerBase>();
	ULSRaidInventoryComponent* RaidInventory = PlayerController ? PlayerController->GetRaidInventoryComponent() : nullptr;
	if (!RaidInventory || !RaidInventory->IsRaidActive())
	{
		return;
	}

	// 수량 차감은 시전 완료 시점(ConsumeConsumableAuthoritative)에서 이미 끝났다. 여기서는 효과만 적용한다.
	// 효과 적용(대상=자기 자신). 소모품 효과 코어가 서버 권한을 다시 검증한다.
	bool bApplied = false;
	if (ULSCharacterCombatComponent* CombatComponent = GetCharacterCombatComponent())
	{
		bApplied = CombatComponent->ApplyConsumableEffects(*ConsumableDef);
	}

	UE_LOG(LogLS, Log, TEXT("[Consumable] 사용 발동: '%s'%s."), *ItemRowName.ToString(), bApplied ? TEXT("") : TEXT(" (적용된 효과 없음)"));
}

namespace
{
// 소모품 Row의 도형/크기를 스킬 범위 인디케이터 스펙으로 변환한다(투척물은 착탄 지점 기준).
FLSSkillAreaPreviewSpec BuildConsumableThrowPreviewSpec(const FLSConsumableRow& Row)
{
	FLSSkillAreaPreviewSpec Spec;
	Spec.LocationMode = ELSSkillPreviewLocationMode::MouseWorld;
	switch (Row.Item_Range_Shape)
	{
	case ELSConsumableRangeShape::Cone:
		Spec.Shape = ELSSkillAreaShape::Circle;
		Spec.Radius = Row.Item_Range_X;
		Spec.Degrees = Row.Item_Range_Y;
		break;
	case ELSConsumableRangeShape::Box:
		Spec.Shape = ELSSkillAreaShape::Box;
		Spec.BoxLength = Row.Item_Range_X;
		Spec.BoxWidth = Row.Item_Range_Y;
		break;
	case ELSConsumableRangeShape::Sphere:
	case ELSConsumableRangeShape::None:
	default:
		Spec.Shape = ELSSkillAreaShape::Circle;
		Spec.Radius = Row.Item_Range_X;
		Spec.Degrees = 360.0f;
		break;
	}
	return Spec;
}
}

ULSSkillPreviewComponent* ALSPlayerCharacter::ResolveSkillPreviewComponent() const
{
	return FindComponentByClass<ULSSkillPreviewComponent>();
}

void ALSPlayerCharacter::BeginThrowAim(const FName ItemRowName, const FLSConsumableRow& ConsumableDef)
{
	if (bIsThrowAiming || bIsConsumableCasting)
	{
		return;
	}

	ULSSkillPreviewComponent* PreviewComponent = ResolveSkillPreviewComponent();
	if (!PreviewComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("[QuickSlot] 투척 조준 실패: ULSSkillPreviewComponent가 없음(%s)."), *GetNameSafe(this));
		return;
	}

	if (!PreviewComponent->BeginAreaPreview(BuildConsumableThrowPreviewSpec(ConsumableDef)))
	{
		return;
	}

	bIsThrowAiming = true;
	ThrowAimRowName = ItemRowName;
	ThrowAimCastRange = FMath::Max(ConsumableDef.Item_Cast_Range, 0.0f);
	ThrowAimTargetLocation = GetActorLocation();
	UpdateThrowAim();
}

void ALSPlayerCharacter::UpdateThrowAim()
{
	if (!bIsThrowAiming)
	{
		return;
	}

	ULSSkillPreviewComponent* PreviewComponent = ResolveSkillPreviewComponent();
	if (!PreviewComponent)
	{
		return;
	}

	FVector MouseWorldPoint = FVector::ZeroVector;
	if (!ResolveMouseWorldPoint(MouseWorldPoint))
	{
		return;
	}

	// 사거리 clamp(소유자 기준 2D).
	const FVector OwnerLocation = GetActorLocation();
	FVector ToTarget = MouseWorldPoint - OwnerLocation;
	ToTarget.Z = 0.0f;
	const float Distance = ToTarget.Size();
	FVector TargetLocation = MouseWorldPoint;
	if (ThrowAimCastRange > 0.0f && Distance > ThrowAimCastRange && Distance > KINDA_SMALL_NUMBER)
	{
		TargetLocation = OwnerLocation + (ToTarget / Distance * ThrowAimCastRange);
		TargetLocation.Z = MouseWorldPoint.Z;
	}

	ThrowAimTargetLocation = TargetLocation;

	FVector AimDirection = TargetLocation - OwnerLocation;
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = GetActorForwardVector();
	}

	PreviewComponent->UpdateAreaPreview(TargetLocation, AimDirection.Rotation());
}

bool ALSPlayerCharacter::ConfirmThrowAim()
{
	if (!bIsThrowAiming)
	{
		return false;
	}

	const FName ItemRowName = ThrowAimRowName;
	const FVector TargetLocation = ThrowAimTargetLocation;

	if (ULSSkillPreviewComponent* PreviewComponent = ResolveSkillPreviewComponent())
	{
		PreviewComponent->EndAreaPreview();
	}
	bIsThrowAiming = false;
	ThrowAimRowName = NAME_None;

	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULSGameDataSubsystem* GameData = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	const FLSConsumableRow* ConsumableDef = GameData ? GameData->FindConsumableRow(ItemRowName, TEXT("ConfirmThrowAim")) : nullptr;
	if (!ConsumableDef)
	{
		return false;
	}

	// 확정된 착탄 지점으로 (시전 시간 반영) 투척한다.
	bPendingThrow = true;
	PendingThrowTargetLocation = TargetLocation;
	BeginConsumableCast(ItemRowName, *ConsumableDef);
	return true;
}

bool ALSPlayerCharacter::CancelThrowAim()
{
	if (!bIsThrowAiming)
	{
		return false;
	}

	if (ULSSkillPreviewComponent* PreviewComponent = ResolveSkillPreviewComponent())
	{
		PreviewComponent->EndAreaPreview();
	}
	bIsThrowAiming = false;
	ThrowAimRowName = NAME_None;
	return true;
}

void ALSPlayerCharacter::ServerUseThrownConsumable_Implementation(const FName ItemRowName, const FVector_NetQuantize TargetLocation)
{
	UseThrownConsumableAuthoritative(ItemRowName, TargetLocation);
}

void ALSPlayerCharacter::UseThrownConsumableAuthoritative(const FName ItemRowName, const FVector& TargetLocation)
{
	if (!HasAuthority() || ItemRowName.IsNone())
	{
		return;
	}

	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULSGameDataSubsystem* GameData = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	const FLSConsumableRow* ConsumableDef = GameData ? GameData->FindConsumableRow(ItemRowName, TEXT("UseThrownConsumableAuthoritative")) : nullptr;
	if (!ConsumableDef)
	{
		UE_LOG(LogLS, Warning, TEXT("[QuickSlot] No DT_Consumable row for '%s' on server (thrown)."), *ItemRowName.ToString());
		return;
	}

	// 수량 차감은 시전 완료 시점(ConsumeConsumableAuthoritative)에서 이미 끝났다.
	// 여기서는 발동 지연 뒤 효과만 적용한다(재차감/재검사 없음).

	// 착탄 지점 범위 내 적을 수집해 효과 적용(Self 효과는 소유자 1회).
	TArray<AActor*> AreaTargets;
	CollectThrowTargets(*ConsumableDef, TargetLocation, AreaTargets);

	bool bApplied = false;
	if (ULSCharacterCombatComponent* CombatComponent = GetCharacterCombatComponent())
	{
		bApplied = CombatComponent->ApplyConsumableEffectsInArea(*ConsumableDef, AreaTargets);
	}

	UE_LOG(LogLS, Log, TEXT("[Consumable] 투척 발동: '%s' 착탄 대상 %d명%s."), *ItemRowName.ToString(), AreaTargets.Num(), bApplied ? TEXT("") : TEXT(" (적용된 효과 없음)"));
}

void ALSPlayerCharacter::ServerConsumeConsumable_Implementation(const FName ItemRowName)
{
	ConsumeConsumableAuthoritative(ItemRowName);
}

void ALSPlayerCharacter::ConsumeConsumableAuthoritative(const FName ItemRowName)
{
	if (!HasAuthority() || ItemRowName.IsNone())
	{
		return;
	}

	ALSPlayerControllerBase* PlayerController = GetController<ALSPlayerControllerBase>();
	ULSRaidInventoryComponent* RaidInventory = PlayerController ? PlayerController->GetRaidInventoryComponent() : nullptr;
	if (!RaidInventory || !RaidInventory->IsRaidActive())
	{
		return;
	}

	if (LSCraftingUtils::CountItem(RaidInventory->GetSessionInventory(), ItemRowName) < 1)
	{
		UE_LOG(LogLS, Warning, TEXT("[Consumable] 차감 실패: '%s' 보유 수량 없음."), *ItemRowName.ToString());
		return;
	}

	// 시전 완료 시점 차감(효과는 발동 지연 뒤 별도 적용). 차감 즉시 클라 미러로 개수 갱신.
	RaidInventory->ConsumeSessionItem(ItemRowName, 1);
	PlayerController->SyncRaidInventoryToClient();
	UE_LOG(LogLS, Log, TEXT("[Consumable] 차감: '%s' (시전 완료)."), *ItemRowName.ToString());
}

void ALSPlayerCharacter::CollectThrowTargets(const FLSConsumableRow& ConsumableDef, const FVector& Center, TArray<AActor*>& OutTargets) const
{
	UWorld* World = GetWorld();
	const float RangeX = FMath::Max(ConsumableDef.Item_Range_X, 0.0f);
	if (!World || RangeX <= 0.0f)
	{
		return;
	}

	// 도형 방향(소유자 → 착탄 지점).
	FVector Forward = Center - GetActorLocation();
	Forward.Z = 0.0f;
	Forward = Forward.GetSafeNormal2D();
	if (Forward.IsNearlyZero())
	{
		Forward = GetActorForwardVector().GetSafeNormal2D();
	}
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();

	const float HalfConeCos = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(ConsumableDef.Item_Range_Y, 0.0f, 360.0f) * 0.5f));
	const float HalfLength = RangeX * 0.5f;
	const float HalfWidth = FMath::Max(ConsumableDef.Item_Range_Y, 0.0f) * 0.5f;

	for (TActorIterator<ALSEnemyCharacter> It(World); It; ++It)
	{
		ALSEnemyCharacter* Enemy = *It;
		if (!Enemy)
		{
			continue;
		}

		FVector ToEnemy = Enemy->GetActorLocation() - Center;
		ToEnemy.Z = 0.0f;
		const float Dist = ToEnemy.Size();

		bool bInside = false;
		switch (ConsumableDef.Item_Range_Shape)
		{
		case ELSConsumableRangeShape::Box:
		{
			const float ForwardDist = FMath::Abs(FVector::DotProduct(ToEnemy, Forward));
			const float RightDist = FMath::Abs(FVector::DotProduct(ToEnemy, Right));
			bInside = ForwardDist <= HalfLength && RightDist <= HalfWidth;
			break;
		}
		case ELSConsumableRangeShape::Cone:
			if (Dist <= KINDA_SMALL_NUMBER)
			{
				bInside = true;
			}
			else if (Dist <= RangeX)
			{
				bInside = FVector::DotProduct(ToEnemy / Dist, Forward) >= HalfConeCos;
			}
			break;
		case ELSConsumableRangeShape::Sphere:
		case ELSConsumableRangeShape::None:
		default:
			bInside = Dist <= RangeX;
			break;
		}

		if (bInside)
		{
			OutTargets.Add(Enemy);
		}
	}
}

void ALSPlayerCharacter::OnInteract()
{
	if (!IsLocallyControlled()) return;

	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetController()))
	{
		if (PlayerController->IsLobbyStorageWidgetOpen())
		{
			HideInventoryWidget();
			return;
		}
	}

	if (IsInventoryWidgetOpen())
	{
		HideInventoryWidget();
		return;
	}

	AActor* BestTarget = ResolveBestInteractTarget();
	if (BestTarget)
	{
		// 칩스테이션처럼 인벤토리 위젯을 거치지 않는 모달도 있으므로 상호작용 직전에 프리뷰를 정리한다.
		CancelActiveSkillPreview();

		ServerRequestInteract(BestTarget);

		if (BestTarget->IsA<ALSLootBox>() || BestTarget->IsA<ALSLobbyStorageActor>())
		{
			ShowInventoryWidgetForTarget(BestTarget);
		}
	}
}

AActor* ALSPlayerCharacter::ResolveBestInteractTarget()
{
	if (!IsLocallyControlled())
	{
		return nullptr;
	}

	const FVector MyLocation = GetActorLocation();

	FVector MouseWorldPoint = FVector::ZeroVector;
	if (!ResolveMouseWorldPoint(MouseWorldPoint))
	{
		return nullptr;
	}

	FVector AimDirection = MouseWorldPoint - MyLocation;
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		return nullptr;
	}

	AimDirection.Normalize();

	AActor* BestTarget = nullptr;
	float BestScore = -FLT_MAX;

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor == this || !Actor->Implements<ULSInteractable>()) continue;
		if (!ILSInteractable::Execute_CanInteract(Actor, this)) continue;

		const FVector ToActor = Actor->GetActorLocation() - MyLocation;
		const float DistSq = ToActor.SizeSquared();
		if (DistSq > FMath::Square(MaxInteractRange)) continue;

		FVector TargetDirection = ToActor;
		TargetDirection.Z = 0.0f;

		const float DistanceScore = 1.0f - FMath::Clamp(FMath::Sqrt(DistSq) / FMath::Max(MaxInteractRange, 1.0f), 0.0f, 1.0f);
		const float AngleScore = TargetDirection.IsNearlyZero()
			? 1.0f
			: (FMath::Clamp(FVector::DotProduct(AimDirection, TargetDirection.GetSafeNormal()), -1.0f, 1.0f) + 1.0f) * 0.5f;
		const float Score = (DistanceScore * InteractDistanceWeight) + (AngleScore * InteractAngleWeight);
		if (Score < InteractScoreThreshold) continue;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Actor;
		}
	}

	return BestTarget;
}

void ALSPlayerCharacter::OnLootTransfer()
{
	if (!IsLocallyControlled() || !IsInventoryWidgetOpen())
	{
		return;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetController());
	if (!PlayerController || !PlayerController->TransferHoveredLootDropItemToInventory())
	{
		return;
	}

	if (PlayerController->HasAuthority())
	{
		RebuildInventoryWidgetSlots();
	}
}

void ALSPlayerCharacter::OnToggleInventory()
{
	if (!IsLocallyControlled()) return;

	// 칩스테이션 등 모달이 열려 있으면 그걸 닫고, 닫을 게 없을 때만 단독 인벤토리를 연다.
	if (TryCloseOpenModalPanel())
	{
		return;
	}

	ShowInventoryWidgetStandalone();
}

void ALSPlayerCharacter::OnMenu()
{
	if (!IsLocallyControlled()) return;

	// ESC(메뉴/백): 열린 모달이 있으면 먼저 닫는다.
	if (TryCloseOpenModalPanel())
	{
		return;
	}

	// 닫을 UI가 없으면 설정 메뉴를 연다. (설정 UI 구현 시 이 자리에 연결)
}

bool ALSPlayerCharacter::TryCloseOpenModalPanel()
{
	// 칩스테이션은 인벤토리 위젯과 별개인 컨트롤러 소유 모달이라 먼저 확인한다.
	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetController()))
	{
		if (PlayerController->IsChipStationWidgetOpen())
		{
			PlayerController->HideChipStationWidget();
			return true;
		}
	}

	// 인벤토리(컨테이너로 열렸든 단독이든) — 함께 떠 있던 룻드랍/로비 창고도 같이 닫힌다.
	if (IsInventoryWidgetOpen())
	{
		HideInventoryWidget();
		return true;
	}

	return false;
}

void ALSPlayerCharacter::RebuildInventoryWidgetSlots()
{
	if (ULSInventoryWidget* LSInventoryWidget = Cast<ULSInventoryWidget>(InventoryWidget))
	{
		LSInventoryWidget->RebuildInventorySlots();
		LSInventoryWidget->RebuildConfirmedStorageSlots();
		// 레이드 중에는 장비도 세션 정식 영역이라 인벤/Safe와 함께 다시 그려야 한다.
		// (안 그리면 레이드 중 장착/해제 후 장비칸이 stale로 남아 아이템이 사라진 것처럼 보인다.)
		LSInventoryWidget->RebuildEquipmentSlots();
	}
}

bool ALSPlayerCharacter::ShowInventoryWidgetInternal(bool bShowStoreAllButton, bool bShowQuickSlotBar)
{
	if (!IsLocallyControlled())
	{
		return false;
	}

	if (!InventoryWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("InventoryWidgetClass is not set on %s."), *GetNameSafe(this));
		return false;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot show inventory widget because player controller is missing on %s."), *GetNameSafe(this));
		return false;
	}

	if (!InventoryWidget)
	{
		InventoryWidget = CreateWidget<UUserWidget>(PlayerController, InventoryWidgetClass);
		if (!InventoryWidget)
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create inventory widget on %s."), *GetNameSafe(this));
			return false;
		}
	}

	if (!InventoryWidget->IsInViewport())
	{
		InventoryWidget->AddToViewport(LSUILayer::ModalPanelInventory);
	}

	if (ULSInventoryWidget* LSInventoryWidget = Cast<ULSInventoryWidget>(InventoryWidget))
	{
		LSInventoryWidget->RebuildInventorySlots();
		LSInventoryWidget->RebuildConfirmedStorageSlots();
		LSInventoryWidget->SetStoreAllButtonVisible(bShowStoreAllButton);
		// 퀵슬롯 바는 Tab 인벤토리(및 로비 창고 동반)에서만 켠다. 레이드 루팅 박스로 연 인벤토리에선 숨긴다.
		LSInventoryWidget->SetQuickSlotBarVisible(bShowQuickSlotBar);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("InventoryWidgetClass is not derived from ULSInventoryWidget on %s."), *GetNameSafe(this));
	}

	InventoryWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// 인벤토리가 뜨면 진행 중이던 스킬 프리뷰는 취소한다(모달 아래 프리뷰 잔류 방지).
	CancelActiveSkillPreview();

	if (ALSPlayerControllerBase* LSPlayerController = Cast<ALSPlayerControllerBase>(PlayerController))
	{
		LSPlayerController->UpdateBackgroundBlurVisibility();
	}

	return true;
}

void ALSPlayerCharacter::ShowInventoryWidgetForTarget(AActor* Target)
{
	if (!Target)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot show inventory widget because target is missing on %s."), *GetNameSafe(this));
		return;
	}

	// 루팅 박스로 연 인벤토리는 퀵슬롯 바를 숨긴다. 로비 창고와 함께 연 경우엔 표시한다(Tab로 연 것으로 취급).
	const bool bShowQuickSlotBar = !Target->IsA<ALSLootBox>();
	if (!ShowInventoryWidgetInternal(Target->IsA<ALSLobbyStorageActor>(), bShowQuickSlotBar))
	{
		return;
	}

	ActiveInventoryTarget = Target;
	bIsStandaloneInventoryOpen = false;
}

void ALSPlayerCharacter::ShowInventoryWidgetStandalone()
{
	if (!IsLocallyControlled() || IsInventoryWidgetOpen())
	{
		return;
	}

	// 단독 인벤토리는 컨테이너가 없으므로 전부 보관 버튼을 숨긴다. Tab로 연 것이므로 퀵슬롯 바는 표시한다.
	if (!ShowInventoryWidgetInternal(false, true))
	{
		return;
	}

	ActiveInventoryTarget.Reset();
	bIsStandaloneInventoryOpen = true;
}

void ALSPlayerCharacter::HideInventoryWidget()
{
	if (InventoryWidget)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetController()))
	{
		PlayerController->HideLootDropWidget();
		PlayerController->HideLobbyStorageWidget();
		PlayerController->UpdateBackgroundBlurVisibility();
	}

	ActiveInventoryTarget.Reset();
	bIsStandaloneInventoryOpen = false;
}

void ALSPlayerCharacter::UpdateInventoryWidgetDistance()
{
	if (!IsInventoryWidgetOpen())
	{
		return;
	}

	// 단독(Tab) 인벤토리는 연동된 컨테이너가 없으므로 거리 기반 자동 닫기 대상이 아니다.
	if (bIsStandaloneInventoryOpen)
	{
		return;
	}

	AActor* Target = ActiveInventoryTarget.Get();
	if (!Target)
	{
		UE_LOG(LogLS, Warning, TEXT("Closing inventory widget because active inventory target is missing on %s."), *GetNameSafe(this));
		HideInventoryWidget();
		return;
	}

	const float Distance = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	if (Distance > MaxInteractRange)
	{
		HideInventoryWidget();
	}
}

bool ALSPlayerCharacter::IsInventoryWidgetOpen() const
{
	return InventoryWidget && InventoryWidget->IsVisible();
}

ELSSkillCastMode ALSPlayerCharacter::ResolveSlotCastMode(ELSPlayerSkillSlot Slot) const
{
	// 디버그 오버라이드 우선 적용을 위해 컴포넌트로 해석을 위임한다(설정 저장소 조회 포함).
	return PlayerSkillComponent ? PlayerSkillComponent->GetEffectiveCastMode(Slot) : ELSSkillCastMode::QuickCastWithIndicator;
}

void ALSPlayerCharacter::HandleSkillInputPressed(ELSPlayerSkillSlot Slot)
{
	// 즉시 퀵캐스트는 프리뷰 없이 바로 발동, 그 외 두 모드는 프리뷰 진입(확정 트리거만 다름).
	if (ResolveSlotCastMode(Slot) == ELSSkillCastMode::QuickCast)
	{
		ActivateSkillInstant(Slot);
		return;
	}

	BeginSkillPreview(Slot);
}

void ALSPlayerCharacter::HandleSkillInputReleased(ELSPlayerSkillSlot Slot)
{
	// 홀드-프리뷰 모드에서만 키를 뗄 때 커서 위치로 확정 발동한다.
	if (ResolveSlotCastMode(Slot) != ELSSkillCastMode::QuickCastWithIndicator)
	{
		return;
	}

	if (!IsLocallyControlled() || !PlayerSkillComponent || PlayerSkillComponent->GetActiveSlot() != Slot || !PlayerSkillComponent->IsPreviewingSkill())
	{
		return;
	}

	ConfirmActiveSkillPreview();
}

void ALSPlayerCharacter::BeginSkillPreview(ELSPlayerSkillSlot Slot)
{
	// 스킬 시전(몽타주) 중이거나 모달 UI가 열려 있으면 새 스킬 프리뷰 진입을 막는다(Skill1~3 공통 게이트).
	if (IsInputBlocked() || IsModalUIBlockingInput())
	{
		return;
	}

	if (!IsLocallyControlled() || !PlayerSkillComponent)
	{
		return;
	}

	if (PlayerSkillComponent->BeginSkillPreview(Slot))
	{
		UpdateActiveSkillPreview();
	}
}

void ALSPlayerCharacter::ActivateSkillInstant(ELSPlayerSkillSlot Slot)
{
	// 프리뷰 진입과 동일한 입력 게이트.
	if (IsInputBlocked() || IsModalUIBlockingInput())
	{
		return;
	}

	if (!IsLocallyControlled() || !PlayerSkillComponent)
	{
		return;
	}

	FVector MouseWorldPoint = FVector::ZeroVector;
	if (!ResolveMouseWorldPoint(MouseWorldPoint))
	{
		return;
	}

	FVector AimDirection = MouseWorldPoint - GetActorLocation();
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = GetActorForwardVector();
	}

	PlayerSkillComponent->ActivateSkillInstant(Slot, MouseWorldPoint, AimDirection.Rotation());
}

void ALSPlayerCharacter::UpdateActiveSkillPreview()
{
	if (!IsLocallyControlled() || !PlayerSkillComponent || !PlayerSkillComponent->IsPreviewingSkill())
	{
		return;
	}

	FVector MouseWorldPoint = FVector::ZeroVector;
	if (!ResolveMouseWorldPoint(MouseWorldPoint))
	{
		return;
	}

	FVector AimDirection = MouseWorldPoint - GetActorLocation();
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = GetActorForwardVector();
	}

	FLSSkillAreaPreviewSpec PreviewSpec;
	const bool bHasPreviewSpec = PlayerSkillComponent->GetActivePreviewSpec(PreviewSpec);
	FVector PreviewLocation =
		bHasPreviewSpec && PreviewSpec.LocationMode == ELSSkillPreviewLocationMode::CasterOrigin
			? GetActorLocation()
			: MouseWorldPoint;

	if (bHasPreviewSpec && !PreviewSpec.LocationOffset.IsNearlyZero())
	{
		const FVector Forward = AimDirection.GetSafeNormal2D();
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
		PreviewLocation += (Forward * PreviewSpec.LocationOffset.X) + (Right * PreviewSpec.LocationOffset.Y);
	}

	PlayerSkillComponent->UpdateActiveSkillPreview(PreviewLocation, AimDirection.Rotation());
}

bool ALSPlayerCharacter::ConfirmActiveSkillPreview()
{
	if (!IsLocallyControlled())
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot confirm skill preview because it is not locally controlled."),
			*GetNameSafe(this));
		return false;
	}

	if (!PlayerSkillComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot confirm skill preview because PlayerSkillComponent is missing."),
			*GetNameSafe(this));
		return false;
	}

	if (!PlayerSkillComponent->IsPreviewingSkill())
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot confirm skill preview because no skill preview is active."),
			*GetNameSafe(this));
		return false;
	}

	FVector MouseWorldPoint = FVector::ZeroVector;
	if (!ResolveMouseWorldPoint(MouseWorldPoint))
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot confirm skill preview because mouse world point could not be resolved."),
			*GetNameSafe(this));
		return false;
	}

	FVector AimDirection = MouseWorldPoint - GetActorLocation();
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = GetActorForwardVector();
	}

	return PlayerSkillComponent->ConfirmAnyActiveSkillPreview(MouseWorldPoint, AimDirection.Rotation());
}

bool ALSPlayerCharacter::CancelActiveSkillPreview()
{
	if (!IsLocallyControlled() || !PlayerSkillComponent || !PlayerSkillComponent->IsPreviewingSkill())
	{
		return false;
	}

	PlayerSkillComponent->CancelAnyActiveSkillPreview();
	return true;
}

void ALSPlayerCharacter::ServerRequestInteract_Implementation(AActor* Target)
{
	if (!Target || !Target->Implements<ULSInteractable>()) return;

	const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	if (Dist > MaxInteractRange * 1.5f) return;

	if (!ILSInteractable::Execute_CanInteract(Target, this)) return;

	ILSInteractable::Execute_Interact(Target, this);

	if (NoiseEmitterComponent)
	{
		NoiseEmitterComponent->EmitInteractNoise();
	}
}

void ALSPlayerCharacter::OnRunStart()
{
	if (!CanStartRunning())
	{
		ApplyRunState(false);
		return;
	}

	ApplyRunState(true);

	if (!HasAuthority())
	{
		ServerSetRunState(true);
	}
}

void ALSPlayerCharacter::OnRunEnd()
{
	ApplyRunState(false);

	if (!HasAuthority())
	{
		ServerSetRunState(false);
	}
}

bool ALSPlayerCharacter::IsInputBlocked() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC && ASC->HasMatchingGameplayTag(LSGameplayTags::State_InputBlocked);
}

bool ALSPlayerCharacter::IsModalUIBlockingInput() const
{
	const ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetController());
	return PlayerController && PlayerController->IsAnyModalPanelOpen();
}

bool ALSPlayerCharacter::IsFacingRotationLocked() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

	// 사망 상태에서는 마우스 조준·이동 방향 회전을 전부 잠근다.
	if (ASC && ASC->HasMatchingGameplayTag(LSGameplayTags::State_Dead))
	{
		return true;
	}

	// 스킬 시전(LS.Combat.SkillCasting)은 전 구간 회전 잠금.
	if (ASC && ASC->HasMatchingGameplayTag(LSGameplayTags::Combat_SkillCasting))
	{
		return true;
	}

	// 기본공격은 히트 판정 프레임(LSAN_PlayerMeleeHit)까지만 잠근다. 이후엔 다음 콤보 조준용 회전을 허용.
	return PlayerCombatComponent
		&& PlayerCombatComponent->IsAttackInProgress()
		&& !PlayerCombatComponent->IsBasicAttackHitConsumed();
}

void ALSPlayerCharacter::Move(const FInputActionValue& Value)
{
	// 이동 불가 소모품을 시전 중이면 이동 입력이 시전을 취소한다.
	if (bIsConsumableCasting && !bCastAllowsMove)
	{
		CancelConsumableCast();
	}

	// 스킬 몽타주 재생 중이면 입력 이동을 무시한다(루트모션 이동은 별도 경로라 영향 없음).
	if (IsInputBlocked())
	{
		LastMoveWorldDirection = FVector::ZeroVector;
		return;
	}

	const FVector2D Input = Value.Get<FVector2D>();

	FVector ForwardDirection = FollowCamera->GetForwardVector();
	ForwardDirection.Z = 0.0f;
	ForwardDirection.Normalize();

	FVector RightDirection = FollowCamera->GetRightVector();
	RightDirection.Z = 0.0f;
	RightDirection.Normalize();

	FVector MoveDirection = (RightDirection * Input.X) + (ForwardDirection * Input.Y);
	MoveDirection.Z = 0.0f;

	if (MoveDirection.IsNearlyZero())
	{
		LastMoveWorldDirection = FVector::ZeroVector;
		return;
	}

	LastMoveWorldDirection = MoveDirection.GetSafeNormal();
	AddMovementInput(LastMoveWorldDirection, FMath::Clamp(MoveDirection.Size(), 0.0f, 1.0f));
}

void ALSPlayerCharacter::FaceMovementDirection(float DeltaSeconds)
{
	FVector MoveDirection = GetVelocity();
	MoveDirection.Z = 0.0f;
	if (MoveDirection.IsNearlyZero())
	{
		return;
	}

	if (CameraBoom)
	{
		const FVector OffsetTarget = bEnableMouseCameraLead
			? MoveDirection.GetSafeNormal() * (MouseCameraLeadDistance * 0.6f)
			: FVector::ZeroVector;
		CameraBoom->TargetOffset = FMath::VInterpTo(CameraBoom->TargetOffset, OffsetTarget, DeltaSeconds, MouseCameraLeadInterpSpeed);
	}

	const FRotator TargetRotation = MoveDirection.Rotation();
	const FRotator NewRotation = FMath::RInterpTo(
		GetActorRotation(),
		FRotator(0.0f, TargetRotation.Yaw, 0.0f),
		DeltaSeconds,
		RunFacingInterpSpeed);

	ApplyFacingRotation(NewRotation);
}

bool ALSPlayerCharacter::ShouldSyncFacingRotation(float NewYaw) const
{
	if (!bHasSentFacingRotation)
	{
		return true;
	}

	if (!GetWorld())
	{
		return false;
	}

	const float TimeSinceLastSync = GetWorld()->GetTimeSeconds() - LastFacingSyncTime;
	if (TimeSinceLastSync < FacingSyncInterval)
	{
		return false;
	}

	const float YawDelta = FMath::Abs(FRotator::NormalizeAxis(NewYaw - LastSentFacingYaw));
	return YawDelta >= FacingSyncYawTolerance;
}

void ALSPlayerCharacter::ApplyRunState(bool bNewIsRunning)
{
	if (bNewIsRunning && !CanStartRunning())
	{
		bNewIsRunning = false;
	}

	bIsRunning = bNewIsRunning;

	RefreshMaxWalkSpeed();
}

void ALSPlayerCharacter::RefreshMaxWalkSpeed()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	// MoveSpeed 어트리뷰트 = 이동속도 배수(기본 1.0, 칩/장비로 가산). 0 이하 방지로 하한 클램프.
	const float Multiplier = PlayerAttributeSet ? FMath::Max(0.01f, PlayerAttributeSet->GetMoveSpeed()) : 1.0f;
	MovementComponent->MaxWalkSpeed = (bIsRunning ? RunSpeed : WalkSpeed) * Multiplier;
}

void ALSPlayerCharacter::HandleMoveSpeedChanged(const FOnAttributeChangeData& ChangeData)
{
	// 칩 장착·신호 게이지 변화 등으로 MoveSpeed 어트리뷰트가 바뀌면 이동속도를 즉시 재계산한다.
	RefreshMaxWalkSpeed();
}

bool ALSPlayerCharacter::CanStartRunning() const
{
	return HasStamina(KINDA_SMALL_NUMBER);
}

bool ALSPlayerCharacter::IsMovingForRunStaminaDrain() const
{
	FVector HorizontalVelocity = GetVelocity();
	HorizontalVelocity.Z = 0.0f;
	return !HorizontalVelocity.IsNearlyZero(1.0f);
}

void ALSPlayerCharacter::UpdateRunStamina(float DeltaSeconds)
{
	if (!HasAuthority() || !bIsRunning || RunStaminaDrainPerSecond <= 0.0f)
	{
		return;
	}

	if (!CanStartRunning())
	{
		ApplyRunState(false);
		ClientSetRunState(false);
		return;
	}

	if (!IsMovingForRunStaminaDrain())
	{
		return;
	}

	if (!TrySpendRunStamina(RunStaminaDrainPerSecond * DeltaSeconds))
	{
		ApplyRunState(false);
		ClientSetRunState(false);
	}
}

void ALSPlayerCharacter::UpdateStaminaRecovery(float DeltaSeconds)
{
	if (!HasAuthority() || !PlayerAttributeSet || StaminaRecoveryPerSecond <= 0.0f)
	{
		return;
	}

	if (bIsRunning || PlayerAttributeSet->GetCurrentStamina() >= PlayerAttributeSet->GetMaxStamina())
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() - LastStaminaSpendTime < StaminaRecoveryDelay)
	{
		return;
	}

	ApplyStaminaChange(StaminaRecoveryPerSecond * DeltaSeconds);
}

void ALSPlayerCharacter::UpdateHealthRecovery(float DeltaSeconds)
{
	if (!HasAuthority() || !PlayerAttributeSet)
	{
		return;
	}

	// Recovery 어트리뷰트 = 초당 회복 체력(HP/s). 칩/장비 합산으로 올라간다.
	const float RecoveryPerSecond = PlayerAttributeSet->GetRecovery();
	if (RecoveryPerSecond <= 0.0f)
	{
		return;
	}

	const ULSCombatAttributeSet* CombatSet = GetCombatAttributeSet();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!CombatSet || !ASC)
	{
		return;
	}

	// 사망 상태면 회복하지 않는다(음수/0 체력을 되살리지 않도록).
	if (const ULSCharacterCombatComponent* CombatComponent = GetCharacterCombatComponent())
	{
		if (CombatComponent->IsDead())
		{
			return;
		}
	}

	const float CurrentHealth = CombatSet->GetCurrentHealth();
	const float MaxHealth = CombatSet->GetMaxHealth();
	if (CurrentHealth >= MaxHealth)
	{
		return;
	}

	const float NewHealth = FMath::Min(CurrentHealth + RecoveryPerSecond * DeltaSeconds, MaxHealth);
	ASC->SetNumericAttributeBase(ULSCombatAttributeSet::GetCurrentHealthAttribute(), NewHealth);
}

bool ALSPlayerCharacter::HasStamina(float RequiredAmount) const
{
	return PlayerAttributeSet && PlayerAttributeSet->GetCurrentStamina() >= FMath::Max(RequiredAmount, 0.0f);
}

bool ALSPlayerCharacter::TrySpendStamina(float Amount)
{
	if (!HasAuthority() || Amount <= 0.0f || !HasStamina(Amount))
	{
		return false;
	}

	ApplyStaminaChange(-Amount);
	LastStaminaSpendTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	return true;
}

bool ALSPlayerCharacter::TrySpendRunStamina(const float Amount)
{
	if (!HasAuthority() || Amount <= 0.0f || !PlayerAttributeSet)
	{
		return false;
	}

	const float CurrentStamina = PlayerAttributeSet->GetCurrentStamina();
	const float SpendAmount = FMath::Min(CurrentStamina, Amount);
	if (SpendAmount <= 0.0f)
	{
		return false;
	}

	ApplyStaminaChange(-SpendAmount);
	LastStaminaSpendTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	return CurrentStamina > SpendAmount + KINDA_SMALL_NUMBER;
}

void ALSPlayerCharacter::ApplyStaminaChange(float Amount)
{
	if (!HasAuthority() || FMath::IsNearlyZero(Amount) || !StaminaChangeEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(StaminaChangeEffectClass, 1.0f, EffectContext);
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		UE_LOG(LogLS, Warning, TEXT("%s failed to create stamina change effect spec."), *GetNameSafe(this));
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(LSGameplayTags::Data_Stamina_Amount, Amount);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void ALSPlayerCharacter::InitializeSurvivalOverheadWidget()
{
	if (GetNetMode() == NM_DedicatedServer || !SurvivalOverheadWidgetComponent)
	{
		return;
	}

	SurvivalOverheadWidgetComponent->SetRelativeLocation(SurvivalOverheadWidgetOffset);
	SurvivalOverheadWidgetComponent->SetDrawSize(SurvivalOverheadDrawSize);

	if (SurvivalOverheadWidgetClass)
	{
		SurvivalOverheadWidgetComponent->SetWidgetClass(SurvivalOverheadWidgetClass);
	}

	if (!SurvivalOverheadWidgetComponent->GetWidgetClass())
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize survival overhead widget because SurvivalOverheadWidgetClass is not set."), *GetNameSafe(this));
		return;
	}

	SurvivalOverheadWidgetComponent->InitWidget();
	ULSSurvivalOverheadWidget* OverheadWidget = Cast<ULSSurvivalOverheadWidget>(SurvivalOverheadWidgetComponent->GetWidget());
	if (!OverheadWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("%s survival overhead widget is not derived from ULSSurvivalOverheadWidget."), *GetNameSafe(this));
		return;
	}

	OverheadWidget->InitializeSurvivalOverheadForCharacter(this);
}

void ALSPlayerCharacter::ServerSetRunState_Implementation(bool bNewIsRunning)
{
	ApplyRunState(bNewIsRunning);
	if (bNewIsRunning && !bIsRunning)
	{
		ClientSetRunState(false);
	}
}

void ALSPlayerCharacter::ClientSetRunState_Implementation(bool bNewIsRunning)
{
	ApplyRunState(bNewIsRunning);
}

void ALSPlayerCharacter::ServerRequestDash_Implementation(FVector_NetQuantizeNormal DashDirection)
{
	if (PlayerCombatComponent && HasStamina(DashStaminaCost) && PlayerCombatComponent->RequestDash(FVector(DashDirection)))
	{
		TrySpendStamina(DashStaminaCost);
	}
}

void ALSPlayerCharacter::ServerSyncFacingRotation_Implementation(float NewYaw)
{
	SetActorRotation(FRotator(0.0f, FRotator::NormalizeAxis(NewYaw), 0.0f));
}

FVector ALSPlayerCharacter::GetDashDirection() const
{
	FVector DashDirection = LastMoveWorldDirection;
	if (DashDirection.IsNearlyZero() && GetCharacterMovement())
	{
		DashDirection = GetCharacterMovement()->GetLastInputVector();
	}

	if (DashDirection.IsNearlyZero())
	{
		DashDirection = GetActorForwardVector();
	}

	DashDirection.Z = 0.0f;
	return DashDirection.IsNearlyZero() ? GetActorForwardVector() : DashDirection.GetSafeNormal();
}

bool ALSPlayerCharacter::ResolveMouseWorldPoint(FVector& OutMouseWorldPoint) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->IsLocalPlayerController())
	{
		return false;
	}

	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;
	if (!PlayerController->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return false;
	}

	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}

	const float TargetPlaneZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float RayDistance = (TargetPlaneZ - WorldOrigin.Z) / WorldDirection.Z;
	if (RayDistance < 0.0f)
	{
		return false;
	}

	OutMouseWorldPoint = WorldOrigin + (WorldDirection * RayDistance);
	return true;
}
