# Inventory Logic 정리

## 최근 인벤토리 변경 요약

최근 인벤토리 로직은 다음 순서로 확장됐다.

1. `24c77eb` - `ULSInventoryWidget` 초안 추가
   - `WBP_Inventory`, `WBP_InventoryItemSlot`, `WBP_InventoryEquipmentSlot` 추가.
   - `InventoryWrapBox`와 `ConfirmedStorageSlotWrapBox`에 슬롯 위젯을 동적으로 생성하도록 구성.
   - 슬롯 개수는 `InventorySlotCount`, `ConfirmedStorageSlotCount`로 조절.

2. `dc9d795` - 루트박스 상호작용 시 인벤토리 UI 표시
   - 플레이어가 상호작용 키를 누르면 주변 `ULSInteractable` 대상을 찾는다.
   - 대상이 `ALSLootBox`이면 서버 상호작용 요청 후 로컬 인벤토리 위젯을 연다.
   - 플레이어가 루트박스에서 멀어지면 위젯을 자동으로 닫는다.

3. `710e1e8` - 아이템 텍스트/이미지 연동 및 저장 연동
   - 슬롯 위젯이 `ItemRowName`, `Amount`를 받아 아이콘과 수량 텍스트를 표시한다.
   - `ItemRowName` 접두사에 따라 `Chip`, `Weapon`, `Armor`, `Item` DataTable을 찾는다.
   - DataTable의 `Icon_Path`를 통해 아이콘 텍스처를 로드한다.

4. `20120a5` - 아이템 수량 기준 슬롯 분할 및 획득 순서 반영
   - 같은 아이템이라도 DataTable의 `Item_Max`를 넘으면 여러 슬롯으로 나뉜다.
   - 새로 먹은 아이템은 기존 같은 아이템 슬롯을 먼저 채우고, 남으면 뒤쪽 슬롯에 추가된다.
   - 저장 데이터도 슬롯 배열 형태로 관리하기 시작했다.

5. `96bd799` - 드래그 교환, 정렬, 저장 구조 변경
   - `ULSInventoryDragDropOperation` 추가.
   - 슬롯 드래그로 세션 인벤토리 슬롯을 교환하거나 이동할 수 있다.
   - 레이드 시작 시 보관함 전체를 `LoadoutSnapshot`과 `SessionInventory`로 복사한다.
   - 레이드 결과에 따라 `SessionInventory` 또는 출발 장비 복구분을 다시 보관함에 저장한다.

6. `d1d206c` - 아이콘 누락 시 기본 텍스처 표시
   - DataTable row 또는 아이콘 경로 문제로 아이콘을 못 찾으면 엔진 기본 텍스처를 표시한다.
   - 빈 슬롯은 아이콘을 숨기고, 아이템이 있는데 아이콘만 없을 때 기본 텍스처가 보인다.

## 핵심 데이터 구조

현재 인벤토리의 최소 단위는 `FLSSessionItem`이다.

```cpp
struct FLSSessionItem
{
	FName ItemRowName;
	int32 Amount = 0;
};
```

의미는 "이 슬롯에 어떤 DataTable Row 아이템이 몇 개 들어있는가"이다.

Unity로 비유하면 `ScriptableObject`나 테이블 row id를 `ItemRowName`으로 들고 있고, 실제 인벤토리 배열은 `List<ItemStack>`처럼 슬롯 단위 스택 목록을 들고 있는 구조다.

현재 저장/세션에서 같은 구조를 같이 쓴다.

- `ULSSaveGame::Stash`: 영구 보관함. 세이브 파일에 저장된다.
- `ULSSessionSubsystem::SessionInventory`: 레이드 중 임시 인벤토리.
- `ULSSessionSubsystem::LoadoutSnapshot`: 레이드 입장 시점의 출발 장비 스냅샷.
- `ULSSessionSubsystem::ConsumedItems`: 레이드 중 사용한 출발 아이템 기록.
- `ULSSessionSubsystem::ResolvedItems`: 레이드 종료 후 최종 확정 아이템 목록.

## 레이드 입장 흐름

진입점은 `ALSLobbyGameMode::StartRaid()`이다.

1. `ULSSaveSubsystem`에서 현재 보관함 `GetStash()`를 읽는다.
2. 그 배열을 `ULSSessionSubsystem::StartRaid()`에 넘긴다.
3. `StartRaid()`는 같은 배열을 두 군데에 복사한다.
   - `LoadoutSnapshot.Items`: 출발 시점 복구용 원본.
   - `SessionInventory`: 실제 레이드 중 변경되는 인벤토리.
