# Inventory Logic 정리

## 목적

이 문서는 인벤토리 UI와 슬롯 조작 로직을 정리한다.

저장 구조, 레이드 네트워크, 클라이언트/서버 신뢰 경계는 [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md)를 기준으로 본다. 이 문서는 그 위에서 실제 슬롯이 화면에서 어떻게 표시되고 이동하는지에 집중한다.

## 핵심 데이터와 영역

슬롯의 최소 단위는 `FLSSessionItem`이다 (저장 포맷·필드 정의는 [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md)가 소유).

`ItemRowName`은 DataTable row id이고, 아이콘/이름/스탯/최대 스택 수는 DataTable에서 다시 읽는다. 같은 row가 여러 슬롯에 나뉘어 있을 수 있으며, 이는 `Item_Max`를 넘는 수량을 슬롯 단위로 표현하기 위한 정상 상태다.

현재 슬롯 영역은 `ELSInventorySlotArea`로 구분한다.

```text
Inventory
- 로비에서는 SaveGame의 Inventory
- 레이드 중에는 RaidInventoryComponent의 SessionInventory

Safe
- 로비에서는 SaveGame의 SafeStash 표시
- 레이드 중에는 RaidInventoryComponent의 SessionSafeInventory

Warehouse
- 로비 창고 SaveGame의 WarehouseItems
- 레이드 세션 인벤토리에는 포함되지 않음
```

저장/런타임 원본은 다음처럼 나뉜다.

```text
로비 Inventory / Safe / Warehouse
-> ULSSaveSubsystem

레이드 Inventory / Safe
-> ULSRaidInventoryComponent

세션 보조/레거시 상태
-> ULSSessionSubsystem
```

## 공용 슬롯 규칙

슬롯 조작 공통 규칙은 `LSInventorySlotUtils`에 모은다.

```text
Source/LostSignal/Inventory/LSInventorySlotUtils.h
Source/LostSignal/Inventory/LSInventorySlotUtils.cpp
```

주요 책임:

- 빈 슬롯 판정과 빈 아이템 생성
- `FLSDropResult`와 `FLSSessionItem` 변환
- DataTable 기반 최대 스택 계산
- 아이템 추가 시 기존 같은 row 슬롯을 먼저 채우고 남으면 빈 슬롯 사용
- 정렬 시 같은 row 수량을 합친 뒤 타입/테이블 순서로 정렬하고 `Item_Max` 기준으로 재분할
- 슬롯 교환, 같은 row 병합, 외부 아이템 드롭 처리

새 슬롯 조작이 필요하면 각 시스템에 복사하지 말고 이 유틸을 먼저 확장한다.

## 인벤토리 열기/닫기

인벤토리는 두 경로로 열린다(둘 다 `ALSPlayerCharacter` 소유).

- **컨테이너 연동:** 룻박스/로비 창고를 상호작용(`OnInteract`)하면 그 타깃 기준으로 열린다. 타깃과 멀어지면 Tick의 거리 검사(`UpdateInventoryWidgetDistance`)로 자동으로 닫힌다.
- **단독 토글:** Tab(`ToggleInventoryAction`)으로 컨테이너 없이 인벤토리만 연다. 단독으로 열린 인벤토리는 연동 컨테이너가 없으므로 거리 기반 자동 닫기 대상이 아니다(`bIsStandaloneInventoryOpen`). 전부 보관 버튼도 숨긴다.

닫기는 Tab(다시 누름)과 ESC(`MenuAction`)가 공유하는 `TryCloseOpenModalPanel`이 우선순위대로 처리한다. 칩스테이션([ChipSystem.md](ChipSystem.md) 소유)이 떠 있으면 인벤토리를 열지 않고 칩스테이션부터 닫고, 아니면 인벤토리를 닫는다(이때 함께 떠 있던 룻드랍/로비 창고도 같이 닫힌다). 닫을 모달이 없을 때만 Tab은 단독 인벤토리를 열고, ESC는 설정 메뉴를 연다(설정 UI 연결은 추후).

