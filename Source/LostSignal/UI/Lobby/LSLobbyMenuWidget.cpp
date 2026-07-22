#include "UI/Lobby/LSLobbyMenuWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Core/LSLobbyGameMode.h"
#include "Core/LSPlayerControllerBase.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "LostSignal.h"
#include "TimerManager.h"
#include "UI/Common/LSConfirmDialogWidget.h"
#include "UI/LSUILayer.h"
#include "UI/Lobby/LSLoadoutPreparationWidget.h"
#include "UI/Lobby/LSLobbyQuestWidget.h"
#include "UI/Lobby/LSLobbyTabWidget.h"
#include "UI/Settings/LSSettingsWidget.h"

#define LOCTEXT_NAMESPACE "LSLobbyMenu"

void ULSLobbyMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// TAB 키 입력을 받기 위해 포커스 가능하게 둔다. 실제 포커스는 GameMode가 SetWidgetToFocus로 준다.
	SetIsFocusable(true);

	// 상단 탭 4개 모두 클릭을 구독해 해당 페이지로 전환한다.
	if (PlayTab)
	{
		PlayTab->OnClicked.AddDynamic(this, &ULSLobbyMenuWidget::HandlePlayTabClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("PlayTab is not bound on %s."), *GetNameSafe(this));
	}

	// 개인정비 탭은 WidgetSwitcher를 개인정비(WBP_LoadoutPreparation) 페이지로 전환한다.
	if (LoadoutPreparation)
	{
		LoadoutPreparation->OnClicked.AddDynamic(this, &ULSLobbyMenuWidget::HandleLoadoutPreparationClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("LoadoutPreparation is not bound on %s."), *GetNameSafe(this));
	}
	if (QuestTab)
	{
		QuestTab->OnClicked.AddDynamic(this, &ULSLobbyMenuWidget::HandleQuestTabClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("QuestTab is not bound on %s."), *GetNameSafe(this));
	}
	if (CharacterTab)
	{
		CharacterTab->OnClicked.AddDynamic(this, &ULSLobbyMenuWidget::HandleCharacterTabClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("CharacterTab is not bound on %s."), *GetNameSafe(this));
	}

	if (MissionStartButton)
	{
		MissionStartButton->OnClicked.AddDynamic(this, &ULSLobbyMenuWidget::HandleMissionStartClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("MissionStartButton is not bound on %s."), *GetNameSafe(this));
	}

	if (!TabSwitcher)
	{
		UE_LOG(LogLS, Warning, TEXT("TabSwitcher is not bound on %s."), *GetNameSafe(this));
	}
	if (!WBP_LoadoutPreparation)
	{
		UE_LOG(LogLS, Warning, TEXT("WBP_LoadoutPreparation is not bound on %s."), *GetNameSafe(this));
	}
	if (!LobbyQuest)
	{
		UE_LOG(LogLS, Warning, TEXT("LobbyQuest is not bound on %s."), *GetNameSafe(this));
	}
	if (!LevelText)
	{
		UE_LOG(LogLS, Warning, TEXT("LevelText is not bound on %s."), *GetNameSafe(this));
	}
	if (!LevelProgressBar)
	{
		UE_LOG(LogLS, Warning, TEXT("LevelProgressBar is not bound on %s."), *GetNameSafe(this));
	}
	if (!PlayerNameText)
	{
		UE_LOG(LogLS, Warning, TEXT("PlayerNameText is not bound on %s."), *GetNameSafe(this));
	}
	if (!CharacterPortraitButton)
	{
		UE_LOG(LogLS, Warning, TEXT("CharacterPortraitButton is not bound on %s."), *GetNameSafe(this));
	}
	if (!BackButton)
	{
		UE_LOG(LogLS, Warning, TEXT("BackButton is not bound on %s."), *GetNameSafe(this));
	}
	// 인벤토리 버튼: TAB 키와 동일하게 인벤토리(창고+인벤토리) 페이지를 토글한다.
	if (InventoryButton)
	{
		InventoryButton->OnClicked.AddDynamic(this, &ULSLobbyMenuWidget::HandleInventoryButtonClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("InventoryButton is not bound on %s."), *GetNameSafe(this));
	}

	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddDynamic(this, &ULSLobbyMenuWidget::HandleSettingsClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("SettingsButton is not bound on %s."), *GetNameSafe(this));
	}

	// 개인정비 페이지에서 배경을 교체하기 위해 WBP 기본 배경 브러시를 캐시한다.
	if (BackgroundImage)
	{
		DefaultBackgroundBrush = BackgroundImage->GetBrush();
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("BackgroundImage is not bound on %s."), *GetNameSafe(this));
	}

	// 로비는 플레이 탭에서 시작한다.
	ShowTab(ELSLobbyTab::Play);
}

