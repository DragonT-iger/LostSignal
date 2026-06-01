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
- 모든 ACK 수신 후 ResultLevel 이동

ALSResultGameMode
- 결과 레벨에서 LobbyLevel 복귀

ALSPlayerControllerBase
- 로컬 SaveSubsystem에서 입장 payload 제출
- 서버의 레이드 인벤토리 상태를 클라이언트에 미러링
- 레이드 결과 payload를 로컬 SaveGame에 반영하고 ACK 전송

ULSRaidInventoryComponent
- 레이드 중 플레이어별 Inventory/Safe 상태의 서버 원본

ULSSessionSubsystem
- 1인/legacy 호환용 세션 미러 상태
- 현재 MO 흐름에서는 PlayerController별 RaidInventoryComponent가 레이드 중 원본
```

## 레벨 설정

레벨 경로는 `ULSSessionSettings`의 프로젝트 설정 값을 사용한다.

```text
Project Settings > LS Session Settings
- LobbyLevel
- FarmingLevel
- ResultLevel
```

현재 사용처:

```text
Lobby -> Farming
- ALSLobbyGameMode::TryStartRaidWithSubmittedData
- Settings->FarmingLevel
- World->ServerTravel(FarmingLevelPath)

Farming -> Result
- ALSFarmingGameMode::TravelToResultLevel
- Settings->ResultLevel
- UGameplayStatics::OpenLevelBySoftObjectPtr

Result -> Lobby
- ALSResultGameMode::ReturnToLobby
- Settings->LobbyLevel
- UGameplayStatics::OpenLevelBySoftObjectPtr
```

`FarmingLevel` 또는 `ResultLevel`이 비어 있으면 레벨 이동을 하지 않고 `UE_LOG(LogLS, Warning, ...)`를 남긴다.

## 레이드 입장 흐름

진입점은 `ALSLobbyGameMode::StartRaid`다.

```text
ALSLobbyGameMode::StartRaid
-> bRaidStartRequested / bWaitingForRaidEntryData 설정
-> 10초 RaidEntryDataTimeout 타이머 시작
-> RequestRaidEntryDataFromPlayers
-> TryStartRaidWithSubmittedData
```

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
-> 서버면 StoreSubmittedRaidEntryData 후 LobbyGameMode 알림
-> 클라이언트면 ServerSubmitRaidEntryData RPC

ALSPlayerControllerBase::ServerSubmitRaidEntryData
-> StoreSubmittedRaidEntryData
-> ALSLobbyGameMode::NotifyRaidEntryDataSubmitted
```

서버는 각 `PlayerController`가 제출한 `Inventory`와 `SafeStash`를 `SubmittedRaidLoadout`, `SubmittedRaidSafeItems`에 저장한다. 저장 전에 `LSInventorySlotUtils::NormalizeSlotArray`로 슬롯 배열을 정규화한다.

모든 플레이어가 제출을 완료하면:

```text
ALSLobbyGameMode::TryStartRaidWithSubmittedData
-> 모든 PlayerController의 HasSubmittedRaidEntryData 확인
-> 각 PlayerController의 RaidInventoryComponent->StartRaidInventory(PlayerLoadout, PlayerSafeItems)
-> 각 PlayerController에 ClientStartRaidSession(PlayerLoadout, PlayerSafeItems)
-> ULSSessionSubsystem::StartRaid / MirrorRaidSessionState 호출
-> FarmingLevel로 ServerTravel
```

`ULSSessionSubsystem`에는 첫 번째 제출 데이터를 legacy 세션 상태로 미러링한다. 2인 이상 MO 기준의 실제 레이드 중 원본은 각 `PlayerController`의 `ULSRaidInventoryComponent`다.

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

ServerTravel 이후 `BeginPlay`에서 기존 세션 상태를 복구해야 할 때:

```text
ALSPlayerControllerBase::InitializeRaidInventoryFromSessionSubsystem
-> RaidInventoryComponent가 비활성이고 SessionSubsystem이 active면
-> SessionSubsystem의 Inventory/Safe를 RaidInventoryComponent에 미러링
-> 서버면 ClientSyncRaidSessionAndLoot
```

이 경로는 legacy/복구용이다. 정상 MO 입장 경로에서는 `ALSLobbyGameMode`가 각 플레이어별 제출 payload를 `RaidInventoryComponent`에 직접 주입한다.

## 레이드 종료 트리거

