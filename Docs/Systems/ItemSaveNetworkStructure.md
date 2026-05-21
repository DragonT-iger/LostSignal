# 아이템 저장/슬롯 구조 메모

## 목적

인벤토리 UI, 드래그 앤 드롭, Shift-click, 정렬 같은 화면 조작 규칙은 `Docs/InventoryLogic.md`를 기준으로 본다. 이 문서는 저장 구조, 레이드 네트워크, 클라이언트/서버 신뢰 경계를 중심으로 다룬다.

현재 방향은 다음 세 가지를 동시에 만족하는 것이다.

- 싱글/로컬 테스트에서는 기존 `SaveGame`을 계속 사용한다.
- 멀티플레이는 전체 월드 MMO가 아니라, 매칭 후 레이드만 같이 뛰는 MO 방식으로 본다.
- DB 서버는 쓰지 않고, 기본 영구 데이터는 각 플레이어 로컬 `SaveGame`에 저장한다.

핵심은 **언제까지 로컬 저장 데이터를 믿고, 언제부터 서버 상태만 믿을 것인가**이다.

매칭/레이드 입장 시점에 각 클라이언트가 자기 로컬 저장 데이터를 서버에 제출한다. 이 최초 제출 데이터는 DB 서버가 없으므로 어쩔 수 없이 신뢰한다. 하지만 서버가 그 데이터를 레이드 세션 상태로 복사한 뒤부터는 클라이언트의 인벤토리 값을 다시 믿지 않는다. 레이드 중 루팅, 사용, 드랍, 사망, 탈출 결과는 서버 상태만 기준으로 확정한다.

Unity식으로 보면 클라이언트 인벤토리는 View/캐시이고, 서버의 인벤토리/저장 관리자가 실제 데이터 원본이다.

## 현재 코드 구조

현재 아이템 슬롯의 최소 저장 단위는 `FLSSessionItem`이다.

```text
FLSSessionItem
- ItemRowName
- Amount
```

즉, 저장되는 값은 “어떤 DataTable Row 아이템이 몇 개 있는가”이다. 아이템 이름, 아이콘, 스탯, 최대 스택 수는 `DT_Item`, `DT_Weapon`, `DT_Armor`, `DT_Chip` 같은 DataTable에서 런타임에 다시 읽는다.

루팅 결과는 별도 타입인 `FLSDropResult`를 사용한다.

```text
FLSDropResult
- ItemRowName
- Amount
- ItemText
```

`FLSDropResult`는 루트 박스 UI/표시용 텍스트를 포함하지만, 인벤토리나 저장으로 이동할 때는 `ItemRowName + Amount`만 `FLSSessionItem`으로 변환해서 다룬다.

## 저장 영역

현재 `ULSSaveGame`은 한 명 기준 저장 데이터를 직접 가진다.

```text
ULSSaveGame
- Inventory
- WarehouseItems
- SafeStash
- bRaidSaveActive
- ActiveRaidLoadout
- ActiveRaidConsumedItems
```

역할은 다음처럼 구분한다.

```text
Inventory
- 로비에서 출발 인벤토리로 쓰는 저장 슬롯
- 레이드 시작 시 Loadout으로 복사됨

WarehouseItems
- 로비 장기 창고
- 로비 창고 UI가 표시하고 조작함

SafeStash
- 레이드 중에도 보이는 안전 보관 슬롯
- 로비 인벤토리 UI의 ConfirmedStorageSlot 영역이 이 데이터를 표시함

ActiveRaidLoadout / ActiveRaidConsumedItems
- 레이드 중단, 강제 종료, PIE 중단 같은 상황에서 복구하기 위한 임시 저장 데이터
```

현재 구조는 싱글 플레이 기준이다. 2명 이상이 되면 저장 데이터가 누구의 것인지 구분할 수 없으므로, 멀티 전환 시 플레이어별 저장 payload로 확장해야 한다.

## 공용 슬롯 유틸

현재 슬롯 조작의 중복 로직은 `LSInventorySlotUtils`로 모으는 방향으로 정리되어 있다.

```text
Source/LostSignal/Inventory/LSInventorySlotUtils.h
Source/LostSignal/Inventory/LSInventorySlotUtils.cpp
```

이 유틸이 담당하는 것:

