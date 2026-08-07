# Raid Level Flow

## 목적

이 문서는 로비에서 레이드에 입장하고, 파밍 레벨에서 레이드 결과를 확정한 뒤, 결과 레벨로 이동하는 흐름을 정리한다.

아이템 슬롯 구조와 저장/네트워크 경계는 [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md)를 기준으로 보고, 인벤토리 UI 조작은 [InventoryLogic.md](InventoryLogic.md)를 기준으로 본다. 이 문서는 레벨 전환, 입장 데이터 제출, 결과 저장 ACK, 실패 처리에 집중한다.

## 관련 클래스

```text
ULSSessionSettings
- LobbyLevel
- FarmingLevel
- ResultLevel

ALSLobbyGameMode
- 레이드 시작 요청
- 각 PlayerController에 입장 payload 요청
- 모든 payload 수신 후 FarmingLevel ServerTravel

ALSFarmingGameMode
- 사망/탈출/중도 종료 결과 확정
- 각 PlayerController의 RaidInventoryComponent 기준으로 결과 payload 생성
- 클라이언트 로컬 SaveGame 저장 ACK 대기
- 모든 ACK 수신 후 ServerTravel (파티 유지)
- 개인 이탈은 QuitRaidForPlayer로 그 PC만 ClientTravel

ALSResultGameMode
- 결과 레벨에서 LobbyLevel 복귀

ALSPlayerControllerBase
- 로컬 SaveSubsystem에서 입장 payload 제출
- 서버의 레이드 인벤토리 상태를 클라이언트에 미러링
- 레이드 결과 payload를 로컬 SaveGame에 반영하고 ACK 전송

ULSRaidInventoryComponent
- 레이드 중 플레이어별 Inventory/Safe 상태의 서버 원본

ALSPlayerState
- 레이드 입장 payload(Inventory/SafeStash/Equipment)의 원본
- CopyProperties로 seamless travel을 건너간다 — 컨트롤러는 travel마다 새로 스폰되므로 여기 둬야 한다
- 서버 전용(리플리케이트 안 함). 클라는 RaidInventoryComponent 미러로 본다

ULSSessionSubsystem
- bAllowQuitRecovery: 중도 포기 시 출발 장비 복구 여부 (레이드 경로에서 남은 유일한 용도)
- 레이드 중 아이템의 원본은 PlayerController별 RaidInventoryComponent

ULSSettingsWidget
- 세팅 화면(WBP_Settings). 타이틀/로비/레이드(ESC) 어디서든 동일한 위젯을 재사용한다 — 레이드 전용 별도 메뉴는 없다
- 로비 기본 Play 화면에서는 ULSLobbyMenuWidget이 ESC 입력을 받아 설정 버튼과 같은 ShowSettingsWidget 경로로 이 위젯을 띄운다
- 레이드 중에는 ALSPlayerControllerBase가 ESC로 이 위젯을 직접 토글해서 띄운다
- "메인메뉴로 돌아가기"(MainMenuButton, ULSTitleMenuButtonWidget) 클릭 시: **한 단계씩 나간다.**
  - 레이드 중이면: 확인 다이얼로그 → ALSPlayerControllerBase::ServerRequestQuitRaid(true) → 서버가 **로비로** 내보낸다
  - 레이드 중이 아니면(로비): TitleLevel로 OpenLevel
  - 타이틀에서 열 때는 이미 메인메뉴라 불필요하므로 이 버튼을 숨긴다(SetMainMenuButtonVisible(false), 타이틀 메뉴가 세팅 생성 시 호출)
- BackButton(일반 UButton)은 세팅 패널만 닫는다(OnBackToMenu 브로드캐스트). ESC/TAB 키로도 동일하게 닫힌다
  (ESC: ULSSettingsWidget::NativeOnKeyDown → CloseSettings, TAB: NativeOnPreviewKeyDown → CloseSettings). 레이드 중에는 이 위젯이 스스로 RemoveFromParent 하므로
  PlayerController가 OnBackToMenu로 캐시를 비워 다음 ESC에 재생성한다. 로비에서는 ULSLobbyMenuWidget이 같은 브로드캐스트로 캐시를 비우고 다음 틱에 로비 루트 포커스를 복구한다
- 레이드 ESC 토글(ALSPlayerControllerBase::ToggleRaidSettingsWidget): 안 떠 있으면 생성, 떠 있으면
  CloseSettings로 닫는다. 위젯에 키보드 포커스가 있으면 위젯의 NativeOnKeyDown이 ESC를 먼저 소비하고,
  포커스가 게임 뷰포트에 있으면 이 토글이 폴백으로 닫는다(중복 처리 없음)
```