4. `bRaidActive = true`가 된다.
5. 파밍 레벨로 이동한다.

즉, 레이드 중에는 세이브 파일의 보관함을 직접 만지는 것이 아니라 `SessionInventory`만 바꾼다.

## 루트박스 상호작용 흐름

플레이어 입력 쪽 흐름은 `ALSPlayerCharacter::OnInteract()`에 있다.

1. 이미 인벤토리 UI가 열려 있으면 닫는다.
2. 마우스 위치를 월드 좌표로 변환한다.
3. 플레이어 주변의 `ULSInteractable` 액터를 찾는다.
4. 거리 점수와 마우스 방향 점수를 합산해서 가장 적절한 대상을 고른다.
5. 서버 RPC `ServerRequestInteract(BestTarget)`를 호출한다.
6. 대상이 `ALSLootBox`이면 로컬에서 인벤토리 위젯을 연다.

서버 쪽 흐름은 `ALSLootBox::Interact_Implementation()`이다.

1. 서버 권한이 없거나 이미 열린 박스면 종료한다.
2. `bIsOpened = true`로 바꾼다.
3. `ULSDropSubsystem::OpenRootingObject(RootingObjectRowName)`로 드랍 결과를 만든다.
4. 각 결과를 `ULSSessionSubsystem::AddSessionItem()`으로 `SessionInventory`에 추가한다.
5. `OnLootResultReceived(Results)` BP 이벤트로 결과 표시 쪽에 넘긴다.

주의할 점은 UI는 클라이언트가 바로 열고, 실제 아이템 추가는 서버에서 처리한다는 점이다. 현재 문서 기준으로는 루트박스 결과가 세션 인벤토리에 들어가고, UI는 다시 `RebuildInventorySlots()`를 호출할 때 그 배열을 읽어 표시한다.

## 아이템 추가 및 슬롯 분할

`AddSessionItem()`은 내부에서 `AddItemsToSlotArraySession()`을 사용한다. 세이브 쪽 `AddToStash()`도 거의 같은 방식이다.

처리 규칙은 다음과 같다.

1. `ItemRowName`이 비었거나 `Amount <= 0`이면 무시하고 경고 로그를 남긴다.
2. `ItemRowName` 접두사를 보고 DataTable을 고른다.
   - `Chip_` -> `ChipTable`
   - `Weapon_` -> `WeaponTable`
   - `Armor_` -> `ArmorTable`
   - `Item_` -> `ItemTable`
3. 해당 row의 `Item_Max`를 최대 스택 수로 사용한다.
4. 기존 슬롯 중 같은 `ItemRowName`이고 아직 `Item_Max`까지 차지 않은 슬롯을 먼저 채운다.
5. 그래도 남은 수량은 새 슬롯을 만들어 뒤에 추가한다.

예시는 다음과 같다.

- `Item_Max = 10`
- 현재 슬롯: `Item_Electricwire x7`
- 새 획득: `Item_Electricwire x8`
- 결과:
  - 기존 슬롯이 `x10`까지 찬다.
  - 남은 `x5`가 새 슬롯으로 추가된다.

그래서 같은 아이템 row가 배열에 여러 번 나올 수 있다. 이것은 버그가 아니라 `Item_Max`를 넘는 수량을 슬롯 단위로 표현하기 위한 의도다.

## 인벤토리 UI 표시 흐름

`ULSInventoryWidget::NativeConstruct()`에서 버튼 이벤트를 연결한 뒤 두 영역을 다시 만든다.

- `RebuildInventorySlots()`
- `RebuildConfirmedStorageSlots()`

`RebuildInventorySlots()`는 현재 상황에 따라 다른 데이터를 읽는다.

- 레이드 중이면 `ULSSessionSubsystem::GetSessionInventory()`.
- 레이드 중이 아니면 `ULSSaveSubsystem::GetStash()`.
- `SessionSubsystem`이 없으면 가능한 경우 `SaveSubsystem`의 보관함으로 fallback.

그 다음 `InventorySlotCount`와 실제 아이템 개수 중 더 큰 값만큼 슬롯 위젯을 만든다.

- 해당 인덱스에 아이템이 있으면 `SetItem(ItemRowName, Amount)`.
- 아이템이 없으면 `ClearItem()`.

