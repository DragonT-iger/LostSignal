// 로비의 세션·멀티플레이 조작(친구 방 참가, 접속 결과 안내). 패널 전환·입력과 분리해 둔다.
#include "UI/Lobby/LSLobbyMenuWidget.h"

#include "Components/EditableTextBox.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "Session/LSSessionSubsystem.h"
#include "UI/Common/LSConfirmDialogWidget.h"
#include "UI/LSUILayer.h"

#define LOCTEXT_NAMESPACE "LSLobbyMenuSession"

void ULSLobbyMenuWidget::HandleJoinClicked()
{
	if (!JoinAddressTextBox)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] JoinAddressTextBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	const FString Address = JoinAddressTextBox->GetText().ToString().TrimStartAndEnd();
	if (Address.IsEmpty())
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Join clicked but the address is empty."));
		ShowSessionNotice(LOCTEXT("JoinAddressEmpty", "접속할 <Emph>IP 주소</>를 입력해 주세요."));
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot join because the owning PlayerController is missing on %s."), *GetNameSafe(this));
		return;
	}

	// 이미 접속 중이면 무시한다. 연타하면 ClientTravel이 매번 새로 나가 접속이 계속 끊긴다.
	if (bJoinInProgress)
	{
		UE_LOG(LogLS, Log, TEXT("[Lobby] Join is already in progress. Ignoring the click."));
		return;
	}

	// 내 로비(리슨 서버)를 버리고 호스트의 현재 맵으로 넘어간다.
	// 실패하면 SessionSubsystem이 로비로 되돌리고 사유를 남긴다.
	bJoinInProgress = true;
	UE_LOG(LogLS, Log, TEXT("[Lobby] Joining server at %s"), *Address);
	OwningPlayer->ClientTravel(Address, TRAVEL_Absolute);
}

void ULSLobbyMenuWidget::ShowSessionNoticeOnOpen()
{
	// 접속 실패로 되돌아온 경우가 우선이다.
	const UGameInstance* GameInstance = GetGameInstance();
	ULSSessionSubsystem* SessionSub = GameInstance ? GameInstance->GetSubsystem<ULSSessionSubsystem>() : nullptr;
	FText FailureMessage;
	if (SessionSub && SessionSub->ConsumePendingNetworkFailureMessage(FailureMessage))
	{
		ShowSessionNotice(FailureMessage);
		return;
	}

	// 클라이언트 월드라는 건 남의 방에 들어와 있다는 뜻이다. 호스트/싱글에는 띄우지 않는다.
	const UWorld* World = GetWorld();
	if (World && World->GetNetMode() == NM_Client)
	{
		ShowSessionNotice(LOCTEXT("JoinSucceeded", "호스트에 <Emph>접속했습니다</>. 호스트가 출발할 때까지 기다려 주세요."));
	}
}

void ULSLobbyMenuWidget::ShowSessionNotice(const FText& Message)
{
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
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Failed to create session notice dialog on %s."), *GetNameSafe(this));
		return;
	}

	// 안내용이라 확인/취소 어느 쪽을 눌러도(또는 ESC) 그냥 닫힌다.
	Dialog->SetMessage(Message);
	Dialog->OnConfirmed.AddDynamic(this, &ULSLobbyMenuWidget::HandleSessionNoticeClosed);
	Dialog->OnCancelled.AddDynamic(this, &ULSLobbyMenuWidget::HandleSessionNoticeClosed);
	Dialog->AddToViewport(LSUILayer::ModalPanel);
	ActiveConfirmDialog = Dialog;
}

void ULSLobbyMenuWidget::HandleSessionNoticeClosed()
{
	ActiveConfirmDialog = nullptr;
	// 다이얼로그가 닫혔으니 TAB/ESC가 다시 로비로 오도록 포커스를 회수한다.
	SetKeyboardFocus();
}

#undef LOCTEXT_NAMESPACE