## 레벨 설정

레벨 경로는 `ULSSessionSettings`의 프로젝트 설정 값을 사용한다.

```text
Project Settings > LS Session Settings
- TitleLevel
- LobbyLevel
- FarmingLevel
- ResultLevel
```

현재 사용처:

```text
Title -> Lobby (Listen Server / Dedicated Server)
- ULSTitleMenuWidget::OpenLobbyLevel
- Settings->LobbyLevel
- World->ServerTravel(LobbyLevelPath)

Title -> Lobby (Standalone = 방 열기)
- ULSTitleMenuWidget::OpenLobbyLevel
- Settings->LobbyLevel
- UGameplayStatics::OpenLevel(..., Options="listen")   로비를 리슨 서버로 연다

Lobby -> 남의 방 참가
- ULSLobbyMenuWidget::HandleJoinClicked (LSLobbyMenuWidget_Session.cpp)
- JoinAddressTextBox의 주소
- PlayerController->ClientTravel(주소, TRAVEL_Absolute)

Lobby -> Farming
- ALSLobbyGameMode::TryStartRaidWithSubmittedData
- Settings->FarmingLevel
- World->ServerTravel(FarmingLevelPath)

Farming -> Lobby (Extracted / Dead)
- ALSFarmingGameMode::TravelToResultLevel
- Settings->LobbyLevel
- ALSFarmingGameMode::ServerTravelToLevel -> World->ServerTravel

Farming -> Lobby (개인 이탈)
- ALSFarmingGameMode::QuitRaidForPlayer -> SendPlayerToOwnLobby
- Settings->LobbyLevel + "?listen"
- PlayerController->ClientTravel (그 클라이언트만 호스트 세션을 떠나 자기 로비를 연다)

Farming -> Lobby (호스트 이탈 = 전원 Quit)
- ALSFarmingGameMode::TravelToResultLevel (PendingRaidResult == Quit)
- Settings->LobbyLevel
- ALSFarmingGameMode::ServerTravelToLevel -> World->ServerTravel (파티 유지)

Result -> Lobby
- ALSResultGameMode::ReturnToLobby
- Settings->LobbyLevel
- World->ServerTravel
```

**레벨 이동에 `UGameplayStatics::OpenLevel` 계열을 쓰지 않는다.** `OpenLevel`은 `UEngine::SetClientTravel`을 부르는데, 이어지는 `Browse`가 넷드라이버를 파괴해 클라이언트가 전원 끊기고, 거기에 더해 `LastURL`에서 `Listen` 옵션까지 제거한다(`UnrealEngine.cpp`의 "Prevent crashing the game by attempting to connect to own listen server"). 즉 호스트가 리슨 서버를 그만두게 되어 **첫 레이드 이후 멀티가 성립하지 않는다.** 서버 전체를 옮길 때는 `World->ServerTravel`, 한 명만 내보낼 때는 `PlayerController->ClientTravel`을 쓴다.

`ALSGameModeBase`는 `bUseSeamlessTravel=true`를 기본 적용한다. 따라서 리슨 서버의 Title→Lobby, Lobby→Farming, Farming→Lobby 전환은 기존 NetDriver와 참가자 연결을 유지한다. 타이틀에서 로컬 `OpenLevel`로 리슨 서버를 닫는 경로는 Standalone에서만 사용한다. PIE는 엔진 기본값으로 seamless travel을 비활성화하므로 `DefaultEngine.ini`의 `net.AllowPIESeamlessTravel=1`을 함께 사용한다.