void ULSLobbyMenuWidget::NativeDestruct()
{
	if (PlayTab)
	{
		PlayTab->OnClicked.RemoveDynamic(this, &ULSLobbyMenuWidget::HandlePlayTabClicked);
	}
	if (LoadoutPreparation)
	{
		LoadoutPreparation->OnClicked.RemoveDynamic(this, &ULSLobbyMenuWidget::HandleLoadoutPreparationClicked);
	}
	if (QuestTab)
	{
		QuestTab->OnClicked.RemoveDynamic(this, &ULSLobbyMenuWidget::HandleQuestTabClicked);
	}
	if (CharacterTab)
	{
		CharacterTab->OnClicked.RemoveDynamic(this, &ULSLobbyMenuWidget::HandleCharacterTabClicked);
	}
	if (InventoryButton)
	{
		InventoryButton->OnClicked.RemoveDynamic(this, &ULSLobbyMenuWidget::HandleInventoryButtonClicked);
	}
	if (MissionStartButton)
	{
		MissionStartButton->OnClicked.RemoveDynamic(this, &ULSLobbyMenuWidget::HandleMissionStartClicked);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &ULSLobbyMenuWidget::HandleSettingsClicked);
	}

	Super::NativeDestruct();
}

void ULSLobbyMenuWidget::SetLevelText(const FText& NewLevelText) const
{
	if (!LevelText)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set level text because LevelText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	LevelText->SetText(NewLevelText);
}

void ULSLobbyMenuWidget::SetLevelProgress(const float Progress) const
{
	if (!LevelProgressBar)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set level progress because LevelProgressBar is not bound on %s."), *GetNameSafe(this));
		return;
	}

	LevelProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
}

void ULSLobbyMenuWidget::SetPlayerName(const FText& NewPlayerName) const
{
	if (!PlayerNameText)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set player name because PlayerNameText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	PlayerNameText->SetText(NewPlayerName);
}

void ULSLobbyMenuWidget::HandlePlayTabClicked()
{
	CloseNotImplementedNotice();
	ShowTab(ELSLobbyTab::Play);
}

void ULSLobbyMenuWidget::HandleLoadoutPreparationClicked()
{
	CloseNotImplementedNotice();
	// 상단 탭으로 진입할 때는 항상 내부 탭 목록부터 보여준다.
	if (WBP_LoadoutPreparation)
	{
		WBP_LoadoutPreparation->ResetToTabs();
	}
	ShowTab(ELSLobbyTab::LoadoutPreparation);
}

void ULSLobbyMenuWidget::HandleQuestTabClicked()
{
	// 안내창이 이미 떠 있으면 닫기만 한다(토글). 없을 때만 플레이 페이지를 유지한 채 미구현 안내창을 띄운다.
	if (CloseNotImplementedNotice())
	{
		return;
	}
	ShowTab(ELSLobbyTab::Play);
	ShowNotImplementedNotice();
}

void ULSLobbyMenuWidget::HandleCharacterTabClicked()
{
	// 안내창이 이미 떠 있으면 닫기만 한다(토글). 없을 때만 플레이 페이지를 유지한 채 미구현 안내창을 띄운다.
	if (CloseNotImplementedNotice())
	{
		return;
	}
	ShowTab(ELSLobbyTab::Play);
	ShowNotImplementedNotice();
}

void ULSLobbyMenuWidget::HandleInventoryButtonClicked()
{
	ToggleStoragePage();
}

void ULSLobbyMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// TAB/ESC는 이 위젯의 키보드 포커스에 의존하는데, PlayerControllerBase::BeginPlay의
	// SetFocusToGameViewport 등 포커스가 뷰포트로 새는 경로가 여럿이라 매 틱 회수한다.
	// 계약: 로비에서 메뉴 트리 밖에 뜨는 포커스 위젯은 ActiveSettingsWidget/ActiveConfirmDialog로
	// 추적한다 — 새 외부 모달을 추가하면 이 가드에 합류시켜야 포커스를 뺏지 않는다.
	// (포커스를 이미 쥐고 있으면 회수 자체가 불필요하니, 외부 모달 판정보다 포커스 보유를 먼저 본다 —
	//  LoadoutPreparation::HasActiveConfirmDialog가 매 틱 위젯 트리를 훑지 않게 하는 최적화.)
	if (!HasKeyboardFocus() && !HasFocusedDescendants())
	{
		const bool bExternalFocusWidgetOpen =
			(ActiveSettingsWidget && ActiveSettingsWidget->IsInViewport()) ||
			(ActiveConfirmDialog && ActiveConfirmDialog->IsInViewport()) ||
			(WBP_LoadoutPreparation && WBP_LoadoutPreparation->HasActiveConfirmDialog());
		if (!bExternalFocusWidgetOpen)
		{
			SetKeyboardFocus();
		}
	}
}