```text
IsFilled
MakeEmptyItem
ToSessionItem
SetDropResultFromSessionItem
ClearDropResult
ResolveItemMaxStack
EnsureSlotIndex
AddItemsToSlotArray
TryAddItemsToSlotArray
NormalizeSlotArray
RemoveItemsFromSlotArray
SortAndCompactSlotArray
SwapSlots
MoveSlotWithinArray
DropSlot
DropExternalItemToSlot
DropResultSlot
```

앞으로 슬롯 병합, 최대 스택, 정렬, 슬롯 이동, 루팅 슬롯 변환 로직은 각 시스템에 복사하지 말고 이 유틸을 먼저 확장한다.

현재 이 유틸을 사용하는 주요 시스템:

```text
ULSSaveSubsystem
ULSSessionSubsystem
ULSRaidInventoryComponent
ALSLootBox
ULSLootDropWidget
ULSLobbyStorageWidget
```

## 로비 저장 흐름

로비에서는 `ULSSaveSubsystem`이 저장 데이터의 원본이다.

```text
ULSSaveSubsystem
- SaveData->Inventory
- SaveData->WarehouseItems
- SaveData->SafeStash
```

로비 인벤토리 UI는 레이드가 비활성일 때 `SaveSubsystem->GetInventory()`를 표시한다.

로비 창고 UI는 `SaveSubsystem->GetWarehouseItems()`를 표시한다.

로비 인벤토리 UI의 안전 보관 슬롯 영역은 `SaveSubsystem->GetSafeStash()`를 표시한다.

로비에서 가능한 조작:

```text
Inventory <-> WarehouseItems
- ULSInventoryWidget
- ULSLobbyStorageWidget
- ULSSaveSubsystem::DropStoredSlot
- ULSSaveSubsystem::TransferStoredSlotToArea

Inventory/Safe/Warehouse -> WorldDroppedItem
- ALSPlayerControllerBase::DropSessionSlotToWorld
- 레이드가 아니면 ULSSaveSubsystem의 저장 슬롯을 원본으로 사용

WorldDroppedItem -> Inventory
- ALSWorldDroppedItem::Interact
- 레이드가 아니면 ULSSaveSubsystem::TryAddToInventory 사용
```

주의할 점:

- `WarehouseItems`는 로비 창고다.
- `SafeStash`는 안전 보관함이다.
- 로비 인벤토리 UI의 ConfirmedStorageSlot은 현재 `SafeStash`를 본다.
- 로비 창고 UI의 슬롯은 `Warehouse` 영역으로 조작한다.

## 레이드 런타임 흐름

레이드 중에는 `ULSRaidInventoryComponent`가 플레이어 컨트롤러에 붙어서 런타임 인벤토리를 들고 있다.

```text
ULSRaidInventoryComponent
- SessionInventory
- SessionSafeInventory
- bRaidActive
- LoadoutSnapshot
- ConsumedItems
```

레이드 시작 흐름:

```text
1. LobbyGameMode가 각 PlayerController에 레이드 입장 데이터 제출을 요청
2. 로컬/싱글 플레이어는 서버에서 직접 SaveSubsystem의 Inventory/SafeStash를 읽음
3. 원격 클라이언트는 ClientRequestRaidEntryData를 받고 자기 로컬 SaveSubsystem 데이터를 ServerSubmitRaidEntryData로 제출
4. 서버는 제출받은 데이터를 PlayerController의 SubmittedRaidLoadout/SubmittedRaidSafeItems에 저장
5. 모든 PlayerController 제출이 끝나면 각 RaidInventoryComponent에 플레이어별 Loadout/SafeStash 주입
6. 클라이언트에는 ClientStartRaidSession으로 자기 데이터만 미러링
7. 기존 SessionSubsystem 전역 1인 흐름은 첫 번째 제출 데이터를 legacy 시작 데이터로 사용
```

주의할 점:

- 레이드 입장 데이터 제출은 ACK 대기 방식이며, 제한 시간 안에 제출하지 않은 PlayerController가 있으면 레이드 시작을 중단하고 로그를 남긴다.
- `ALSPlayerControllerBase::InitializeRaidInventoryFromSessionSubsystem`은 이미 `RaidInventoryComponent`가 활성 상태면 전역 `SessionSubsystem` 데이터로 다시 덮어쓰지 않는다.
- 이 보호가 없으면 ServerTravel 이후 모든 플레이어 인벤토리가 첫 번째 플레이어 데이터로 덮일 수 있다.