`ConfirmedStorageSlotWrapBox`는 현재 확정 보관 영역처럼 보이지만, 지금 구현에서는 항상 빈 슬롯만 만든다. `StoreAllButton`도 클릭 로그만 있고 실제 동작은 아직 없다.

## 슬롯 아이콘 표시 흐름

`ULSInventoryItemSlotWidget::SetItem()`이 표시를 담당한다.

1. `ItemIconImage`, `AmountText` 바인딩을 확인한다.
2. `ItemRowName`이 비어 있으면 `ClearItem()`으로 빈 슬롯 처리한다.
3. `LoadIconTextureByRowName()`으로 아이콘을 찾는다.
4. 아이콘을 못 찾으면 `LoadDefaultIconTexture()`로 엔진 기본 텍스처를 사용한다.
5. 아이콘 이미지를 보이고, 수량 텍스트를 `FText::AsNumber(Amount)`로 표시한다.

아이콘 경로 규칙은 다음과 같다.

- `Icon_Path`가 `/Game/...` 전체 경로면 그대로 사용한다.
- 전체 경로지만 `.AssetName`이 없으면 자동으로 붙인다.
- 파일명만 있으면 접두사별 기본 폴더를 붙인다.
  - Chip: `/Game/LostSignal/UI/Icons/Chips/`
  - Weapon: `/Game/LostSignal/UI/Icons/Weapons/`
  - Armor: `/Game/LostSignal/UI/Icons/Armors/`
  - Item: `/Game/LostSignal/UI/Icons/Items/`

빈 슬롯은 `ClearItem()`에서 아이콘을 숨긴다. "인벤토리에 아무것도 없는 것도 기본 텍스처 로드" 변경은 빈 슬롯 표시가 아니라, 아이템은 있는데 아이콘 로드가 실패한 경우 기본 텍스처를 보여주는 의미에 가깝다.

## 드래그 앤 드롭 흐름

드래그는 `ULSInventoryItemSlotWidget`과 `ULSInventoryDragDropOperation`이 담당한다.

1. 빈 슬롯은 드래그가 시작되지 않는다.
2. 아이템이 있는 슬롯에서 좌클릭 드래그를 시작하면 `ULSInventoryDragDropOperation`을 만든다.
3. Operation에는 다음 정보가 들어간다.
   - 원본 인벤토리 위젯
   - 원본 슬롯 위젯
   - 원본 슬롯 인덱스
   - Shift 키 여부
4. 드래그 중 원본 슬롯은 투명도 `0.25`로 낮아진다.
5. 드롭 또는 취소 시 원본 슬롯 표시가 원상복구된다.

드롭 처리 규칙은 다음과 같다.

- 같은 `ULSInventoryWidget` 안에서만 드롭 가능하다.
- 레이드 중일 때만 슬롯 변경 가능하다.
- Shift를 누르지 않고 드래그하면 `SwapSessionInventorySlots()`.
- Shift를 누르고 드래그하면 `MoveSessionInventorySlot()`.

`SwapSessionInventorySlots()`:

- 대상 인덱스에 실제 아이템 슬롯이 있으면 두 슬롯을 교환한다.
- 대상 인덱스가 비어 있는 UI 슬롯이면 내부적으로 `MoveSessionInventorySlot()`로 처리한다.

`MoveSessionInventorySlot()`:

- 원본 슬롯을 배열에서 제거한다.
- 대상 인덱스를 `0 ~ SessionInventory.Num()` 범위로 보정한다.
- 해당 위치에 원본 아이템을 삽입한다.

중요한 제한은 드래그 정렬이 `SessionInventory`에만 적용된다는 점이다. 레이드가 아닐 때 보관함 UI를 보고 있으면 드래그 교환은 지원하지 않는다.

## 정렬 흐름

정렬 버튼은 `ULSInventoryWidget::HandleSortButtonClicked()`에서 처리한다.

- 레이드 중이면 `SessionSubsystem->SortSessionInventory()`.
- 레이드 중이 아니면 `SaveSubsystem->SortStash()`.

정렬은 단순히 배열 순서만 바꾸는 것이 아니라 다음 작업을 같이 한다.