seamless travel은 **PlayerController를 호스트 것까지 전부 새로 스폰한다**(`AGameModeBase::HandleSeamlessTravelPlayer`). PlayerState도 같은 객체가 넘어오는 것이 아니라 `APlayerState::CopyProperties`로 값만 복사된다. 따라서 레벨 전환을 건너뛰어야 하는 상태는 PlayerController 멤버에 두면 안 되고, PlayerState에 두더라도 `CopyProperties` 오버라이드가 필요하다.

리슨 서버의 타이틀에서 Continue/New을 누르면 `ALSTitleGameMode::RequestOpenLobbyLevel`이 로그인 중인 ClientConnection을 먼저 확인한다. 참가자의 `PostLogin`이 끝나면 Lobby로 ServerTravel하며, 10초 안에 로그인이 끝나지 않으면 참가자를 버리고 이동하지 않고 로비 전환을 취소한다.

`FarmingLevel` 또는 `ResultLevel`이 비어 있으면 레벨 이동을 하지 않고 `UE_LOG(LogLS, Warning, ...)`를 남긴다.

## 레이드 입장 흐름

진입점은 `ALSLobbyGameMode::StartRaid`다.

```text
ALSLobbyGameMode::StartRaid
-> bRaidStartRequested 설정
-> 10초 RaidEntryDataTimeout 타이머 시작
-> NetDriver에 로그인 중인 ClientConnection이 있으면 완료될 때까지 폴링
-> 로그인 완료 후 bWaitingForRaidEntryData 설정
-> RequestRaidEntryDataFromPlayers
-> TryStartRaidWithSubmittedData
```

리슨 서버가 클라이언트 접속을 수락했지만 아직 `PlayerController`를 생성하지 못한 상태에서 호스트가 출발을 누르면, 해당 연결의 `ClientLoginState`가 `ReceivedJoin`이 될 때까지 입장 payload 수집을 시작하지 않는다. `PostLogin`으로 뒤늦게 참가한 플레이어가 payload 수집 중 합류한 경우에도 그 컨트롤러에 입장 데이터를 요청한다. `Logout`이 발생하면 남아 있는 컨트롤러 기준으로 준비 상태를 다시 평가한다.

플레이어별 입장 데이터 제출:

```text
ALSLobbyGameMode::RequestRaidEntryDataFromPlayers
-> 각 ALSPlayerControllerBase::RequestRaidEntryDataForRaidStart

ALSPlayerControllerBase::RequestRaidEntryDataForRaidStart
-> ClearSubmittedRaidEntryData
-> 로컬 컨트롤러면 SubmitLocalRaidEntryData
-> 원격 컨트롤러면 ClientRequestRaidEntryData RPC

ALSPlayerControllerBase::SubmitLocalRaidEntryData
-> ULSSaveSubsystem::GetInventory
-> ULSSaveSubsystem::GetSafeStash
-> ULSSaveSubsystem::GetEquipmentSlots
-> 서버면 StoreSubmittedRaidEntryData 후 LobbyGameMode 알림
-> 클라이언트면 ServerSubmitRaidEntryData RPC

ALSPlayerControllerBase::ServerSubmitRaidEntryData
-> StoreSubmittedRaidEntryData
-> ALSLobbyGameMode::NotifyRaidEntryDataSubmitted
```

서버는 각 `PlayerController`가 제출한 payload를 그 플레이어의 `ALSPlayerState`에 저장한다(`StoreRaidEntryData`). 저장 전에 `LSInventorySlotUtils::NormalizeSlotArray`로 인벤토리와 금고를 정규화한다. 무기/방어구 장비도 함께 제출하지만, 장비는 인덱스=슬롯타입 불변식이라 **Normalize하지 않고 SetNum(5) 패딩만** 한다.

