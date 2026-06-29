#include "Core/LSTitleGameMode.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "UI/Title/LSTitleMenuWidget.h"

ALSTitleGameMode::ALSTitleGameMode()
{
	// 타이틀은 조작할 폰이 없다. 기본 폰 스폰을 막는다.
	DefaultPawnClass = nullptr;
}

void ALSTitleGameMode::BeginPlay()
{
	Super::BeginPlay();
	CreateTitleMenuWidget();
}

void ALSTitleGameMode::CreateTitleMenuWidget()
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] Cannot create title menu because PlayerController is missing."));
		return;
	}

	if (!TitleMenuWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] TitleMenuWidgetClass is not set on %s. Check BP_TitleGameMode."), *GetNameSafe(this));
		return;
	}

	TitleMenuWidgetInstance = CreateWidget<ULSTitleMenuWidget>(PlayerController, TitleMenuWidgetClass);
	if (!TitleMenuWidgetInstance)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] Failed to create title menu widget on %s."), *GetNameSafe(this));
		return;
	}

	TitleMenuWidgetInstance->AddToViewport();

	PlayerController->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
}
