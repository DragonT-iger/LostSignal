// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/LSPlayerControllerBase.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"

void ALSPlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	// 마우스 커서 활성화 + GameAndUI 모드 (UI 클릭 + 게임플레이 동시 가능)
	bShowMouseCursor = true;
	bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	// 뷰포트에 포커스 — UI와 게임 입력이 모두 동작하도록
	UWidgetBlueprintLibrary::SetFocusToGameViewport();
}

void ALSPlayerControllerBase::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	// EnhancedInput 서브시스템에 IMC 등록
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* IMC : DefaultMappingContexts)
		{
			if (IMC)
			{
				Subsystem->AddMappingContext(IMC, 0);
			}
		}
	}
}
