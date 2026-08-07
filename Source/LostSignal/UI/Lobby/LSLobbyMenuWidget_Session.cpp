// 로비의 세션·멀티플레이 조작(친구 방 참가 등). 패널 전환·입력과 분리해 둔다.
#include "UI/Lobby/LSLobbyMenuWidget.h"

#include "Components/EditableTextBox.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"

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
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot join because the owning PlayerController is missing on %s."), *GetNameSafe(this));
		return;
	}

	// 내 로비(리슨 서버)를 버리고 호스트의 현재 맵으로 넘어간다.
	// 접속에 실패하면 엔진이 네트워크 에러 처리로 되돌린다.
	UE_LOG(LogLS, Log, TEXT("[Lobby] Joining server at %s"), *Address);
	OwningPlayer->ClientTravel(Address, TRAVEL_Absolute);
}