`ALSPlayerControllerBase`의 `GetSubmittedRaid*` / `HasSubmittedRaidEntryData` / `ClearSubmittedRaidEntryData`는 이제 PlayerState로 포워딩하는 얇은 래퍼다. **컨트롤러에 payload를 다시 들이면 안 된다** — seamless travel에서 전원 유실된다.

모든 플레이어가 제출을 완료하면:

```text
ALSLobbyGameMode::TryStartRaidWithSubmittedData
-> 모든 PlayerController의 HasSubmittedRaidEntryData 확인
-> 각 PlayerController마다:
   - RaidInventoryComponent->StartRaidInventory(PlayerLoadout, PlayerSafeItems, PlayerEquipment)
   - ClientStartRaidSession(PlayerLoadout, PlayerSafeItems, PlayerEquipment)
-> bRaidTravelStarted = true
-> FarmingLevel로 ServerTravel
```

여기서 주입한 `StartRaidInventory`는 ServerTravel로 컨트롤러/컴포넌트가 새로 생성되면 유실된다. 실제 복원 경로는 `ALSPlayerState`에 실려 건너간 payload다(아래 "레이드 중 상태 원본" 참고).

`bRaidTravelStarted`는 seamless travel 중에 원격 PC가 하나씩 Logout될 때 `Logout` 핸들러가 입장 데이터를 다시 수집하고 `ServerTravel`을 또 거는 것을 막는다. 이 가드가 없으면 인원수만큼 재수집·재전환이 반복된다.

`ClientStartRaidSession`은 클라이언트의 `RaidInventoryComponent`를 미러링한 뒤 로컬 `SaveSubsystem->BeginRaidSave(Loadout)`로 복구용 레이드 저장을 시작한다.

## 레이드 중 상태 원본

레이드 중 아이템 상태의 서버 원본은 플레이어별 `ULSRaidInventoryComponent`다.

```text
ULSRaidInventoryComponent
- SessionInventory
- SessionSafeInventory
- bRaidActive
- LoadoutSnapshot
- ConsumedItems
```

레이드 중 클라이언트 값을 다시 신뢰하지 않고 서버 미러만 표시하는 신뢰 경계 원칙은 [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md)가 단일 출처다. 여기서는 UI가 서버에서 내려온 `RaidInventoryComponent` 미러 상태를 보여준다는 점만 짚는다.

대표 동기화 흐름:

```text
서버에서 레이드 인벤토리 변경
-> ALSPlayerControllerBase::SyncRaidInventoryToClient
-> ClientSyncRaidSessionAndLoot
-> 클라이언트 RaidInventoryComponent->MirrorRaidInventoryState
-> 인벤토리/루팅 UI 갱신
```

ServerTravel 이후 각 `PlayerController`가 자기 `PlayerState`에서 인벤토리를 복원한다.

```text
ALSPlayerControllerBase::InitializeRaidInventoryFromPlayerState
-> RaidInventoryComponent가 이미 active면 (서버면 ClientSyncRaidSessionAndLoot 후) 종료
-> 클라이언트면 종료 (payload는 서버에만 있다)
-> PlayerState에 입장 데이터가 없으면 종료
-> RaidInventoryComponent->StartRaidInventory(PlayerState의 Loadout/Safe/Equipment)
-> ClientSyncRaidSessionAndLoot
```

**호출 시점이 `BeginPlay`가 아니라 `PostSeamlessTravel`이다.** `AGameModeBase::HandleSeamlessTravelPlayer`는 컨트롤러를 먼저 스폰하고(=BeginPlay가 여기서 돈다) 그 뒤에 `SeamlessTravelFrom`으로 PlayerState 값을 복사한다. 그래서 `BeginPlay`에서 읽으면 아직 비어 있다. `BeginPlay`에서도 한 번 호출하지만 그건 travel이 아닌 진입(초기 로그인·PIE 비 seamless)용 폴백이고, 실제 MO 복원은 `PostSeamlessTravel`이 담당한다.

순서 기반 큐(`PendingRaidEntries`)와 전역 미러(`MirrorRaidSessionState`)는 제거됐다. 접속 순서에 의존해 3인에서 인벤토리를 섞던 경로이며, `PendingRaidEntryIndex`가 GameInstance에 누적돼 두 번째 레이드에서 반드시 어긋났다.