레이드 중 조작:

```text
LootBox -> RaidInventory
RaidInventory -> LootBox
RaidInventory Inventory <-> Safe
RaidInventory -> WorldDroppedItem
WorldDroppedItem -> RaidInventory
```

이 조작들은 서버의 `ALSPlayerControllerBase`를 통해 확정되고, 결과는 `ClientSyncRaidSessionAndLoot`로 클라이언트에 미러링된다.

## 레이드 종료 흐름

현재 `ALSFarmingGameMode::EndRaid`는 서버의 플레이어별 `ULSRaidInventoryComponent`를 원본으로 삼아 결과 payload를 만든다.

```text
ALSFarmingGameMode::EndRaid
-> 각 PlayerController의 RaidInventoryComponent 순회
-> Result 규칙에 따라 Inventory/SafeStash 결과 payload 생성
-> ClientApplyRaidResult로 각 클라이언트 로컬 SaveGame에 저장 요청
-> ServerConfirmRaidResultSaved ACK 대기
-> 모든 ACK 수신 후 ResultLevel로 이동
```

현재 구조에서는 `ULSRaidInventoryComponent`가 레이드 중 실제 원본이고, 각 클라이언트의 `ULSSaveSubsystem`이 영구 저장 원본이다.

```text
Extracted
- 해당 플레이어의 SessionInventory를 Inventory로 저장
- 해당 플레이어의 SessionSafeInventory를 SafeStash로 저장

Dead
- Inventory는 비움
- 해당 플레이어의 SessionSafeInventory를 SafeStash로 저장

Quit
- bAllowQuitRecovery가 true면 SubmittedRaidLoadout을 Inventory로 저장
- bAllowQuitRecovery가 false면 빈 Inventory를 저장
- SafeStash는 저장하지 않고 기존 로컬 값을 유지
```

주의할 점:

- 결과 저장 ACK가 제한 시간 안에 오지 않으면 ResultLevel 이동을 중단하고 미응답 PlayerController를 로그로 남긴다.
- 레이드 종료 성공 시 `ULSSessionSubsystem::ClearRaidSessionState`를 호출해 전역 1인 세션 상태가 다음 레벨에서 다시 미러링되지 않게 한다.
- 현재 Quit 복구는 제출된 출발 Loadout 기준이다. 플레이어별 소모품 차감은 아직 별도 기록이 없으므로 추후 확장 지점이다.

## UI/드래그 앤 드롭 흐름

공통 슬롯 위젯은 `ULSItemSlotWidget`이다. 슬롯이 어느 컨테이너 소속인지 context로 구분한다.

```text
SetSlotContext
- 인벤토리/안전 보관 슬롯

SetLootSlotContext
- 루트 박스 슬롯

SetWarehouseSlotContext
- 로비 창고 슬롯
```

드래그 데이터는 `ULSInventoryDragDropOperation`에 담긴다.

```text
SourceInventoryWidget
SourceLootDropWidget
SourceLobbyStorageWidget
SourceSlotWidget
SourceSlotIndex
SourceSlotArea
```

지원하는 주요 이동:

```text
Inventory -> Inventory
Inventory -> Safe
Safe -> Inventory
Inventory/Safe -> LootBox
LootBox -> Inventory/Safe
Warehouse -> Inventory/Safe
Inventory/Safe -> Warehouse
Warehouse -> Warehouse
Inventory/Safe/Warehouse -> WorldDroppedItem
```

Shift 클릭 동작:

```text
LootBox 슬롯 Shift+좌클릭
-> LootBox에서 인벤토리로 빠른 이동

Inventory 슬롯 Shift+좌클릭
-> LootBox가 열려 있으면 LootBox로 이동 시도
-> 로비 창고가 같이 열려 있고 레이드가 아니면 Warehouse로 이동 시도
-> 인벤토리만 열려 있으면 아무 동작도 하지 않음

Warehouse 슬롯 Shift+좌클릭
-> Inventory로 빠른 이동
```

드래그 취소 시 월드 드랍:

```text
InventoryWidget에서 시작한 드래그 취소
-> 인벤토리 창 밖이면 TryDropInventoryDragToWorld

LobbyStorageWidget에서 시작한 드래그 취소
-> UI 위가 아니면 TryDropStorageDragToWorld
```

## 월드 드랍/픽업 흐름

월드에 떨어진 아이템은 `ALSWorldDroppedItem`이 담당한다.