키 매핑은 Enhanced Input 에셋(`IA_Inventory`=Tab, `IA_Menu`=ESC)을 `IMC_Default`에서 연결하고, Pawn BP에서 두 `UInputAction` 슬롯에 할당한다. 공유 블러 토글은 [UILayerStructure.md](UILayerStructure.md)가 소유한다 — 위 show/hide가 컨트롤러 `UpdateBackgroundBlurVisibility()`를 호출해 자동 반영된다.

## UI 표시 흐름

`ULSInventoryWidget`은 인벤토리와 SafeStash 영역을 표시한다.

또한 `WeaponSlot`, `HeadphoneSlot`, `HeadSlot`, `GlovesSlot`, `BodySlot` 장비 슬롯을 `ULSItemSlotWidget`으로 바인딩한다. 슬롯 배경 텍스처는 각 `ULSItemSlotWidget`의 `DefaultSlotTexture`에서 지정하며, 값이 없으면 WBP 디자이너의 배경 브러시를 그대로 쓴다(아래 "UI 표시 흐름" 참고). 현재 이 5개 슬롯은 표시용이라 장착 저장/드래그 장착/장착 타입 검증 로직은 아직 연결하지 않는다.

```text
RebuildInventorySlots
-> 레이드 중이면 RaidInventoryComponent::GetSessionInventory
-> 레이드가 아니면 SaveSubsystem::GetInventory

RebuildConfirmedStorageSlots
-> 레이드 중이면 RaidInventoryComponent::GetSessionSafeInventory
-> 레이드가 아니면 SaveSubsystem::GetSafeStash
```

`ULSLobbyStorageWidget`은 로비 창고를 표시한다.

```text
RefreshStorage
-> SaveSubsystem::GetWarehouseItems
```

## 적재 프로토콜 슬롯 수

인벤토리와 Safe(보호슬롯)의 표시 슬롯 수와 실제 이동 제한은 `DT_Protocol`의 적재 프로토콜 row를 기준으로 계산한다.

```text
기본 Inventory 10칸 + 보이는 Protocol_Carrying Inventory 값 합산
기본 Safe 0칸 + 보이는 Protocol_Carrying Protected_Inventory 값 합산, 총합 최대 4칸
```

현재 레벨은 신호 게이지로 비활성화된 칩 슬롯을 제외한 장착 칩의 `Carrying` 합산값이다. 이전 레벨은 전체 장착 칩의 `Carrying` 합산값이며, 기존 `Protocol_Protected_Level` 규칙으로 보호 표시가 유지되는 row도 슬롯 보너스에 포함한다.

슬롯 수는 `ULSSaveSubsystem::GetMaxInventorySlotCount`, `ULSSaveSubsystem::GetMaxSafeStashSlotCount`, `ULSRaidInventoryComponent::GetMaxInventorySlotCount`, `ULSRaidInventoryComponent::GetMaxSafeSlotCount`를 통해 조회한다. UI에서 보이는 슬롯 수와 저장/레이드 드래그, 루팅, 월드 픽업 제한은 같은 값을 사용해야 한다.

공통 슬롯 위젯은 `ULSItemSlotWidget`이다. 슬롯 context에 따라 인벤토리 슬롯, 루트 박스 슬롯, 창고 슬롯으로 동작한다.

```text
SetSlotContext
-> Inventory / Safe

SetLootSlotContext
-> LootBox

SetWarehouseSlotContext
-> Warehouse
```

슬롯은 배경과 아이콘을 별도 위젯으로 겹쳐 표시한다. 슬롯 루트는 `Overlay`이고, 바닥에 `SlotBackgroundImage`(항상 표시되는 슬롯 배경 프레임), 그 위에 `ItemIconImage`(아이템 아이콘), 그 위에 `AmountText`를 둔다. 아이템 아이콘이 배경을 덮어쓰지 않으므로 아이템이 있어도 슬롯 배경이 유지된다.

