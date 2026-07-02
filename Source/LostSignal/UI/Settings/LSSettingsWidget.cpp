#include "UI/Settings/LSSettingsWidget.h"

#include "Components/Button.h"
#include "Core/LSFarmingGameMode.h"
#include "Core/LSPlayerControllerBase.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "Session/LSSessionSettings.h"
#include "Session/LSSessionSubsystem.h"
#include "UI/Common/LSConfirmDialogWidget.h"
#include "UI/LSUILayer.h"
#include "UI/Settings/LSSoundSettingsWidget.h"
#include "UI/Title/LSTitleMenuButtonWidget.h"

#define LOCTEXT_NAMESPACE "LSSettings"

void ULSSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SoundButton)
	{
		SoundButton->OnClicked.AddDynamic(this, &ULSSettingsWidget::HandleSoundClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("SoundButton is not bound on %s."), *GetNameSafe(this));
	}

	if (ControlButton)
	{
		ControlButton->OnClicked.AddDynamic(this, &ULSSettingsWidget::HandleControlClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("ControlButton is not bound on %s."), *GetNameSafe(this));
	}

	if (GraphicsButton)
	{
		GraphicsButton->OnClicked.AddDynamic(this, &ULSSettingsWidget::HandleGraphicsClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("GraphicsButton is not bound on %s."), *GetNameSafe(this));
	}

	if (LanguageButton)
	{
		LanguageButton->OnClicked.AddDynamic(this, &ULSSettingsWidget::HandleLanguageClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("LanguageButton is not bound on %s."), *GetNameSafe(this));
	}

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddDynamic(this, &ULSSettingsWidget::HandleMainMenuClicked);
		// 타이틀에서 열렸으면 이 시점 이전에 SetMainMenuButtonVisible(false)가 호출돼 있으므로 반영한다.
		MainMenuButton->SetVisibility(bMainMenuButtonVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("MainMenuButton is not bound on %s."), *GetNameSafe(this));
	}

	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &ULSSettingsWidget::HandleBackClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("BackButton is not bound on %s."), *GetNameSafe(this));
	}

	// ESC로 스스로 닫을 수 있도록 키보드 포커스를 가져온다. (마우스 조작은 포커스와 무관하게 동작)
	SetIsFocusable(true);
	SetKeyboardFocus();
}

void ULSSettingsWidget::NativeDestruct()
{
	if (SoundButton)
	{
		SoundButton->OnClicked.RemoveDynamic(this, &ULSSettingsWidget::HandleSoundClicked);
	}
	if (ControlButton)
	{
		ControlButton->OnClicked.RemoveDynamic(this, &ULSSettingsWidget::HandleControlClicked);
	}
	if (GraphicsButton)
	{
		GraphicsButton->OnClicked.RemoveDynamic(this, &ULSSettingsWidget::HandleGraphicsClicked);
	}
	if (LanguageButton)
	{
		LanguageButton->OnClicked.RemoveDynamic(this, &ULSSettingsWidget::HandleLanguageClicked);
	}
	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.RemoveDynamic(this, &ULSSettingsWidget::HandleMainMenuClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &ULSSettingsWidget::HandleBackClicked);
	}

	Super::NativeDestruct();
}

void ULSSettingsWidget::HandleSoundClicked()
{
	if (ActiveSoundWidget && ActiveSoundWidget->IsInViewport())
	{
		return;
	}

	if (!SoundSettingsWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Settings] SoundSettingsWidgetClass is not set on %s. Check WBP_Settings."), *GetNameSafe(this));
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	ULSSoundSettingsWidget* SoundWidget = OwningPlayer
		? CreateWidget<ULSSoundSettingsWidget>(OwningPlayer, SoundSettingsWidgetClass)
		: CreateWidget<ULSSoundSettingsWidget>(this, SoundSettingsWidgetClass);
	if (!SoundWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("[Settings] Failed to create sound settings widget on %s."), *GetNameSafe(this));
		return;
	}

	// 사운드로 들어가는 동안 세팅 화면은 숨긴다. 사운드가 닫히면(OnClosed) 다시 보이게 한다.
	SoundWidget->OnClosed.AddDynamic(this, &ULSSettingsWidget::HandleSoundClosed);
	SoundWidget->AddToViewport(LSUILayer::SettingsSubPanel);
	ActiveSoundWidget = SoundWidget;

	SetVisibility(ESlateVisibility::Collapsed);
}

void ULSSettingsWidget::HandleSoundClosed()
{
	// 사운드 화면이 닫혔으니 세팅 화면을 다시 보이게 하고, ESC가 다시 세팅으로 오도록 포커스를 되찾는다.
	SetVisibility(ESlateVisibility::Visible);
	SetKeyboardFocus();
	ActiveSoundWidget = nullptr;
}

void ULSSettingsWidget::HandleControlClicked()
{
	// 미구현: Control 화면 준비되면 연결. 그전까지는 안내창만 띄운다.
	ShowNotImplementedNotice();
}

void ULSSettingsWidget::HandleGraphicsClicked()
{
	// 미구현: Graphics 화면 준비되면 연결. 그전까지는 안내창만 띄운다.
	ShowNotImplementedNotice();
}

void ULSSettingsWidget::HandleLanguageClicked()
{
	// 미구현: Language 화면 준비되면 연결. 그전까지는 안내창만 띄운다.
	ShowNotImplementedNotice();
}

