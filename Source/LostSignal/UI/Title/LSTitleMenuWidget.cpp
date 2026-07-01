#include "UI/Title/LSTitleMenuWidget.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSettings.h"
#include "UI/Common/LSConfirmDialogWidget.h"
#include "UI/LSUILayer.h"
#include "UI/Settings/LSSettingsWidget.h"
#include "UI/Title/LSTitleMenuButtonWidget.h"

#define LOCTEXT_NAMESPACE "LSTitleMenu"

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
	// 데이터가 삭제되므로 확인 다이얼로그를 거친다.
	ULSConfirmDialogWidget* Dialog = ShowConfirmDialog(
		LOCTEXT("NewGameConfirm", "저장된 데이터가 삭제됩니다. 계속 진행하시겠습니까?"));
	if (Dialog)
	{
		Dialog->OnConfirmed.AddDynamic(this, &ULSTitleMenuWidget::HandleNewConfirmed);
	}
}

void ULSTitleMenuWidget::HandleNewConfirmed()
{
	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] New confirmed but SaveSubsystem is missing."));
		return;
	}

	UE_LOG(LogLS, Log, TEXT("[Title] New game confirmed - resetting save and opening lobby."));
	SaveSubsystem->StartNewGame();
	OpenLobbyLevel();
}

void ULSTitleMenuWidget::HandleSettingsClicked()
{
	ShowSettingsWidget();
}

void ULSTitleMenuWidget::HandleCrewClicked()
{
	// 미구현: Crew 화면 준비되면 연결. 그전까지는 안내창만 띄운다.
	ULSConfirmDialogWidget* Dialog = ShowConfirmDialog(LOCTEXT("NotImplemented", "아직 구현되지 않았습니다."));
	if (Dialog)
	{
		// 확인/취소 어느 쪽을 눌러도(또는 ESC) 그냥 닫힌다.
		Dialog->OnConfirmed.AddDynamic(this, &ULSTitleMenuWidget::HandleDialogCancelled);
	}
}

void ULSTitleMenuWidget::HandleExitClicked()
{
	// 종료 전에도 같은 확인 다이얼로그를 거친다.
	ULSConfirmDialogWidget* Dialog = ShowConfirmDialog(
		LOCTEXT("ExitConfirm", "게임을 종료하시겠습니까?"));
	if (Dialog)
	{
		Dialog->OnConfirmed.AddDynamic(this, &ULSTitleMenuWidget::HandleExitConfirmed);
	}
}

void ULSTitleMenuWidget::HandleExitConfirmed()
{
	UE_LOG(LogLS, Log, TEXT("[Title] Exit confirmed - quitting game."));
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void ULSTitleMenuWidget::HandleDialogCancelled()
{
	UE_LOG(LogLS, Log, TEXT("[Title] Confirm dialog cancelled."));
	ActiveConfirmDialog = nullptr;
}

void ULSTitleMenuWidget::HandleSettingsBackToMenu()
{
	ActiveSettingsWidget = nullptr;
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

ULSConfirmDialogWidget* ULSTitleMenuWidget::ShowConfirmDialog(const FText& Message)
{
	// 이미 다이얼로그가 떠 있으면 중복 생성하지 않는다.
	if (ActiveConfirmDialog && ActiveConfirmDialog->IsInViewport())
	{
		return nullptr;
	}

	if (!ConfirmDialogClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] ConfirmDialogClass is not set on %s. Check WBP_TitleMenu."), *GetNameSafe(this));
		return nullptr;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	ULSConfirmDialogWidget* Dialog = OwningPlayer
		? CreateWidget<ULSConfirmDialogWidget>(OwningPlayer, ConfirmDialogClass)
		: CreateWidget<ULSConfirmDialogWidget>(this, ConfirmDialogClass);
	if (!Dialog)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] Failed to create confirm dialog on %s."), *GetNameSafe(this));
		return nullptr;
	}

	Dialog->SetMessage(Message);
	Dialog->OnCancelled.AddDynamic(this, &ULSTitleMenuWidget::HandleDialogCancelled);
	Dialog->AddToViewport(100);
	ActiveConfirmDialog = Dialog;
	return Dialog;
}

ULSSettingsWidget* ULSTitleMenuWidget::ShowSettingsWidget()
{
	// 이미 세팅 화면이 떠 있으면 중복 생성하지 않는다.
	if (ActiveSettingsWidget && ActiveSettingsWidget->IsInViewport())
	{
		return nullptr;
	}

	if (!SettingsWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] SettingsWidgetClass is not set on %s. Check WBP_TitleMenu."), *GetNameSafe(this));
		return nullptr;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	ULSSettingsWidget* SettingsWidget = OwningPlayer
		? CreateWidget<ULSSettingsWidget>(OwningPlayer, SettingsWidgetClass)
		: CreateWidget<ULSSettingsWidget>(this, SettingsWidgetClass);
	if (!SettingsWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] Failed to create settings widget on %s."), *GetNameSafe(this));
		return nullptr;
	}

	SettingsWidget->OnBackToMenu.AddDynamic(this, &ULSTitleMenuWidget::HandleSettingsBackToMenu);
	// 타이틀에서는 이미 메인메뉴이므로 "메인메뉴로 돌아가기" 버튼을 숨긴다.
	SettingsWidget->SetMainMenuButtonVisible(false);
	SettingsWidget->AddToViewport(LSUILayer::Settings);
	ActiveSettingsWidget = SettingsWidget;
	return SettingsWidget;
}

#undef LOCTEXT_NAMESPACE