`SlotBackgroundImage` 브러시는 `DefaultSlotTexture`로 C++가 설정하며, `DefaultSlotTexture`가 미지정이면 WBP 디자이너에서 설정한 배경 브러시를 그대로 둔다(이때 `UE_LOG(LogLS, Warning, ...)`). 호버/잠금/드래그 틴트는 배경과 아이콘 양쪽에 적용해 빈 슬롯에서도 피드백이 보인다.

평상시(특수 상태가 아닐 때) 배경 틴트는 아이템 등급색으로 칠한다. 등급은 Row Name 토큰에서 파싱하며(`LSInventorySlotUtils::ResolveItemGradeFromRowName`, 툴팁 등급 표기와 동일 출처), 6등급(`Supply/Standard/Precision/Tuning/Prototype/Masterpiece`)별 색은 `ULSItemSlotWidget`의 `*GradeColor` `UPROPERTY` 기본값으로 두고 디자이너가 조정한다. 등급이 없는 아이템은 `DefaultGradeColor`, 아이템이 없는 빈 슬롯은 `EmptySlotBackgroundColor`를 쓴다(둘 다 UI 시그니처 블루 `#124B6B` 기본값). 호버/잠금/드래그 등 특수 상태에서는 기존 피드백 틴트가 우선한다.

아이콘은 슬롯의 `ItemRowName`을 기준으로 DataTable row를 찾고, row의 아이콘 경로를 로드한다. 아이콘 경로 문제로 로드에 실패하면 기본 아이콘 텍스처를 표시하고, 빈 슬롯은 `ItemIconImage`를 `Collapsed`로 숨겨 배경만 보이게 한다.

## 드래그 앤 드롭

드래그 데이터는 `ULSInventoryDragDropOperation`에 들어간다.

```text
SourceInventoryWidget
SourceLootDropWidget
SourceStorageWidget
SourceSlotWidget
SourceSlotIndex
SourceSlotArea
SourceLootBox
```

지원하는 이동:

```text
Inventory <-> Inventory
Inventory <-> Safe
Safe <-> Safe
Inventory/Safe <-> LootBox
Inventory/Safe <-> Warehouse
Warehouse <-> Warehouse
Inventory/Safe/Warehouse -> WorldDroppedItem
WorldDroppedItem -> Inventory
```

레이드 중 `Warehouse`는 세션 대상이 아니므로 레이드 인벤토리 조작에서 제외한다. 로비에서 `Warehouse` 조작은 `ULSSaveSubsystem::DropStoredSlot`, `TransferStoredSlotToArea`, `ReplaceStoredSlotItem` 같은 저장 슬롯 API를 통해 처리한다.

## Shift-click 빠른 이동

빠른 이동은 열린 컨테이너 기준으로 동작한다.

```text
LootBox 슬롯 Shift+좌클릭
-> LootBox에서 Inventory로 이동

Inventory/Safe 슬롯 Shift+좌클릭
-> LootBox가 열려 있으면 LootBox로 이동
-> LootBox가 없고 LobbyStorage가 열려 있으며 레이드가 아니면 Warehouse로 이동
-> 인벤토리만 열려 있으면 아무 동작도 하지 않음

Warehouse 슬롯 Shift+좌클릭
-> Inventory로 이동
```

중요한 의도는 "인벤토리만 켜져 있는 상태에서는 Shift-click이 동작하지 않는다"이다. 빠른 이동은 대상 컨테이너가 명확할 때만 처리한다.

단, 빠른이동이 대상 부재 등으로 실패하면(`TryHandleQuickTransfer`가 false) 클릭을 소비하지 않고 일반 드래그 감지로 넘어간다. 달리기 키가 `LeftShift`라 뛰는 동안 Shift가 눌려 있어도, 인벤토리만 열린 상태에서 아이템을 슬롯 밖으로 드래그해 월드에 버릴 수 있다(월드 드랍은 드래그 취소 시 발생).

