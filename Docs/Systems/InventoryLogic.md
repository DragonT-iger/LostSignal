# Inventory Logic 정리

## 목적

이 문서는 인벤토리 UI와 슬롯 조작 로직을 정리한다.

저장 구조, 레이드 네트워크, 클라이언트/서버 신뢰 경계는 `Docs/ItemSaveNetworkStructure.md`를 기준으로 본다. 이 문서는 그 위에서 실제 슬롯이 화면에서 어떻게 표시되고 이동하는지에 집중한다.

Unity식으로 보면 `FLSSessionItem` 배열이 `List<ItemStack>`이고, 각 위젯은 그 배열을 보여주고 조작 요청을 보내는 View다. 레이드 중 실제 원본은 서버의 `ULSRaidInventoryComponent`이고, 로비의 영구 저장 원본은 로컬 `ULSSaveSubsystem`이다.

## 핵심 데이터와 영역

슬롯의 최소 단위는 `FLSSessionItem`이다.

```cpp
struct FLSSessionItem
{
	FName ItemRowName;
	int32 Amount = 0;
};
```

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

## UI 표시 흐름

`ULSInventoryWidget`은 인벤토리와 SafeStash 영역을 표시한다.

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

공통 슬롯 위젯은 `ULSItemSlotWidget`이다. 슬롯 context에 따라 인벤토리 슬롯, 루트 박스 슬롯, 창고 슬롯으로 동작한다.

```text
SetSlotContext
-> Inventory / Safe

SetLootSlotContext
-> LootBox

SetWarehouseSlotContext
-> Warehouse
```

아이콘은 슬롯의 `ItemRowName`을 기준으로 DataTable row를 찾고, row의 아이콘 경로를 로드한다. 아이콘 경로 문제로 로드에 실패하면 기본 텍스처를 표시하고, 빈 슬롯은 아이콘을 숨긴다.

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
ALSLootBox::Interact
-> 서버에서 드랍 결과 생성
-> ClientSyncRaidSessionAndLoot로 클라이언트 UI 미러링
```

루트 박스에서 인벤토리로 옮길 때는 `FLSDropResult`를 `FLSSessionItem` 형태로 변환하고, 레이드 중이면 서버의 `ULSRaidInventoryComponent`에 추가한다.

월드 드랍은 `ALSWorldDroppedItem`이 담당한다.

```text
DropSessionSlotToWorld
-> 레이드 중이면 RaidInventoryComponent 슬롯을 원본으로 사용
-> 레이드가 아니면 SaveSubsystem 저장 슬롯을 원본으로 사용
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

레이드 입장, 플레이어별 payload 제출, 결과 저장 ACK 정책은 `Docs/ItemSaveNetworkStructure.md`를 기준으로 한다.

인벤토리 로직 관점에서 지켜야 할 규칙은 다음과 같다.

- 레이드 중 UI는 서버가 확정한 자기 `RaidInventoryComponent` 상태를 미러링해서 보여준다.
- 레이드 중 클라이언트가 로컬 SaveGame 값을 다시 주장하지 않는다.
- 레이드 종료 결과 저장은 `ALSFarmingGameMode`가 만든 결과 payload를 클라이언트가 로컬 `SaveGame`에 반영하는 흐름이다.
- `ULSSessionSubsystem`은 아직 보조/레거시 API가 남아 있지만, 2인 이상 레이드의 플레이어별 원본으로 보지 않는다.

## 현재 주의점

- `ItemRowName` 접두사 규칙에 의존한다. 새 아이템 타입을 추가하면 아이콘 로드, 최대 스택, 정렬 키 처리도 같이 추가해야 한다.
- 아이콘과 DataTable은 UI 표시 중 동기 로드될 수 있다. 아이템 수가 많아지면 캐싱을 고려한다.
- Quit 복구에서 플레이어별 소모품 차감이 필요하면 `ULSRaidInventoryComponent`에 `ConsumedItems` 기록을 추가해야 한다.
- 로컬/PIE 다중 프로필 테스트가 필요하면 SaveGame을 `PlayerSaves[ProfileId]` 형태로 확장하는 설계는 `ItemSaveNetworkStructure.md`를 따른다.

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
