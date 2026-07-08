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
- ServerTravel 후 플레이어별 인벤토리 복원용 PendingRaidEntries 큐 보유
- 1인/legacy 호환용 세션 미러 상태(SessionInventory/SessionSafeInventory)도 함께 보유
- 레이드 중 아이템의 원본은 PlayerController별 RaidInventoryComponent
- bAllowQuitRecovery: 중도 포기 시 출발 장비 복구 여부

ULSSettingsWidget
- 세팅 화면(WBP_Settings). 타이틀/로비/레이드(ESC) 어디서든 동일한 위젯을 재사용한다 — 레이드 전용 별도 메뉴는 없다
- 로비 기본 Play 화면에서는 ULSLobbyMenuWidget이 ESC 입력을 받아 설정 버튼과 같은 ShowSettingsWidget 경로로 이 위젯을 띄운다
- 레이드 중에는 ALSPlayerControllerBase가 ESC로 이 위젯을 직접 토글해서 띄운다
- "메인메뉴로 돌아가기"(MainMenuButton, ULSTitleMenuButtonWidget) 클릭 시: 레이드 여부와 무관하게 항상 타이틀로 나간다.
  - 레이드 중이 아니면(로비): 바로 TitleLevel로 OpenLevel
  - 레이드 중이면: 확인 다이얼로그 → bAllowQuitRecovery=true → ALSFarmingGameMode::OnQuit() → TitleLevel
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
Lobby -> Farming
- ALSLobbyGameMode::TryStartRaidWithSubmittedData
- Settings->FarmingLevel
- World->ServerTravel(FarmingLevelPath)

Farming -> Lobby (Extracted / Dead)
- ALSFarmingGameMode::TravelToResultLevel
- Settings->LobbyLevel
- UGameplayStatics::OpenLevelBySoftObjectPtr

Farming -> Title (Quit)
- ALSFarmingGameMode::TravelToResultLevel (PendingRaidResult == Quit)
- Settings->TitleLevel
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
-> ULSSaveSubsystem::GetEquipmentSlots
-> 서버면 StoreSubmittedRaidEntryData 후 LobbyGameMode 알림
-> 클라이언트면 ServerSubmitRaidEntryData RPC

ALSPlayerControllerBase::ServerSubmitRaidEntryData
-> StoreSubmittedRaidEntryData
-> ALSLobbyGameMode::NotifyRaidEntryDataSubmitted
```

서버는 각 `PlayerController`가 제출한 `Inventory`와 `SafeStash`를 `SubmittedRaidLoadout`, `SubmittedRaidSafeItems`에 저장한다. 저장 전에 `LSInventorySlotUtils::NormalizeSlotArray`로 슬롯 배열을 정규화한다. 무기/방어구 장비도 함께 제출해 `SubmittedRaidEquipment`에 저장하지만, 장비는 인덱스=슬롯타입 불변식이라 **Normalize하지 않고 SetNum(5) 패딩만** 한다. 이 3종(Inventory/SafeStash/Equipment)이 `EnqueuePendingRaidEntry`→`StartRaidInventory`→`ClientStartRaidSession` 체인을 타고 세션에 주입된다.

모든 플레이어가 제출을 완료하면:

```text
ALSLobbyGameMode::TryStartRaidWithSubmittedData
-> 모든 PlayerController의 HasSubmittedRaidEntryData 확인
-> 각 PlayerController마다:
   - SessionSubsystem->EnqueuePendingRaidEntry(PlayerLoadout, PlayerSafeItems)
   - RaidInventoryComponent->StartRaidInventory(PlayerLoadout, PlayerSafeItems)
   - ClientStartRaidSession(PlayerLoadout, PlayerSafeItems)