`ALSFarmingGameMode`가 레이드 결과를 확정한다.

```text
ALSFarmingGameMode::OnExtraction
-> EndRaid(ELSRaidResult::Extracted)

ALSFarmingGameMode::OnPlayerDied
-> EndRaid(ELSRaidResult::Dead)

ALSFarmingGameMode::OnQuit
-> EndRaid(ELSRaidResult::Quit)
```

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
- SaveInventory = true
- SaveSafeStash = true

Dead
- Inventory = empty
- SafeStash = RaidInventoryComponent.SessionSafeInventory
- SaveInventory = true
- SaveSafeStash = true

Quit
- bAllowQuitRecovery가 true면 Inventory = SubmittedRaidLoadout
- bAllowQuitRecovery가 false면 Inventory = empty
- SaveInventory = true
- SaveSafeStash = false
```

결과 payload를 받은 `PlayerController`는 로컬 `ULSSaveSubsystem`에 반영한다.

```text
ALSPlayerControllerBase::RequestRaidResultSave
-> 로컬 컨트롤러면 ApplyRaidResultToLocalSave
-> 원격 컨트롤러면 ClientApplyRaidResult RPC

ALSPlayerControllerBase::ApplyRaidResultToLocalSave
-> bSaveInventory면 SaveSubsystem->ReplaceInventory
-> bSaveSafeStash면 SaveSubsystem->ReplaceSafeStash
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
-> ResultLevel로 OpenLevel
```

## 타임아웃과 실패 처리

입장 데이터 제출 대기:

```text
타임아웃: 10초
처리: ALSLobbyGameMode::HandleRaidEntryDataTimeout
동작:
- 제출하지 않은 PlayerController를 Warning 로그로 남김
- ClearRaidEntryDataWait
- 레이드 시작 취소
```

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
- 결과 저장 ACK가 누락되면 ResultLevel 이동이 멈춘다. 멀티 테스트에서 가장 먼저 확인해야 할 지점이다.
- `ServerTravel` 실패 시 현재 `ClearRaidEntryDataWait`를 호출하지만 일부 `RaidInventoryComponent`는 이미 시작된 뒤일 수 있다. 이 경우 복구 처리가 더 필요할 수 있다.

## 현재 남아 있는 legacy 경로

`ULSSessionSubsystem::EndRaid`는 자체적으로 결과 저장 후 `ResultLevel`로 이동하는 legacy 흐름을 갖고 있다.

현재 MO 기준 주 경로는 다음이다.

```text
ALSFarmingGameMode::EndRaid
-> BuildRaidResultForPlayer
-> PlayerController::RequestRaidResultSave
-> PlayerController::ServerConfirmRaidResultSaved
-> ALSFarmingGameMode::TravelToResultLevel
```

새 레이드 종료 로직을 추가할 때는 `ULSSessionSubsystem::EndRaid`를 직접 호출하지 말고, `ALSFarmingGameMode`의 결과 확정 경로를 기준으로 확장한다.

## 테스트 체크리스트

싱글/Listen Server:

```text
- Lobby에서 StartRaid 호출 시 FarmingLevel로 이동하는지
- Inventory와 SafeStash가 RaidInventoryComponent에 복사되는지
- 레이드 중 루팅/드랍 후 UI가 서버 상태로 다시 동기화되는지
- Extracted 결과에서 Inventory/SafeStash가 모두 저장되는지
- Dead 결과에서 Inventory는 비고 SafeStash는 유지되는지
- Quit 결과에서 bAllowQuitRecovery 값에 따라 Inventory 저장 결과가 달라지는지
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
  -> Each RaidInventoryComponent starts with its own payload
  -> ClientStartRaidSession mirrors local raid state
  -> ServerTravel(FarmingLevel)

Farming Raid
  -> Server owns each RaidInventoryComponent
  -> Loot/drop/sort changes happen on server
  -> ClientSyncRaidSessionAndLoot mirrors UI state

Raid End
  -> FarmingGameMode decides Extracted/Dead/Quit
  -> Build result from each RaidInventoryComponent
  -> ClientApplyRaidResult writes local SaveSubsystem
  -> ServerConfirmRaidResultSaved ACK
  -> All ACKed
  -> EndRaidInventory / ClearSubmittedRaidEntryData / ClearRaidSessionState
  -> OpenLevel(ResultLevel)

Result
  -> ResultGameMode::ReturnToLobby
  -> OpenLevel(LobbyLevel)
```