## 레이드 종료 트리거

`ALSFarmingGameMode`가 레이드 결과를 확정한다.

> **현재는 개별 탈출이 없다.** 아래처럼 누가 탈출·사망하든 `EndRaid`가 모든 PlayerController를 순회해 **레이드를 한 번에 끝낸다.** 3인 MO에서는 개별 탈출 + 관전자 방식으로 개조하기로 결정했으며, 그 계획과 근거(플레이어별 결과 확정 경로, 호스트 이탈 시 생존자 소지품 확정 규칙)는 [DedicatedServerBuildout.md](DedicatedServerBuildout.md)가 단일 출처다. 이 문서는 계획이 아니라 현재 코드를 기술한다.

```text
ALSFarmingGameMode::OnExtraction
-> EndRaid(ELSRaidResult::Extracted)

ALSFarmingGameMode::OnPlayerDied
-> DeathRaidEndDelaySeconds 타이머 (사망 연출 시간, EditDefaultsOnly)
-> EndRaid(ELSRaidResult::Dead)

ALSFarmingGameMode::OnQuit
-> EndRaid(ELSRaidResult::Quit)          전원 종료 (호스트 이탈 전용)

ALSFarmingGameMode::QuitRaidForPlayer(PC)
-> 호스트면 OnQuit()으로 위 경로 (전원 확정 후 파티째 로비로)
-> 아니면 그 PC만 BuildRaidResultForPlayer(Quit) -> RequestRaidResultSave
   -> ACK 수신 시 EndRaidInventory + ClearSubmittedRaidEntryData + SendPlayerToOwnLobby
   -> 남은 사람의 레이드는 계속된다
```

**나가는 방향은 한 단계씩이다 — 레이드에서는 로비로, 로비에서는 타이틀로.** 레이드에서 곧바로 타이틀로 나가는 경로는 없다.

개인 이탈자는 호스트 세션에 남을 수 없다(접속 상태에서는 서버와 같은 맵에 있어야 한다). 그래서 접속을 끊고 자기 로비를 `?listen`으로 열어 준다 — 돌아온 로비에서 바로 다시 친구를 받을 수 있다. 파티에 남아 관전하는 형태는 개별 탈출·관전자(C6·C7) 작업에 속하며 [DedicatedServerBuildout.md](DedicatedServerBuildout.md)가 소유한다.

개인 이탈 진입점은 `ULSSettingsWidget::HandleReturnToTitleConfirmed`다. 클라이언트에는 GameMode가 없으므로 위젯이 `ALSPlayerControllerBase::ServerRequestQuitRaid(bAllowRecovery)` RPC로 넘기고, 서버가 그 RPC 안에서 `ULSSessionSubsystem::bAllowQuitRecovery`를 세운 뒤 `QuitRaidForPlayer`를 호출한다. (클라가 자기 쪽 SessionSubsystem에 켜봐야 결과를 만드는 서버는 그 값을 보지 못한다)

**호스트 이탈은 아직 구멍이다.** 리슨 서버는 호스트가 곧 서버 프로세스라 혼자 빠질 수 없어 전원을 로비로 데려간다. 생존자 소지품 확정(C2 규칙)은 [DedicatedServerBuildout.md](DedicatedServerBuildout.md)가 단일 출처이며 아직 구현되지 않았다.

`OnPlayerDied`는 플레이어 캐릭터 사망에서 호출된다. 체력 0 도달 시
`ULSCharacterCombatComponent`가 `OnDeathStateChanged(true)` 훅을 호출하고,
`ALSPlayerCharacter::OnDeathStateChanged`가 서버 권한에서만 FarmingGameMode를 찾아 알린다
(파밍 외 레벨에서는 GameMode 캐스트 실패로 무시). 사망 딜레이 중 중복 호출은
타이머 활성 검사로 무시된다.