드랍:

```text
ALSPlayerControllerBase::DropSessionSlotToWorld
-> 레이드 중이면 RaidInventoryComponent의 슬롯을 원본으로 사용
-> 레이드가 아니면 SaveSubsystem의 저장 슬롯을 원본으로 사용
-> 슬롯을 먼저 비움
-> ALSWorldDroppedItem 스폰
-> 스폰 실패 시 원래 슬롯 복구
```

픽업:

```text
ALSWorldDroppedItem::Interact
-> 레이드 중이면 RaidInventoryComponent::TryAddSessionItem
-> 레이드가 아니면 SaveSubsystem::TryAddToInventory
-> 다 들어가면 액터 Destroy
-> 일부만 들어가면 남은 수량으로 월드 아이템 갱신
```

## MO 멀티 저장 원칙

LostSignal의 멀티는 MO 방식이다.

```text
로비/개인 진행
- 각 플레이어 로컬 SaveGame 사용
- DB 서버 없음
- 플레이어의 기본 영구 데이터는 로컬 파일을 신뢰

매칭/레이드 입장
- 클라이언트가 로컬 Inventory/SafeStash 등을 서버에 제출
- 서버는 이 데이터를 플레이어별 RaidInventoryComponent로 복사
- 이 시점 이후 레이드 중 클라이언트 인벤토리 값은 신뢰하지 않음

레이드 중
- 서버가 루팅/사용/드랍/사망/탈출을 전부 확정
- 클라이언트는 요청과 UI 표시만 담당

레이드 종료
- 서버가 플레이어별 최종 결과를 클라이언트에 전달
- 클라이언트는 서버가 보낸 결과를 자기 로컬 SaveGame에 저장
```

즉, 신뢰 경계는 다음과 같다.

```text
레이드 입장 전:
클라이언트 로컬 SaveGame을 신뢰

레이드 입장 순간:
클라이언트 데이터를 서버 세션 상태로 복사

레이드 중:
서버 세션 상태만 신뢰

레이드 종료 후:
서버 결과를 클라이언트 로컬 SaveGame에 반영
```

이 구조는 치트에 완전히 강한 구조는 아니다. 하지만 DB 서버를 두지 않는 조건에서는 현실적인 절충안이다. 중요한 것은 레이드 중간에 클라이언트가 “내 인벤토리는 이렇다”고 다시 주장하지 못하게 하는 것이다.

## 멀티 전환 시 권장 구조

한 PC에서 여러 테스트 프로필을 돌리거나, PIE에서 2인 테스트를 하려면 `ULSSaveGame` 안에 플레이어별 저장 payload를 둘 수 있다.

```text
ULSSaveGame
└── PlayerSaves
    ├── "Player1"
    │   ├── Inventory
    │   ├── WarehouseItems
    │   ├── SafeStash
    │   ├── bRaidSaveActive
    │   ├── ActiveRaidLoadout
    │   └── ActiveRaidConsumedItems
    │
    └── "Player2"
        ├── Inventory
        ├── WarehouseItems
        ├── SafeStash
        ├── bRaidSaveActive
        ├── ActiveRaidLoadout
        └── ActiveRaidConsumedItems
```

예상 구조체:

```cpp
USTRUCT(BlueprintType)
struct FLSPlayerInventorySave
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FLSSessionItem> Inventory;

	UPROPERTY()
	TArray<FLSSessionItem> WarehouseItems;

	UPROPERTY()
	TArray<FLSSessionItem> SafeStash;

	UPROPERTY()
	bool bRaidSaveActive = false;

	UPROPERTY()
	TArray<FLSSessionItem> ActiveRaidLoadout;

	UPROPERTY()
	TArray<FLSSessionItem> ActiveRaidConsumedItems;
};
```

초기 로컬 테스트 키:

```text
Player1
Player2
Player3
```

나중에 온라인 매칭에서 식별용으로 사용할 수 있는 키:

```text
Steam_7656...
EOS_xxxxx
Account_1234
```

중요한 설계 원칙:

- 저장 payload 구조와 플레이어 식별 키를 분리한다.
- 현재는 로컬 테스트용으로 `Player1`, `Player2`를 쓴다.
- 실제 MO 매칭에서는 각 클라이언트가 자기 로컬 저장 데이터를 서버에 제출한다.
- 서버는 레이드 시작 시 받은 초기 데이터만 신뢰하고, 레이드 중에는 서버 상태만 신뢰한다.
- 레이드 종료 결과는 서버가 계산해서 각 클라이언트에 돌려주고, 클라이언트가 자기 로컬 SaveGame에 저장한다.