FReply ULSLobbyMenuWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// 포커스가 버튼으로 넘어가도 터널링 단계라 루트가 먼저 받는다. TAB은 여기서 소비해 포커스 이동을 막는다.
	if (InKeyEvent.GetKey() == EKeys::Tab)
	{
		ToggleStoragePage();
		return FReply::Handled();
	}

	// 로비는 InputModeUIOnly라 PlayerController의 BindKey(Insert)까지 입력이 내려오지 않는다.
	// 여기서 직접 프로토콜 디버그 패널을 토글한다.
	if (InKeyEvent.GetKey() == EKeys::Insert)
	{
		if (ALSPlayerControllerBase* LSPlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
		{
			LSPlayerController->ToggleProtocolDebugWidget();
		}
		return FReply::Handled();
	}

	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (TabSwitcher && TabSwitcher->GetActiveWidgetIndex() != static_cast<int32>(ELSLobbyTab::Play))
		{
			// 개인정비의 가장 안쪽 화면부터 한 단계씩 닫고, 탭 목록에서는 플레이 페이지로 돌아온다.
			const bool bLoadoutActive =
				TabSwitcher->GetActiveWidgetIndex() == static_cast<int32>(ELSLobbyTab::LoadoutPreparation);
			const bool bHandledLoadoutBack =
				bLoadoutActive && WBP_LoadoutPreparation && WBP_LoadoutPreparation->TryHandleBack();
			if (!bHandledLoadoutBack)
			{
				ShowTab(ELSLobbyTab::Play);
			}
			return FReply::Handled();
		}

		if (ActiveSettingsWidget && ActiveSettingsWidget->IsInViewport())
		{
			ActiveSettingsWidget->SetKeyboardFocus();
			return FReply::Handled();
		}

		ShowSettingsWidget();
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply ULSLobbyMenuWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 여기까지 왔다는 것은 어떤 자식도 클릭을 처리하지 않았다는 뜻(버블링 종점). 포커스가 뷰포트로
	// 빠지면 TAB/ESC가 죽으므로 빈 영역 클릭은 소비하면서 포커스를 루트로 되돌린다.
	Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
}

void ULSLobbyMenuWidget::ToggleStoragePage() const
{
	if (!TabSwitcher)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot toggle storage because TabSwitcher is not bound on %s."), *GetNameSafe(this));
		return;
	}
	if (!WBP_LoadoutPreparation)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot toggle storage because WBP_LoadoutPreparation is not bound on %s."), *GetNameSafe(this));
		return;
	}

	// 개인정비에서는 자판기/제작대 같은 가장 안쪽 화면부터 한 단계씩 되돌린다.
	const bool bLoadoutActive =
		TabSwitcher->GetActiveWidgetIndex() == static_cast<int32>(ELSLobbyTab::LoadoutPreparation);
	if (bLoadoutActive && WBP_LoadoutPreparation->TryHandleBack())
	{
		return;
	}

	// 플레이 등 다른 페이지나 개인정비 탭 목록에서는 물품창고로 바로 진입한다.
	WBP_LoadoutPreparation->OpenTab(ELSLoadoutTab::Storage);
	ShowTab(ELSLobbyTab::LoadoutPreparation);
}

void ULSLobbyMenuWidget::HandleMissionStartClicked()
{
	// 싱글 전용 경로: 로컬이 곧 서버다. 멀티 전환 시 PlayerController Server RPC로 교체한다.
	ALSLobbyGameMode* LobbyGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALSLobbyGameMode>() : nullptr;
	if (!LobbyGameMode)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Mission start clicked but LobbyGameMode is missing."));
		return;
	}

	UE_LOG(LogLS, Log, TEXT("[Lobby] Mission start - requesting raid start."));
	LobbyGameMode->StartRaid();
}

void ULSLobbyMenuWidget::HandleSettingsClicked()
{
	ShowSettingsWidget();
}

void ULSLobbyMenuWidget::HandleSettingsBackToMenu()
{
	ActiveSettingsWidget = nullptr;

	UWorld* World = GetWorld();
	if (!World)
	{
		SetKeyboardFocus();
		return;
	}

	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (IsInViewport())
		{
			SetKeyboardFocus();
		}
	}));
}