1. 잘못된 슬롯을 제외한다.
2. 같은 `ItemRowName`의 수량을 전부 합친다.
3. 아이템 종류별 정렬 키를 만든다.
   - Chip: `0 + DataTable row 순서`
   - Weapon: `100000 + DataTable row 순서`
   - Armor: `200000 + DataTable row 순서`
   - Item: `300000 + DataTable row 순서`
4. 같은 정렬 키면 row name 문자열 순서로 정렬한다.
5. 다시 `Item_Max` 기준으로 슬롯을 나눈다.

즉, 정렬 버튼을 누르면 기존 슬롯 배치는 유지되지 않는다. 같은 아이템은 합쳐졌다가 다시 최대 스택 단위로 쪼개진다.

## 레이드 종료 및 저장 흐름

레이드 종료는 `ULSSessionSubsystem::EndRaid(ELSRaidResult Result)`에서 처리한다.

결과별 동작은 다음과 같다.

- `Extracted`
  - 현재 `SessionInventory` 전체가 `ResolvedItems`가 된다.
  - `SaveSubsystem->ReplaceStash(ResolvedItems)`로 보관함을 통째로 교체한다.

- `Dead`
  - `ResolvedItems`를 비운다.
  - 보관함 저장을 하지 않는다.
  - 현재 구현 기준으로 죽었을 때 기존 보관함이 즉시 비워지는 구조는 아니다. 단, 레이드 시작 때 보관함을 세션으로 복사만 했기 때문에 "출발 장비를 잃는 처리"가 실제 저장에 어떻게 반영되어야 하는지는 기획 결정이 더 필요하다.

- `Quit`
  - `bAllowQuitRecovery`가 true면 `BuildQuitRecovery()`를 저장한다.
  - 복구 목록은 `LoadoutSnapshot.Items`에서 `ConsumedItems` 수량만 뺀 것이다.
  - false면 저장하지 않는다.

저장은 `ULSSaveSubsystem::ReplaceStash()` 또는 `AddToStash()`가 `Save()`를 호출하면서 이루어진다.

비 Shipping 빌드에서는 `Saved/SaveGames/LostSignalSave_Debug.json`도 같이 써서 현재 보관함을 눈으로 확인할 수 있다.

## 현재 미구현/주의할 점

- `StoreAllButton`은 아직 실제 기능이 없다. 클릭 시 "not implemented yet" 로그만 남긴다.
- `ConfirmedStorageSlotWrapBox`는 빈 슬롯 표시만 한다. 실제 확정 보관함과 연결되어 있지 않다.
- 드래그 교환/이동은 레이드 중 `SessionInventory`에서만 동작한다.
- 정렬은 슬롯 배치를 보존하지 않는다. 같은 row를 합친 뒤 다시 나누기 때문에 수동 배치가 사라진다.
- 아이콘 로드는 `StaticLoadObject`와 DataTable 동기 로드를 사용한다. UI를 열 때마다 동기 로드가 발생할 수 있으므로 아이템 수가 많아지면 캐싱을 고려해야 한다.
- `ItemRowName` 접두사 규칙에 의존한다. 새 아이템 타입을 추가하면 아이콘 로드, 최대 스택, 정렬 키 처리도 같이 추가해야 한다.
- `Dead` 결과에서 보관함을 어떻게 처리할지 아직 설계가 애매하다. 현재 코드만 보면 탈출 성공 때만 보관함을 `SessionInventory`로 교체한다.

## 빠른 흐름도

```text
Lobby StartRaid
  SaveSubsystem.GetStash()
  -> SessionSubsystem.StartRaid(Stash)
     -> LoadoutSnapshot = Stash
     -> SessionInventory = Stash

LootBox Interact
  DropSubsystem.OpenRootingObject()
  -> SessionSubsystem.AddSessionItem()
     -> Item_Max 기준으로 기존 슬롯 채움
     -> 남은 수량은 새 슬롯 추가

Inventory UI Rebuild
  if RaidActive:
    read SessionInventory
  else:
    read SaveSubsystem.Stash
  -> slot widget 생성
  -> SetItem or ClearItem

Sort Button
  if RaidActive:
    SortSessionInventory()
  else:
    SortStash()
  -> 같은 row 합산
  -> 타입/테이블순 정렬
  -> Item_Max 기준 재분할

EndRaid
  Extracted:
    SaveSubsystem.ReplaceStash(SessionInventory)
  Quit with recovery:
    SaveSubsystem.ReplaceStash(LoadoutSnapshot - ConsumedItems)
  Dead:
    Save 안 함
```