칩스테이션 안에서는 대상이 칩 장착(하드웨어)이라 컨테이너 조건 없이 동작한다.

```text
칩 목록 슬롯(인벤토리/창고의 칩) Shift+좌클릭
-> 첫 빈 장착 슬롯(마지막 인덱스부터 역방향)에 순서대로 장착
-> ULSChipStationWidget::QuickEquipChipToFirstEmptyHardwareSlot (SaveSubsystem::EquipChipFromStoredSlot)

칩 장착 슬롯 Shift+좌클릭
-> 창고로 해제
-> ULSChipStationWidget::QuickUnequipEquippedChipToWarehouse (SaveSubsystem::UnequipChipToWarehouse)
```

장착 슬롯 수의 단일 출처는 `ULSSaveSubsystem`이며, 첫 빈 칸 탐색은 `GetChipEquipmentSlots()`를 스캔한다. 모든 장착 칸이 차 있으면 장착하지 않고 `UE_LOG(LogLS, Warning, ...)`만 남긴다.

Shift+좌클릭 상태를 유지한 채 마우스를 다른 슬롯으로 이동하면, 마우스가 지나가는 아이템 슬롯은 같은 빠른 이동 규칙으로 차례대로 이동한다. 빈 슬롯, 잠긴 슬롯, 대상 컨테이너가 없는 상태는 기존 Shift-click과 같이 무시한다.

## 전부 보관

전부 보관 버튼은 로비 창고가 인벤토리와 함께 열린 상태에서만 보이며, 레이아웃 공간 유지를 위해 다른 상황에서는 `Hidden`으로 숨긴다.

```text
Inventory StoreAllButton
-> 로비 창고가 열려 있고 레이드가 아니면 Inventory에서 Warehouse로 이동
-> 레이드 중이거나 창고가 열려 있지 않으면 동작하지 않음
```

보관 순서는 Inventory 슬롯 배열 순서다. UI 기준으로 첫 번째 줄 왼쪽부터 오른쪽으로 처리하고, 마지막 칸 뒤에는 다음 줄로 넘어가는 것과 같다.

각 슬롯 아이템은 Warehouse에 같은 `ItemRowName` 슬롯이 있으면 먼저 `Item_Max`까지 스택하고, 남은 수량은 Warehouse의 앞쪽 빈 슬롯부터 채운다. Warehouse 최대 슬롯 수 안에 더 넣을 수 없으면 가능한 수량까지만 이동하고 즉시 중단한다. 남은 아이템은 원래 Inventory 슬롯에 남기며, 현재 구현에서는 경고 UI 대신 `UE_LOG(LogLS, Warning, ...)`만 남긴다.

## 루팅과 월드 드랍

루트 박스는 `ALSLootBox`가 서버에서 열고, 결과는 `FLSDropResult` 배열로 관리한다.

```text
ALSLootBox::Interact (서버 권한에서만 드랍 생성)
-> DropSubsystem::OpenRootingObject로 LootResults 생성
-> LootResults는 Replicated로 클라이언트에 동기화
-> PlayerController::ShowLootDropWidget(ClientShowLootDropWidget RPC)로 루팅 UI 표시
```

박스를 연 뒤 슬롯을 옮기는 transfer/drop 조작은 서버에서 확정된다. 레이드 중이면 `ClientSyncRaidSessionAndLoot`로 인벤토리/루팅 UI를 미러링하고, 레이드가 아니면(로비 파밍) `ClientRefreshLobbyLoot`로 루팅 UI와 SaveSubsystem 기반 인벤토리 UI를 다시 그린다.

루트 박스에서 인벤토리로 옮길 때는 `FLSDropResult`를 `FLSSessionItem` 형태로 변환한다.