`EndRaid`는 중복 실행을 막기 위해 `bRaidEnded`를 먼저 확인하고, 실제 처리는 `BeginRaidResultSave`로 넘긴다.

## 결과 저장 흐름

`BeginRaidResultSave`는 레이드가 active인 `RaidInventoryComponent`를 가진 플레이어만 대상으로 결과 저장을 요청한다.

```text
ALSFarmingGameMode::BeginRaidResultSave
-> active RaidInventoryComponent를 가진 PlayerController 수집
-> PendingRaidResultSaveControllers에 추가
-> 10초 RaidResultSaveTimeout 타이머 시작
-> BuildRaidResultForPlayer
-> PlayerController->RequestRaidResultSave
```

플레이어별 결과 payload 생성 규칙:

```text
Extracted
- Inventory = RaidInventoryComponent.SessionInventory
- SafeStash = RaidInventoryComponent.SessionSafeInventory
- Equipment = RaidInventoryComponent.SessionEquipmentSlots
- SaveInventory = true
- SaveSafeStash = true
- SaveEquipment = true                (레이드 최종 장착 상태 저장)

Dead
- Inventory = empty
- SafeStash = RaidInventoryComponent.SessionSafeInventory
- Equipment = empty                   (빈 5칸 저장 = 장비 소멸, 바닥 드랍 없음)
- SaveInventory = true
- SaveSafeStash = true
- SaveEquipment = true

Quit
- bAllowQuitRecovery가 true면 Inventory = SubmittedRaidLoadout
- bAllowQuitRecovery가 false면 Inventory = empty
- SaveInventory = true
- SaveSafeStash = false
- SaveEquipment = false                (저장 생략 = 입장 시점 장착 상태로 자동 복구)
```

장비 결과 규칙은 인벤토리와 다르다. 레이드 중 클라 `SaveGame.EquipmentSlots`는 불변이므로 **Quit/강제종료는 "저장하지 않기"가 곧 입장 시점 복구**다(인벤의 `bAllowQuitRecovery`처럼 출발 payload를 되쓰지 않는다). 자세한 불변식은 [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md)가 단일 출처다.

결과 payload를 받은 `PlayerController`는 로컬 `ULSSaveSubsystem`에 반영한다.

```text
ALSPlayerControllerBase::RequestRaidResultSave
-> 로컬 컨트롤러면 ApplyRaidResultToLocalSave
-> 원격 컨트롤러면 ClientApplyRaidResult RPC

ALSPlayerControllerBase::ApplyRaidResultToLocalSave
-> bSaveInventory면 SaveSubsystem->ReplaceInventory
-> bSaveSafeStash면 SaveSubsystem->ReplaceSafeStash
-> bSaveEquipment면 SaveSubsystem->ReplaceEquipmentSlots
-> SaveSubsystem->ClearRaidSave
-> 서버 컨트롤러면 FarmingGameMode->NotifyRaidResultSaved
-> 클라이언트면 ServerConfirmRaidResultSaved RPC

ALSPlayerControllerBase::ServerConfirmRaidResultSaved
-> ALSFarmingGameMode::NotifyRaidResultSaved
```

모든 대상 플레이어가 ACK를 보내면:

```text
ALSFarmingGameMode::NotifyRaidResultSaved
-> PendingRaidResultSaveControllers에서 제거
-> 모두 제거되면 TravelToResultLevel

ALSFarmingGameMode::TravelToResultLevel
-> 각 RaidInventoryComponent->EndRaidInventory
-> 각 PlayerController->ClearSubmittedRaidEntryData
-> SessionSubsystem->ClearRaidSessionState
-> 종료 결과가 Extracted 또는 Dead면 LobbyLevel로 ServerTravel (일단 ResultLevel 건너뜀)
-> 종료 결과가 Quit이면 LobbyLevel로 ServerTravel (ResultLevel 건너뜀)
```

