#include "UI/Title/LSTitleMenuWidget.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSettings.h"
#include "UI/Title/LSTitleMenuButtonWidget.h"

void ULSTitleMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddDynamic(this, &ULSTitleMenuWidget::HandleContinueClicked);

		// 세이브가 없으면 Continue 비활성화.
		const ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
		const bool bHasSave = SaveSubsystem && SaveSubsystem->HasExistingSave();
		ContinueButton->SetButtonEnabled(bHasSave);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("ContinueButton is not bound on %s."), *GetNameSafe(this));
	}

	if (NewButton)
	{
		NewButton->OnClicked.AddDynamic(this, &ULSTitleMenuWidget::HandleNewClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("NewButton is not bound on %s."), *GetNameSafe(this));
	}

	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddDynamic(this, &ULSTitleMenuWidget::HandleSettingsClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("SettingsButton is not bound on %s."), *GetNameSafe(this));
	}

	if (CrewButton)
	{
		CrewButton->OnClicked.AddDynamic(this, &ULSTitleMenuWidget::HandleCrewClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("CrewButton is not bound on %s."), *GetNameSafe(this));
	}

	if (ExitButton)
	{
		ExitButton->OnClicked.AddDynamic(this, &ULSTitleMenuWidget::HandleExitClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("ExitButton is not bound on %s."), *GetNameSafe(this));
	}
}

void ULSTitleMenuWidget::NativeDestruct()
{
	if (ContinueButton)
	{
		ContinueButton->OnClicked.RemoveDynamic(this, &ULSTitleMenuWidget::HandleContinueClicked);
	}
	if (NewButton)
	{
		NewButton->OnClicked.RemoveDynamic(this, &ULSTitleMenuWidget::HandleNewClicked);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &ULSTitleMenuWidget::HandleSettingsClicked);
	}
	if (CrewButton)
	{
		CrewButton->OnClicked.RemoveDynamic(this, &ULSTitleMenuWidget::HandleCrewClicked);
	}
	if (ExitButton)
	{
		ExitButton->OnClicked.RemoveDynamic(this, &ULSTitleMenuWidget::HandleExitClicked);
	}

	Super::NativeDestruct();
}

void ULSTitleMenuWidget::HandleContinueClicked()
{
	const ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem || !SaveSubsystem->HasExistingSave())
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] Continue clicked but no existing save."));
		return;
	}

	UE_LOG(LogLS, Log, TEXT("[Title] Continue - opening lobby."));
	OpenLobbyLevel();
}

void ULSTitleMenuWidget::HandleNewClicked()
{
	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] New clicked but SaveSubsystem is missing."));
		return;
	}

	UE_LOG(LogLS, Log, TEXT("[Title] New game - resetting save and opening lobby."));
	SaveSubsystem->StartNewGame();
	OpenLobbyLevel();
}

void ULSTitleMenuWidget::HandleSettingsClicked()
{
	// 미구현: Settings 화면 준비되면 연결.
	UE_LOG(LogLS, Warning, TEXT("[Title] Settings is not implemented yet."));
}

void ULSTitleMenuWidget::HandleCrewClicked()
{
	// 미구현: Crew 화면 준비되면 연결.
	UE_LOG(LogLS, Warning, TEXT("[Title] Crew is not implemented yet."));
}

void ULSTitleMenuWidget::HandleExitClicked()
{
	UE_LOG(LogLS, Log, TEXT("[Title] Exit - quitting game."));
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

ULSSaveSubsystem* ULSTitleMenuWidget::GetSaveSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}
	return GameInstance->GetSubsystem<ULSSaveSubsystem>();
}

void ULSTitleMenuWidget::OpenLobbyLevel()
{
	const ULSSessionSettings* Settings = GetDefault<ULSSessionSettings>();
	if (!Settings || Settings->LobbyLevel.IsNull())
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] LobbyLevel is not set. Check Project Settings > LS Session Settings."));
		return;
	}

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Settings->LobbyLevel);
}
