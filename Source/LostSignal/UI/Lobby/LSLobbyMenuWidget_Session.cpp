// 로비의 세션·멀티플레이 조작(초대 코드, 친구 방 참가, 접속 결과 안내). 패널 전환·입력과 분리해 둔다.
#include "UI/Lobby/LSLobbyMenuWidget.h"

#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "IPAddress.h"
#include "LostSignal.h"
#include "Session/LSInviteCode.h"
#include "Session/LSSessionSubsystem.h"
#include "SocketSubsystem.h"
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

	const FString RawInput = JoinAddressTextBox->GetText().ToString().TrimStartAndEnd();
	if (RawInput.IsEmpty())
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Join clicked but the invite code is empty."));
		ShowSessionNotice(LOCTEXT("JoinInputEmpty", "<Emph>초대 코드</>를 입력해 주세요."));
		return;
	}

	const FString JoinUrl = ResolveJoinUrl(RawInput);
	if (JoinUrl.IsEmpty())
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Could not resolve a join address from '%s'."), *RawInput);
		ShowSessionNotice(LOCTEXT("JoinInputInvalid", "<Emph>초대 코드</>를 다시 확인해 주세요."));
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
	// 실패하면 SessionSubsystem이 로비로 되돌리고 사유를 다이얼로그로 알린다.
	bJoinInProgress = true;
	UE_LOG(LogLS, Log, TEXT("[Lobby] Joining server at %s (input: %s)"), *JoinUrl, *RawInput);
	OwningPlayer->ClientTravel(JoinUrl, TRAVEL_Absolute);
}

FString ULSLobbyMenuWidget::ResolveJoinUrl(const FString& RawInput)
{
	// 초대 코드만 받는다. 코드에 포트가 들어 있어 리슨 포트가 밀려도 정확히 찾아간다.
	// 생 IP 입력을 허용하면 포트를 빠뜨린 채 기본 포트로 붙어 조용히 타임아웃 나는 경로가 다시 생긴다.
	FString DecodedAddress;
	int32 DecodedPort = 0;
	if (!LSInviteCode::Decode(RawInput, DecodedAddress, DecodedPort))
	{
		return FString();
	}

	return FString::Printf(TEXT("%s:%d"), *DecodedAddress, DecodedPort);
}

void ULSLobbyMenuWidget::RefreshInviteCode() const
{
	if (!InviteCodeText)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] InviteCodeText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	const UWorld* World = GetWorld();
	const UNetDriver* NetDriver = World ? World->GetNetDriver() : nullptr;
	if (!World || World->GetNetMode() == NM_Client || !NetDriver)
	{
		// 남의 방에 들어와 있으면 초대할 방이 없다.
		InviteCodeText->SetText(LOCTEXT("InviteCodeUnavailable", "참가 중"));
		return;
	}

	// 포트는 실제로 바인딩된 값을 읽는다. 7777이 이미 잡혀 있으면 UE가 7778로 올리기 때문에
	// 설정값을 그대로 믿으면 안 된다.
	const TSharedPtr<const FInternetAddr> LocalAddr = const_cast<UNetDriver*>(NetDriver)->GetLocalAddr();
	const int32 Port = LocalAddr.IsValid() ? LocalAddr->GetPort() : 0;

	// 주소는 리슨 소켓이 0.0.0.0에 묶여 있어 소켓 서브시스템에서 따로 얻는다.
	FString HostIPv4;
	if (ISocketSubsystem* Sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
	{
		bool bCanBindAll = false;
		const TSharedRef<FInternetAddr> HostAddr = Sockets->GetLocalHostAddr(*GLog, bCanBindAll);
		HostIPv4 = HostAddr->ToString(false);
	}

	const FString Code = LSInviteCode::Encode(HostIPv4, Port);
	if (Code.IsEmpty())
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Could not build an invite code. Address=%s Port=%d"), *HostIPv4, Port);
		InviteCodeText->SetText(LOCTEXT("InviteCodeFailed", "코드를 만들 수 없습니다"));
		return;
	}

	UE_LOG(LogLS, Log, TEXT("[Lobby] Invite code %s (%s:%d)"), *Code, *HostIPv4, Port);
	InviteCodeText->SetText(FText::FromString(LSInviteCode::Format(Code)));
}

void ULSLobbyMenuWidget::ShowSessionNoticeOnOpen()
{
	RefreshInviteCode();

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