`NotifyRaidResultSaved`는 개인 이탈 ACK와 전원 종료 ACK를 둘 다 받는다. `PendingQuitControllers`에 있으면 그 사람만 자기 로비로 내보내고, 이어서 `PendingRaidResultSaveControllers`에서도 제거한다(두 대기가 겹쳤을 때 그룹 전환이 막히지 않도록 early return 하지 않는다).

> 현재 탈출(Extracted) 성공과 사망(Dead)은 임시로 ResultLevel을 거치지 않고 바로 `LobbyLevel`로 복귀한다. 결과 레벨(전리품 정산 등)이 준비되면 이 분기를 제거하고 다시 ResultLevel을 거치도록 되돌린다.
>
> 중도 포기(Quit)도 결과를 보여줄 필요가 없어 ResultLevel을 건너뛰지만, 목적지는 다른 결과와 같은 `LobbyLevel`이다. 레이드 중 ESC로 띄운 `ULSSettingsWidget`(WBP_Settings, 타이틀/로비와 동일한 위젯)의 "메인메뉴로 돌아가기"가 이 경로를 탄다 — 확인 다이얼로그를 거친 뒤 `ALSPlayerControllerBase::ServerRequestQuitRaid(true)`로 서버에 넘기고, 서버가 `bAllowQuitRecovery`를 세운 뒤 출발 장비를 복구한다.

## 타임아웃과 실패 처리

입장 데이터 제출 대기:

```text
타임아웃: 10초
처리: ALSLobbyGameMode::HandleRaidEntryDataTimeout
동작:
- 로그인 중인 ClientConnection이 남아 있으면 Warning 로그로 남김
- 제출하지 않은 PlayerController를 Warning 로그로 남김
- ClearRaidEntryDataWait
- 레이드 시작 취소
```

타이틀의 로비 전환도 로그인 중인 ClientConnection에 대해 10초 타임아웃을 사용하며, 초과 시 `ALSTitleGameMode::HandlePendingPlayerConnectionTimeout`이 전환을 취소한다.

결과 저장 ACK 대기:

```text
타임아웃: 10초
처리: ALSFarmingGameMode::HandleRaidResultSaveTimeout
동작:
- ACK를 보내지 않은 PlayerController를 Warning 로그로 남김
- ClearRaidResultSaveWait
- bRaidEnded = false
- ResultLevel 이동 안 함
```

주의할 점:

- timeout 값은 현재 cpp 내부 `constexpr`이다. 기획/QA에서 조정해야 하면 `ULSSessionSettings`로 빼는 것이 좋다.
- 로그인 대기와 입장 데이터 제출 대기는 같은 10초 제한을 공유한다.
- 결과 저장 ACK가 누락되면 ResultLevel 이동이 멈춘다. 멀티 테스트에서 가장 먼저 확인해야 할 지점이다.
- **개인 이탈(`PendingQuitControllers`)에는 아직 타임아웃이 없다.** ACK가 안 오면 그 플레이어만 레이드에 갇힌다. (접속이 끊기는 경우는 `Logout`이 잡는다)

`ALSFarmingGameMode::Logout`이 이탈자를 두 대기 목록(`PendingQuitControllers` / `PendingRaidResultSaveControllers`)에서 모두 제거하고, 그 결과 목록이 비면 곧바로 레벨 전환을 진행한다. 이 처리가 없으면 ACK를 못 보내는 이탈자 때문에 남은 인원이 전부 ACK해도 전환이 영구히 막힌다 — 3인 이상에서 "로비로 안 넘어감"의 주 원인이었다. `BeginRaidResultSave`에서 결과 생성에 실패한 컨트롤러도 같은 이유로 대기 목록에서 즉시 제거한다.
- `ServerTravel` 실패 시 현재 `ClearRaidEntryDataWait`를 호출하지만 일부 `RaidInventoryComponent`는 이미 시작된 뒤일 수 있다. 이 경우 복구 처리가 더 필요할 수 있다.

## 레이드 종료의 단일 경로

레이드 종료는 `ALSFarmingGameMode` 하나만 담당한다.