```text
LootBox 슬롯 -> 인벤토리
-> 레이드 중이면 ALSLootBox::TransferLootSlotToSession / TransferLootSlotToSessionSlot (RaidInventoryComponent)
-> 레이드가 아니면 ALSLootBox::TransferLootSlotToSave / TransferLootSlotToSaveSlot (SaveSubsystem)

인벤토리 슬롯 -> LootBox
-> 레이드 중이면 ALSLootBox::TransferSessionSlotToLootSlot (RaidInventoryComponent)
-> 레이드가 아니면 ALSLootBox::TransferSaveSlotToLootSlot (SaveSubsystem)
```

월드 드랍은 `ALSWorldDroppedItem`이 담당한다.

```text
DropSessionSlotToWorld
-> 레이드 중이면 RaidInventoryComponent 슬롯을 원본으로 사용
-> 레이드가 아니면 SaveSubsystem 저장 슬롯을 원본으로 사용
-> 드래그 취소 이벤트 위치에서 캐릭터 위치로 향하는 2D 단위 방향을 서버에 전달하고, 서버가 캐릭터 발 위치 기준으로 드랍 위치만 확정
-> 슬롯을 먼저 비움
-> WorldDroppedItem 스폰
-> 스폰 실패 시 원래 슬롯 복구

ALSWorldDroppedItem::Interact
-> 레이드 중이면 RaidInventoryComponent::TryAddSessionItem
-> 레이드가 아니면 SaveSubsystem::TryAddToInventory
-> 일부만 들어가면 남은 수량으로 월드 아이템 갱신
```

## 정렬 흐름

정렬은 현재 슬롯 배치를 보존하지 않는다. 같은 `ItemRowName`의 수량을 전부 합친 뒤, 정렬 키 순서로 다시 배치하고 `Item_Max` 기준으로 슬롯을 재분할한다.

```text
Inventory 정렬
-> 레이드 중이면 RaidInventoryComponent::SortSessionInventory
-> 레이드가 아니면 SaveSubsystem::SortInventory

Warehouse 정렬
-> SaveSubsystem::SortWarehouseItems
```

정렬 키:

```text
Chip   -> 0      + DataTable row 순서
Weapon -> 100000 + DataTable row 순서
Armor  -> 200000 + DataTable row 순서
Item   -> 300000 + DataTable row 순서
```

이 정렬 키는 현재 구현 기준이다. 나중에 기획용 정렬 index 컬럼을 DataTable에 추가하거나 별도 정렬 테이블을 두면, DataTable row 순서 기반 정렬 대신 그 index 값을 기준으로 바뀔 수 있다. 같은 정렬 키면 row name 문자열 순서를 사용한다.

## 레이드와 저장 경계

레이드 입장, 플레이어별 payload 제출, 결과 저장 ACK 정책은 [RaidLevelFlow.md](RaidLevelFlow.md)를 기준으로 한다.

인벤토리 로직 관점에서 지켜야 할 규칙은 다음과 같다.

- 레이드 중 UI 표시의 신뢰 경계(클라이언트 값 불신, 서버 `RaidInventoryComponent` 미러만 표시)는 [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md)가 단일 출처다.
- 레이드 종료 결과 저장은 `ALSFarmingGameMode`가 만든 결과 payload를 클라이언트가 로컬 `SaveGame`에 반영하는 흐름이다.
- `ULSSessionSubsystem`은 아직 보조/레거시 API가 남아 있지만, 2인 이상 레이드의 플레이어별 원본으로 보지 않는다.

## 현재 주의점