void ULSSettingsWidget::HandleMainMenuClicked()
{
	// "메인메뉴로 돌아가기"는 레이드 여부와 무관하게 타이틀로 나간다.
	// 단, 레이드 중이면 파밍 성과를 포기하는 행동이라 확인 다이얼로그를 먼저 거친다.
	if (IsRaidActive())
	{
		ShowReturnToTitleConfirmDialog();
		return;
	}

	// 레이드 중이 아니면(타이틀/로비) 바로 타이틀 레벨로 이동한다.
	TravelToTitle();
}

void ULSSettingsWidget::HandleBackClicked()
{
	CloseSettings();
}

FReply ULSSettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		CloseSettings();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULSSettingsWidget::CloseSettings()
{
	// 세팅 패널만 닫는다. 밑에 있던 화면(타이틀/로비 메뉴, 레이드 게임)이 그대로 다시 보인다.
	// 레이드에서는 OnBackToMenu 구독자(PlayerController)가 캐시를 비워 다음 ESC에 다시 생성한다.
	OnBackToMenu.Broadcast();
	RemoveFromParent();
}

void ULSSettingsWidget::SetMainMenuButtonVisible(bool bVisible)
{
	bMainMenuButtonVisible = bVisible;
	// NativeConstruct 전/후 어느 시점에 호출돼도 반영되도록, 바인딩돼 있으면 즉시 적용한다.
	if (MainMenuButton)
	{
		MainMenuButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void ULSSettingsWidget::HandleReturnToTitleConfirmed()
{
	if (ULSSessionSubsystem* SessionSub = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULSSessionSubsystem>() : nullptr)
	{
		// 타이틀 복귀는 파밍 성과를 포기하는 대신 항상 출발 장비를 복구한다.
		SessionSub->bAllowQuitRecovery = true;
	}

	if (ALSFarmingGameMode* FarmingGameMode = Cast<ALSFarmingGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		FarmingGameMode->OnQuit();
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Settings] Failed to quit raid: ALSFarmingGameMode not found."));
	}
}

void ULSSettingsWidget::HandleDialogCancelled()
{
	ActiveConfirmDialog = nullptr;
	// 다이얼로그가 닫혔으니 ESC가 다시 세팅으로 오도록 포커스를 회수한다.
	SetKeyboardFocus();
}

bool ULSSettingsWidget::IsRaidActive() const
{
	const ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	const ULSRaidInventoryComponent* RaidInventory = PlayerController ? PlayerController->GetRaidInventoryComponent() : nullptr;
	return RaidInventory && RaidInventory->IsRaidActive();
}

ULSConfirmDialogWidget* ULSSettingsWidget::ShowReturnToTitleConfirmDialog()
{
	ULSConfirmDialogWidget* Dialog = CreateDialog(
		LOCTEXT("ReturnToTitleConfirm", "레이드를 포기하고 타이틀로 돌아가시겠습니까? 파밍한 아이템은 사라지고 출발 장비만 복구됩니다."));
	if (!Dialog)
	{
		return nullptr;
	}

	Dialog->OnConfirmed.AddDynamic(this, &ULSSettingsWidget::HandleReturnToTitleConfirmed);
	Dialog->OnCancelled.AddDynamic(this, &ULSSettingsWidget::HandleDialogCancelled);
	return Dialog;
}

void ULSSettingsWidget::ShowNotImplementedNotice()
{
	ULSConfirmDialogWidget* Dialog = CreateDialog(LOCTEXT("NotImplemented", "아직 <Emph>구현</>되지 않았습니다."));
	if (!Dialog)
	{
		return;
	}

	// 확인/취소 어느 쪽을 눌러도(또는 ESC) 그냥 닫히고 세팅으로 돌아온다.
	Dialog->OnConfirmed.AddDynamic(this, &ULSSettingsWidget::HandleDialogCancelled);
	Dialog->OnCancelled.AddDynamic(this, &ULSSettingsWidget::HandleDialogCancelled);
}

ULSConfirmDialogWidget* ULSSettingsWidget::CreateDialog(const FText& Message)
{
	// 이미 다이얼로그가 떠 있으면 중복 생성하지 않는다.
	if (ActiveConfirmDialog && ActiveConfirmDialog->IsInViewport())
	{
		return nullptr;
	}

	if (!ConfirmDialogClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Settings] ConfirmDialogClass is not set on %s. Check WBP_Settings."), *GetNameSafe(this));
		return nullptr;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	ULSConfirmDialogWidget* Dialog = OwningPlayer
		? CreateWidget<ULSConfirmDialogWidget>(OwningPlayer, ConfirmDialogClass)
		: CreateWidget<ULSConfirmDialogWidget>(this, ConfirmDialogClass);
	if (!Dialog)
	{
		UE_LOG(LogLS, Warning, TEXT("[Settings] Failed to create confirm dialog on %s."), *GetNameSafe(this));
		return nullptr;
	}

	Dialog->SetMessage(Message);
	Dialog->AddToViewport(LSUILayer::SettingsSubPanel);
	ActiveConfirmDialog = Dialog;
	return Dialog;
}

void ULSSettingsWidget::TravelToTitle()
{
	const ULSSessionSettings* Settings = GetDefault<ULSSessionSettings>();
	if (!Settings || Settings->TitleLevel.IsNull())
	{
		UE_LOG(LogLS, Warning, TEXT("[Settings] TitleLevel is not set. Check Project Settings > LS Session Settings."));
		return;
	}

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Settings->TitleLevel);
}

#undef LOCTEXT_NAMESPACE
