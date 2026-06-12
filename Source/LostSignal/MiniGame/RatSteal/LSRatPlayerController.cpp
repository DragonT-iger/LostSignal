#include "MiniGame/RatSteal/LSRatPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "MiniGame/RatSteal/LSRatGameMode.h"
#include "MiniGame/RatSteal/LSRatInventoryComponent.h"
#include "MiniGame/RatSteal/LSRatPlayer.h"
#include "MiniGame/RatSteal/LSRatStealSubsystem.h"

void ALSRatPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!RatMappingContext)
	{
		BuildDefaultInputAssets();
	}

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->AddMappingContext(RatMappingContext, 0);
		}
	}
}

void ALSRatPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!RatMappingContext)
	{
		BuildDefaultInputAssets();
	}

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		UE_LOG(LogLS, Warning, TEXT("[RatSteal] EnhancedInputComponent가 아님 — 입력 바인딩 실패"));
		return;
	}

	EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALSRatPlayerController::HandleMove);
	EIC->BindAction(StealAction, ETriggerEvent::Started, this, &ALSRatPlayerController::HandleSteal);
	EIC->BindAction(SlotNextAction, ETriggerEvent::Started, this, &ALSRatPlayerController::HandleSlotNext);
	EIC->BindAction(ThrowAction, ETriggerEvent::Triggered, this, &ALSRatPlayerController::HandleThrow);
	EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &ALSRatPlayerController::HandlePause);
	EIC->BindAction(ConfirmAction, ETriggerEvent::Started, this, &ALSRatPlayerController::HandleConfirm);
}

void ALSRatPlayerController::BuildDefaultInputAssets()
{
	// 원작 키 유지: 이동 WASD/방향키, Z 훔치기, X 슬롯, C 버리기, Esc 일시정지
	MoveAction = NewObject<UInputAction>(this, TEXT("IA_Rat_Move_Runtime"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;

	StealAction = NewObject<UInputAction>(this, TEXT("IA_Rat_Steal_Runtime"));
	SlotNextAction = NewObject<UInputAction>(this, TEXT("IA_Rat_SlotNext_Runtime"));
	ThrowAction = NewObject<UInputAction>(this, TEXT("IA_Rat_Throw_Runtime"));
	PauseAction = NewObject<UInputAction>(this, TEXT("IA_Rat_Pause_Runtime"));
	PauseAction->bTriggerWhenPaused = true; // 일시정지 중에도 Esc로 해제 가능해야 함

	ConfirmAction = NewObject<UInputAction>(this, TEXT("IA_Rat_Confirm_Runtime"));
	ConfirmAction->bTriggerWhenPaused = true; // 결과 화면은 일시정지 상태

	RatMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_RatSteal_Runtime"));

	auto MapMoveKey = [this](const FKey& Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = RatMappingContext->MapKey(MoveAction, Key);
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(this));
		}
		if (bSwizzle)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));
		}
	};

	// X=좌우, Y=상하 (Swizzle로 키 입력을 Y축으로)
	MapMoveKey(EKeys::D, false, false);
	MapMoveKey(EKeys::A, false, true);
	MapMoveKey(EKeys::W, true, false);
	MapMoveKey(EKeys::S, true, true);
	MapMoveKey(EKeys::Right, false, false);
	MapMoveKey(EKeys::Left, false, true);
	MapMoveKey(EKeys::Up, true, false);
	MapMoveKey(EKeys::Down, true, true);

	RatMappingContext->MapKey(StealAction, EKeys::Z);
	RatMappingContext->MapKey(SlotNextAction, EKeys::X);
	RatMappingContext->MapKey(ThrowAction, EKeys::C);
	RatMappingContext->MapKey(PauseAction, EKeys::Escape);
	RatMappingContext->MapKey(ConfirmAction, EKeys::Enter);
	RatMappingContext->MapKey(ConfirmAction, EKeys::SpaceBar);

	UE_LOG(LogLS, Log, TEXT("[RatSteal] IMC 에셋 미할당 — 런타임 기본 키 매핑 생성"));
}

void ALSRatPlayerController::HandleMove(const FInputActionValue& Value)
{
	if (ALSRatPlayer* RatPlayer = Cast<ALSRatPlayer>(GetPawn()))
	{
		RatPlayer->SetMoveInput(Value.Get<FVector2D>());
	}
}

void ALSRatPlayerController::HandleSteal(const FInputActionValue& Value)
{
	if (ALSRatPlayer* RatPlayer = Cast<ALSRatPlayer>(GetPawn()))
	{
		RatPlayer->TrySteal();
	}
}

void ALSRatPlayerController::HandleSlotNext(const FInputActionValue& Value)
{
	const ALSRatPlayer* RatPlayer = Cast<ALSRatPlayer>(GetPawn());
	if (RatPlayer && RatPlayer->GetInventory())
	{
		RatPlayer->GetInventory()->ChangeSlot();
	}
}

void ALSRatPlayerController::HandleThrow(const FInputActionValue& Value)
{
	// 원작 PlayerController: 첫 입력 즉시 + 홀드 시 throwTime(0.2s) 간격 반복
	const float Now = GetWorld()->GetTimeSeconds();
	if (LastThrowTime >= 0.f && Now - LastThrowTime < ThrowInterval)
	{
		return;
	}

	const ALSRatPlayer* RatPlayer = Cast<ALSRatPlayer>(GetPawn());
	if (RatPlayer && RatPlayer->GetInventory())
	{
		RatPlayer->GetInventory()->ThrowItem();
		LastThrowTime = Now;
	}
}

void ALSRatPlayerController::HandlePause(const FInputActionValue& Value)
{
	ALSRatGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSRatGameMode>();
	if (GameMode && GameMode->GetPhase() != ELSRatPhase::End)
	{
		GameMode->TogglePause();
	}
}

void ALSRatPlayerController::HandleConfirm(const FInputActionValue& Value)
{
	const ALSRatGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSRatGameMode>();
	if (!GameMode || GameMode->GetPhase() != ELSRatPhase::End)
	{
		return;
	}

	UGameplayStatics::SetGamePaused(this, false);
	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULSRatStealSubsystem* Subsystem = GI->GetSubsystem<ULSRatStealSubsystem>())
		{
			Subsystem->ReturnToMainWorld();
		}
	}
}