ULSSettingsWidget* ULSLobbyMenuWidget::ShowSettingsWidget()
{
	// 이미 세팅 화면이 떠 있으면 중복 생성하지 않는다.
	if (ActiveSettingsWidget && ActiveSettingsWidget->IsInViewport())
	{
		return nullptr;
	}

	if (!SettingsWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] SettingsWidgetClass is not set on %s. Check WBP_Lobby."), *GetNameSafe(this));
		return nullptr;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	ULSSettingsWidget* SettingsWidget = OwningPlayer
		? CreateWidget<ULSSettingsWidget>(OwningPlayer, SettingsWidgetClass)
		: CreateWidget<ULSSettingsWidget>(this, SettingsWidgetClass);
	if (!SettingsWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Failed to create settings widget on %s."), *GetNameSafe(this));
		return nullptr;
	}

	SettingsWidget->OnBackToMenu.AddDynamic(this, &ULSLobbyMenuWidget::HandleSettingsBackToMenu);
	SettingsWidget->AddToViewport(LSUILayer::Settings);
	ActiveSettingsWidget = SettingsWidget;
	return SettingsWidget;
}

void ULSLobbyMenuWidget::ShowNotImplementedNotice()
{
	// 이미 안내창이 떠 있으면 중복 생성하지 않는다.
	if (ActiveConfirmDialog && ActiveConfirmDialog->IsInViewport())
	{
		return;
	}

	if (!ConfirmDialogClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] ConfirmDialogClass is not set on %s. Check WBP_LobbyMenu."), *GetNameSafe(this));
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	ULSConfirmDialogWidget* Dialog = OwningPlayer
		? CreateWidget<ULSConfirmDialogWidget>(OwningPlayer, ConfirmDialogClass)
		: CreateWidget<ULSConfirmDialogWidget>(this, ConfirmDialogClass);
	if (!Dialog)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Failed to create confirm dialog on %s."), *GetNameSafe(this));
		return;
	}

	// 확인/취소 어느 쪽을 눌러도(또는 ESC) 그냥 닫히고 로비로 돌아온다.
	Dialog->SetMessage(LOCTEXT("NotImplemented", "아직 <Emph>구현</>되지 않았습니다."));
	Dialog->OnConfirmed.AddDynamic(this, &ULSLobbyMenuWidget::HandleNotImplementedDialogClosed);
	Dialog->OnCancelled.AddDynamic(this, &ULSLobbyMenuWidget::HandleNotImplementedDialogClosed);
	Dialog->AddToViewport(LSUILayer::ModalPanel);
	ActiveConfirmDialog = Dialog;
}

bool ULSLobbyMenuWidget::CloseNotImplementedNotice()
{
	bool bClosedAny = false;

	if (ActiveConfirmDialog && ActiveConfirmDialog->IsInViewport())
	{
		// Cancel이 OnCancelled를 브로드캐스트해 HandleNotImplementedDialogClosed에서 참조 정리까지 이어진다.
		ActiveConfirmDialog->Cancel();
		bClosedAny = true;
	}

	if (WBP_LoadoutPreparation && WBP_LoadoutPreparation->HasActiveConfirmDialog())
	{
		WBP_LoadoutPreparation->CloseActiveConfirmDialog();
		bClosedAny = true;
	}

	return bClosedAny;
}

void ULSLobbyMenuWidget::HandleNotImplementedDialogClosed()
{
	ActiveConfirmDialog = nullptr;
	// 안내창이 닫혔으니 TAB/ESC가 다시 로비로 오도록 포커스를 회수한다.
	SetKeyboardFocus();
}

void ULSLobbyMenuWidget::ShowTab(const ELSLobbyTab Tab) const
{
	if (!TabSwitcher)
	{
		return;
	}

	TabSwitcher->SetActiveWidgetIndex(static_cast<int32>(Tab));
	UpdateBackground(Tab);
}

void ULSLobbyMenuWidget::UpdateBackground(const ELSLobbyTab Tab) const
{
	if (!BackgroundImage)
	{
		return;
	}

	// 개인정비 페이지이고 전용 배경 리소스가 지정돼 있으면 교체하고, 그 외에는 기본 배경으로 복원한다.
	const bool bUseLoadoutBackground = Tab == ELSLobbyTab::LoadoutPreparation
		&& LoadoutPreparationBackgroundBrush.GetResourceObject() != nullptr;
	BackgroundImage->SetBrush(bUseLoadoutBackground ? LoadoutPreparationBackgroundBrush : DefaultBackgroundBrush);
}

#undef LOCTEXT_NAMESPACE