```text
ALSFarmingGameMode::EndRaid
-> BuildRaidResultForPlayer
-> PlayerController::RequestRaidResultSave
-> PlayerController::ServerConfirmRaidResultSaved
-> ALSFarmingGameMode::TravelToResultLevel
```

`ULSSessionSubsystem::EndRaid`(자체 결과 저장 + `OpenLevel`)와 `StartRaidClientMirror`, `MirrorRaidSessionState`, `PendingRaidEntries` 큐는 제거됐다. 새 레이드 종료 로직은 위 경로를 기준으로 확장한다.

`ULSSessionSubsystem`에 레이드 경로로 남은 것은 `bAllowQuitRecovery`와 `ClearRaidSessionState`뿐이다. 나머지 세션 인벤토리 API(`AddSessionItem`/`SwapSessionSlots` 등)는 싱글 시절 잔재이며 호출부가 없다.

## 테스트 체크리스트

싱글/Listen Server:

```text
- Lobby에서 StartRaid 호출 시 FarmingLevel로 이동하는지
- Inventory와 SafeStash가 RaidInventoryComponent에 복사되는지
- 레이드 중 루팅/드랍 후 UI가 서버 상태로 다시 동기화되는지
- Extracted 결과에서 Inventory/SafeStash/Equipment가 모두 저장되는지
- Dead 결과에서 Inventory/Equipment는 비고 SafeStash는 유지되는지
- Quit 결과에서 bAllowQuitRecovery 값에 따라 Inventory 저장 결과가 달라지는지 / 장비는 입장 시점으로 복구되는지
- ResultLevel 이동 후 RaidInventoryComponent와 SubmittedRaidEntryData가 정리되는지
- ResultLevel에서 LobbyLevel 복귀가 되는지
```

2인 이상 MO 준비:

```text
- 각 클라이언트가 자기 로컬 SaveGame에서 서로 다른 입장 payload를 제출하는지
- Player1의 제출 payload가 Player2의 RaidInventoryComponent에 들어가지 않는지
- 레이드 중 Player1 루팅/드랍이 Player2 인벤토리 상태를 변경하지 않는지
- 각 클라이언트가 자기 결과 payload만 로컬 SaveGame에 반영하는지
- 한 클라이언트가 입장 payload 또는 결과 저장 ACK를 보내지 않을 때 timeout 로그가 남는지
```

## 빠른 흐름도

```text
Lobby StartRaid
  -> Request raid entry data from each PlayerController
  -> Client reads local SaveSubsystem Inventory/SafeStash
  -> Server stores SubmittedRaidLoadout/SubmittedRaidSafeItems
  -> All submitted
  -> Each payload enqueued to SessionSubsystem PendingRaidEntries
  -> Each RaidInventoryComponent starts with its own payload
  -> ClientStartRaidSession mirrors local raid state + BeginRaidSave
  -> ServerTravel(FarmingLevel)
  -> Each PlayerController dequeues its payload to restore RaidInventoryComponent

Farming Raid
  -> Server owns each RaidInventoryComponent
  -> Loot/drop/sort changes happen on server
  -> ClientSyncRaidSessionAndLoot mirrors UI state

Raid End
  -> FarmingGameMode decides Extracted/Dead/Quit
     (Dead: 캐릭터 사망 -> OnPlayerDied -> 사망 연출 딜레이 후 확정)
  -> Build result from each RaidInventoryComponent
  -> ClientApplyRaidResult writes local SaveSubsystem
  -> ServerConfirmRaidResultSaved ACK
  -> All ACKed
  -> EndRaidInventory / ClearSubmittedRaidEntryData / ClearRaidSessionState
  -> ServerTravel(Extracted/Dead/전원 Quit: LobbyLevel)   파티 유지

Individual Quit
  -> ServerRequestQuitRaid -> QuitRaidForPlayer
  -> Build result(Quit) -> ClientApplyRaidResult -> ServerConfirmRaidResultSaved
  -> ClientTravel(LobbyLevel?listen)   그 사람만 자기 로비로 나간다

Result
  -> ResultGameMode::ReturnToLobby
  -> OpenLevel(LobbyLevel)
```