## 멀티 전환 상태와 남은 지점

현재 구현 상태:

```text
LSLobbyGameMode::StartRaid
- 클라이언트 로컬 저장 데이터 제출 구현됨
- 플레이어별 RaidInventoryComponent 주입 구현됨
- 제출 ACK 타임아웃 실패 처리 구현됨

ALSFarmingGameMode::EndRaid
- 플레이어별 RaidInventoryComponent 결과 payload 생성 구현됨
- ClientApplyRaidResult / ServerConfirmRaidResultSaved ACK 저장 구현됨
- 결과 저장 ACK 타임아웃 실패 처리 구현됨
- 모든 ACK 이후 ResultLevel 이동 구현됨

ULSSessionSubsystem
- 레거시/보조 상태로 남아 있음
- ResultLevel 이동 전 ClearRaidSessionState로 stale 상태를 비움

ULSSaveSubsystem
- 현재 SaveData에 단일 Inventory/Warehouse/SafeStash 보유
- 로컬 테스트 다중 프로필이 필요하면 PlayerSaves[ProfileId] 구조로 확장
- 실제 MO 매칭에서는 각 클라이언트 로컬 SaveGame이 영구 저장 원본
```

선호 방향:

```text
레이드 중 원본
-> 각 PlayerController의 ULSRaidInventoryComponent

영구 저장 원본
-> 각 클라이언트의 로컬 ULSSaveSubsystem

플레이어 구분
-> PlayerState 또는 서버 PlayerController의 SaveProfileId

클라이언트
-> 레이드 입장 전에는 로컬 SaveGame 표시
-> 레이드 중에는 서버가 보내준 자기 인벤토리 미러만 표시
-> 레이드 종료 후 서버 결과를 로컬 SaveGame에 저장
```

## 다음 구현 순서

현재까지 구현된 순서:

```text
1. 로컬 SaveGame에서 레이드 입장용 payload를 뽑는 흐름 구현
2. 클라이언트가 서버에 초기 payload를 제출하는 RPC 구현
3. 서버가 제출 payload를 플레이어별 RaidInventoryComponent에 복사
4. 서버가 레이드 중 모든 아이템 변경을 확정하는 기존 흐름 유지
5. 레이드 종료 시 서버가 플레이어별 결과 payload 생성
6. 서버가 각 클라이언트에 결과 payload 전송
7. 클라이언트가 결과 payload를 로컬 SaveGame에 반영
8. 결과 저장 ACK 이후 ResultLevel 이동
9. ResultLevel 이동 전 SessionSubsystem의 stale 전역 상태 제거
```

남은 확장 지점:

```text
- 로컬/PIE 다중 테스트가 필요하면 PlayerSaves[ProfileId] 구조 추가
- Quit 복구에서 플레이어별 소모품 차감이 필요하면 RaidInventoryComponent에 ConsumedItems 기록 추가
- 입장/결과 ACK timeout 값을 설정 에셋으로 빼기
```

## 테스트 체크리스트

싱글/로컬:

```text
- Inventory 정렬/이동/스택 병합
- Warehouse 정렬/이동/스택 병합
- SafeStash 표시와 이동
- Inventory <-> Warehouse 드래그
- Warehouse <-> Warehouse 드래그
- Shift 클릭 빠른 이동
- 인벤토리/창고 슬롯 월드 드랍
- 월드 아이템 픽업
- 레이드 시작 후 Loadout/SafeStash 미러링
- 레이드 탈출/사망/중도 종료 저장
```

2인 테스트 준비 후:

```text
- 각 클라이언트가 자기 로컬 SaveGame에서 레이드 입장 payload를 제출하는지
- 서버가 입장 이후 클라이언트 인벤토리 재주장을 받지 않는지
- Player1 루팅 결과가 Player2 레이드 상태에 섞이지 않는지
- Player2 사망 결과가 Player1 레이드 결과에 영향 없는지
- 레이드 종료 후 서버 결과가 각 클라이언트 로컬 SaveGame에 반영되는지
- 강제 종료/연결 해제 시 어떤 결과를 저장할지 정책이 명확한지
```