- `ItemRowName` 접두사 규칙에 의존한다. 새 아이템 타입을 추가하면 아이콘 로드, 최대 스택, 정렬 키 처리도 같이 추가해야 한다.
- 저장 슬롯(`SaveSubsystem`)을 바꾸는 UI(칩스테이션 장착/해제 등)는 인벤토리 위젯을 폰 전용 `RebuildInventoryWidgetSlots`가 아니라 PC의 `RefreshActiveInventoryWidget`로 갱신해야 한다. 로비는 폰이 없어 폰 전용 경로가 무시되며, 그러면 인벤토리에 있던 칩을 장착해 슬롯을 비운 뒤 인벤토리 위젯이 stale로 남아 이후 창고 이동이 `GetInventory()[Index]` 빈 슬롯으로 실패한다.
- 아이콘과 DataTable은 UI 표시 중 동기 로드될 수 있다. 아이템 수가 많아지면 캐싱을 고려한다.
- Quit 복구에서 플레이어별 소모품 차감이 필요하면 `ULSRaidInventoryComponent`에 `ConsumedItems` 기록을 추가해야 한다.
- 로컬/PIE 다중 프로필 테스트가 필요하면 SaveGame을 `PlayerSaves[ProfileId]` 형태로 확장하는 설계는 [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md)를 따른다.

## 빠른 흐름도

```text
Lobby Inventory UI
  SaveSubsystem.Inventory / SafeStash / WarehouseItems
  -> ULSInventoryWidget / ULSLobbyStorageWidget
  -> ULSItemSlotWidget

Raid Start
  Client local SaveGame payload
  -> Server submitted loadout/safe
  -> PlayerController.RaidInventoryComponent

LootBox
  Server loot result
  -> RaidInventoryComponent
  -> ClientSyncRaidSessionAndLoot

Drag / Drop / Shift-click
  ULSItemSlotWidget
  -> ULSInventoryDragDropOperation
  -> PlayerController or SaveSubsystem
  -> Rebuild/Refresh widgets

Raid End
  FarmingGameMode builds result from each RaidInventoryComponent
  -> Client applies result to local SaveGame
```

## 적재 프로토콜 감소 처리

적재 프로토콜 감소로 슬롯 최대치가 줄어들면 일반 인벤토리와 보호 슬롯을 다르게 처리한다.

- 일반 인벤토리: 현재 최대 슬롯 수보다 뒤에 있는 아이템 슬롯은 기존 월드 드랍 흐름을 재사용해 플레이어 주변 바닥에 떨어뜨리고 원본 슬롯을 비운다.
- 보호 슬롯: 현재 최대 보호 슬롯 수보다 뒤에 있는 아이템은 보존하되 잠긴 슬롯으로 표시한다. 잠긴 보호 슬롯은 드래그, 드롭, Shift-click, 월드 드랍 대상/원본으로 사용할 수 없다.
- 로비에서 감소 시 초과 아이템을 창고로 넣거나 창 닫을 때 선택하게 하는 UX는 추후 결정한다. 현재 정책은 일반 슬롯 초과분 즉시 월드 드랍이다.

## 새 게임 기본 지급

타이틀의 New 버튼으로 새 게임을 시작할 때만 기본 아이템을 지급한다. 적용 경로는 `ULSTitleMenuWidget::HandleNewConfirmed()` -> `ULSSaveSubsystem::StartNewGame()`이다.

칩 기본 지급은 `ULSSaveSettings.bGrantLowestGradeChipsOnNewGame`과 `LowestGradeChipsStarterTargetArea`가 제어한다. 기본값은 켜짐이며 대상은 `Warehouse`다. `ChipTable`에서 가장 낮은 등급인 `Supply` RowName만 읽어 칩 종류별로 1개씩 지급하고, 칩 스탯은 지급 시점에 `LSChipStats::RollChipStats`로 확정한다.

추가 기본 아이템 목록은 `ULSSaveSettings.StarterItems`가 단일 출처다. 에디터에서는 `Project Settings > LS Save Settings`에서 `ItemRowName`, `Amount`, `TargetArea`를 설정한다. 수량/대상 영역이 잘못됐거나 슬롯 제한 때문에 전부 들어가지 못하면 `UE_LOG(LogLS, Warning, ...)`를 남긴다.