-> ULSSessionSubsystem::StartRaid / MirrorRaidSessionState 호출 (첫 번째 제출 데이터)
-> FarmingLevel로 ServerTravel
```

각 `PlayerController`의 제출 payload는 `SessionSubsystem`의 `PendingRaidEntries` 큐에 순서대로 적재된다. ServerTravel로 컨트롤러/컴포넌트가 새로 생성되면 직접 주입한 `StartRaidInventory` 결과는 유실되므로, 이 큐가 ServerTravel 이후 플레이어별 인벤토리를 복원하는 실제 경로다 (아래 "레이드 중 상태 원본" 참고).

`ULSSessionSubsystem`에는 첫 번째 제출 데이터를 legacy 세션 상태로 미러링한다. 2인 이상 MO 기준의 실제 레이드 중 원본은 각 `PlayerController`의 `ULSRaidInventoryComponent`다.

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

ServerTravel 이후 `BeginPlay`에서 각 `PlayerController`가 자기 인벤토리를 복원한다.

```text
ALSPlayerControllerBase::InitializeRaidInventoryFromSessionSubsystem
-> RaidInventoryComponent가 이미 active면 (서버면 ClientSyncRaidSessionAndLoot 후) 종료
-> SessionSubsystem이 active가 아니면 종료
-> SessionSubsystem::DequeuePendingRaidEntry 성공하면
   -> RaidInventoryComponent->StartRaidInventory(꺼낸 Inventory/Safe)   ← 정상 MO 복원 경로
-> 큐가 비어 있으면 (fallback)
   -> SessionSubsystem의 SessionInventory/SessionSafeInventory를 MirrorRaidInventoryState
-> 서버면 ClientSyncRaidSessionAndLoot
```

정상 MO 입장 경로에서는 `ALSLobbyGameMode`가 `EnqueuePendingRaidEntry`로 적재해 둔 플레이어별 payload를 각 `PlayerController`가 `DequeuePendingRaidEntry`로 꺼내 복원한다. 큐가 비는 경우(legacy/1인 미러 경로)에만 `SessionSubsystem`의 세션 미러 상태로 폴백한다.

## 레이드 종료 트리거

`ALSFarmingGameMode`가 레이드 결과를 확정한다.

```text
ALSFarmingGameMode::OnExtraction
-> EndRaid(ELSRaidResult::Extracted)

ALSFarmingGameMode::OnPlayerDied
-> DeathRaidEndDelaySeconds 타이머 (사망 연출 시간, EditDefaultsOnly)
-> EndRaid(ELSRaidResult::Dead)

ALSFarmingGameMode::OnQuit
-> EndRaid(ELSRaidResult::Quit)
```

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
-> 종료 결과가 Extracted 또는 Dead면 LobbyLevel로 OpenLevel (일단 ResultLevel 건너뜀)
-> 종료 결과가 Quit이면 TitleLevel로 OpenLevel (ResultLevel 건너뜀)
```

> 현재 탈출(Extracted) 성공과 사망(Dead)은 임시로 ResultLevel을 거치지 않고 바로 `LobbyLevel`로 복귀한다. 결과 레벨(전리품 정산 등)이 준비되면 이 분기를 제거하고 다시 ResultLevel을 거치도록 되돌린다.
>
> 중도 포기(Quit)는 레이드 자체를 그만두는 행동이라 결과를 보여줄 필요가 없어 `TitleLevel`로 바로 나간다. 레이드 중 ESC로 띄운 `ULSSettingsWidget`(WBP_Settings, 타이틀/로비와 동일한 위젯)의 "메인메뉴로 돌아가기"(BackButton)가 이 경로를 탄다 — 확인 다이얼로그를 거친 뒤 `ULSSessionSubsystem::bAllowQuitRecovery`를 true로 설정하고 `ALSFarmingGameMode::OnQuit()`을 호출해 출발 장비(SubmittedRaidLoadout)를 복구한다.

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
  -> OpenLevel(Extracted/Dead: LobbyLevel, Quit: TitleLevel)

Result
  -> ResultGameMode::ReturnToLobby
  -> OpenLevel(LobbyLevel)
```
