// 로비 루트 위젯의 키 입력·포커스 회수·오버레이(설정/미구현 안내창).
// 라이프사이클은 LSLobbyMenuWidget.cpp, 패널 전환은 LSLobbyMenuWidget_Panels.cpp에 있다.
#include "UI/Lobby/LSLobbyMenuWidget.h"

#include "Core/LSPlayerControllerBase.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "LostSignal.h"
#include "TimerManager.h"
#include "UI/ChipSystem/LSChipStationWidget.h"
#include "UI/Common/LSConfirmDialogWidget.h"
#include "UI/Inventory/LSInventoryWidget.h"
#include "UI/LSUILayer.h"
#include "UI/Lobby/Store/LSStoreWidget.h"
#include "UI/Settings/LSSettingsWidget.h"

// 세 cpp 모두 같은 네임스페이스를 쓴다. 다르게 두면 LOCTEXT 키가 갈려 번역이 끊긴다.
#define LOCTEXT_NAMESPACE "LSLobbyMenu"

void ULSLobbyMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// TAB/ESC는 이 위젯의 키보드 포커스에 의존하는데, PlayerControllerBase::BeginPlay의
	// SetFocusToGameViewport 등 포커스가 뷰포트로 새는 경로가 여럿이라 매 틱 회수한다.
	// 계약: 로비 메뉴 트리 밖에 뜨는 포커스 위젯은 여기 예외 목록에 반드시 합류시켜야 한다.
	// 합류하지 않으면 그 위젯이 매 틱 포커스를 뺏겨 확인 버튼 첫 클릭이 씹힌다.
	// (포커스를 이미 쥐고 있으면 회수가 불필요하니 외부 모달 판정보다 포커스 보유를 먼저 본다.)
	if (!HasKeyboardFocus() && !HasFocusedDescendants())
	{
		const bool bExternalFocusWidgetOpen =
			(ActiveSettingsWidget && ActiveSettingsWidget->IsInViewport()) ||
			(ActiveConfirmDialog && ActiveConfirmDialog->IsInViewport()) ||
			HasActivePanelModal();
		if (!bExternalFocusWidgetOpen)
		{
			SetKeyboardFocus();
		}
	}
}

bool ULSLobbyMenuWidget::HasActivePanelModal() const
{
	// 패널이 스위처 안에 중첩돼 자체 모달을 띄우므로 루트가 대신 보고한다.
	// 활성 패널만 묻지 않고 전부 확인한다 — 모달을 띄운 직후 패널이 바뀌는 경로를 놓치지 않기 위해서다.
	// 새 패널을 스위처에 추가하면 그 패널이 자체 모달을 띄우는지 확인해 여기에 합류시킨다.
	if (ChipStationPanelInstance && ChipStationPanelInstance->HasActiveConfirmDialog())
	{
		return true;
	}
	// 보급소는 내부에서 자판기 수량 다이얼로그까지 위임 보고한다.
	if (StorePanelInstance && StorePanelInstance->HasActiveConfirmDialog())
	{
		return true;
	}
	// 인벤토리는 "가득 찼습니다" 알림을 자체적으로 띄운다. 구 개인정비 사슬에서 빠져 있어
	// 로비 가방에서 빠른이동이 실패했을 때 알림 첫 클릭이 씹히던 원인이다.
	if (LobbyInventoryInstance && LobbyInventoryInstance->HasActiveNotificationDialog())
	{
		return true;
	}

	return false;
}

FReply ULSLobbyMenuWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// 포커스가 버튼으로 넘어가도 터널링 단계라 루트가 먼저 받는다. TAB은 여기서 소비해 포커스 이동을 막는다.
	if (InKeyEvent.GetKey() == EKeys::Tab)
	{
		ToggleBagPanel();
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
		// 보급소의 자판기/제작대 같은 가장 안쪽 화면부터 한 단계씩 되돌린다.
		// 한 입력으로 두 단계를 건너뛰지 않는다(Docs/Systems/CraftingSystem.md).
		if (ActivePanel == ELSLobbyPanel::Supply && StorePanelInstance && StorePanelInstance->TryHandleBack())
		{
			return FReply::Handled();
		}

		// 패널이 열려 있으면 닫아 로비로 돌아온다.
		if (ActivePanel != ELSLobbyPanel::None)
		{
			ShowPanel(ELSLobbyPanel::None);
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

void ULSLobbyMenuWidget::ToggleBagPanel()
{
	// 보급소 안쪽 화면(자판기/제작대)에서는 TAB도 한 단계만 되돌린다.
	if (ActivePanel == ELSLobbyPanel::Supply && StorePanelInstance && StorePanelInstance->TryHandleBack())
	{
		return;
	}

	ShowPanel(ActivePanel == ELSLobbyPanel::Bag ? ELSLobbyPanel::None : ELSLobbyPanel::Bag);
}

void ULSLobbyMenuWidget::HandleSettingsBackToMenu()
{
	ActiveSettingsWidget = nullptr;
	UpdateSelectedTab(ActivePanel);

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
		UE_LOG(LogLS, Warning, TEXT("[Lobby] SettingsWidgetClass is not set on %s. Check WBP_LobbyMenu."), *GetNameSafe(this));
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
	UpdateSelectedTab(ActivePanel, true);
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
	if (ActiveConfirmDialog && ActiveConfirmDialog->IsInViewport())
	{
		// Cancel이 OnCancelled를 브로드캐스트해 HandleNotImplementedDialogClosed에서 참조 정리까지 이어진다.
		ActiveConfirmDialog->Cancel();
		return true;
	}

	return false;
}

void ULSLobbyMenuWidget::HandleNotImplementedDialogClosed()
{
	ActiveConfirmDialog = nullptr;
	UpdateMissionStartVisibility(ActivePanel == ELSLobbyPanel::None);
	// 안내창이 닫혔으니 TAB/ESC가 다시 로비로 오도록 포커스를 회수한다.
	SetKeyboardFocus();
}

#undef LOCTEXT_NAMESPACE
