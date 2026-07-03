# 위젯 스위쳐에서 장착(드롭) 간헐 실패

위젯 스위쳐로 창고/인벤토리/칩스테이션을 띄웠을 때, **드래그는 되는데 장착(드롭)이 가끔 실패**하는 문제의 조사 기록.

> **상태: 조사 중 (원인 미확정).** 추측 패치 금지. 아래 진단 로그로 실패 순간을 먼저 확정한 뒤 한 곳만 수정한다. 패키지 빌드에서 드래그 자체가 죽는 별개 문제는 [UIDragDropPackagedBuild.md](UIDragDropPackagedBuild.md).

## 증상

- 위젯 스위쳐/오버레이 레이아웃으로 창고·인벤토리·칩스테이션을 띄운 상태에서 장착이 **가끔** 안 됨.
- **드래그는 정상**(아이콘이 커서를 따라옴). 놓는 순간에만 조용히 실패 — 에러/경고 없음.
- 실패 위치(슬롯 정중앙 vs 가장자리)와의 상관관계는 아직 불명(사용자도 "잘 모르겠음").

## 원인 분석 (드롭 라우팅)

드래그(`NativeOnDragDetected`)와 드롭(`NativeOnDrop`)은 완전히 분리돼 있다. 드래그는 소스 슬롯에서 시작되지만, **드롭은 "놓는 순간 커서 밑에 있는 위젯"에게만 전달**된다(Slate가 leaf→root로 버블링하며 `true`를 반환하는 위젯을 찾음).

따라서 장착 실패는 십중팔구 **드롭 이벤트가 장비 슬롯이 아닌 다른 위젯으로 라우팅**된 경우다. 그러면:

- 장비 슬롯(`WeaponSlot` 등 `ULSItemSlotWidget`)의 `NativeOnDrop`이 아예 안 불리고,
- 뒤에 깔린 인벤토리 창(`ULSInventoryWidget::HandleInventoryBackgroundDrop`)이 대신 받아, 포인터가 창 안이면 `false`를 반환하고,
- **아무 일도 안 일어남 = "장착이 안 됨"** (에러도 안 뜸).

드래그는 멀쩡한데 드롭만 조용히 실패하는 증상과 일치한다.

## 원인 후보 (가능성 순)

1. **겹친 히트테스트 패널이 드롭을 가로챔 (제일 유력)**
   위젯 스위쳐/오버레이 레이아웃으로 바꾸면서 장비 슬롯 뒤에 `Visibility=Visible`(=히트테스트됨)인 페이지 배경/오버레이가 깔린다. 슬롯 실제 사각형을 살짝 벗어난 지점에 놓으면 그 뒤 패널이 드롭을 먹고 `false`를 반환 → 장착 실패. 놓는 위치에 따라 되기도/안 되기도 하니 "가끔"으로 보인다.

2. **호버 확대(`HoveredRenderScale=1.1`)로 인한 착시 가장자리**
   드래그 타겟이 되면 슬롯이 1.1배로 커 보인다(`LSItemSlotWidget.h`의 `HoveredRenderScale`). 커 보이는 테두리에 놓으면 실제 드롭 대상은 그 뒤 위젯이 될 수 있다.

3. **탭 전환 직후 캐시 지오메트리 stale (가능성 낮음)**
   스위쳐가 페이지를 막 활성화한 프레임엔 슬롯 지오메트리가 아직 갱신 전이라 히트테스트가 어긋날 수 있음.

## 다음 단계 — 진단 로그로 확정

지난 드래그 death 버그([UIDragDropPackagedBuild.md](UIDragDropPackagedBuild.md))도 임시 로그로 "어디까지 도달하는지"를 좁혀서 잡았다. 이번에도 같은 방식으로 **실패 순간 어느 위젯의 `NativeOnDrop`이 불리고 소스/타겟 area가 뭔지**를 임시 `[EquipDiag]` 로그로 찍어 판별한다.

로그 넣을 지점 (코드 위치는 헤더/구현이 단일 출처):

- `ULSItemSlotWidget::NativeOnDrop` 진입부 — `this` 슬롯의 `SlotArea`/`SlotIndex`/`EquipmentSlotIndex`, 드롭옵의 source, 어느 분기를 타는지, 최종 반환값.
- `ULSInventoryWidget::NativeOnDrop` + `HandleInventoryBackgroundDrop` — 여기가 불리면 = 드롭이 장비 슬롯을 놓치고 뒷 배경으로 샌 것(후보 1·2 확정).
- `ULSInventoryWidget::HandleEquipmentSlotDrop` — 불리는데 `bChanged=false`면 라우팅은 맞고 로직 문제.

판별:

- 실패 시 **인벤토리 배경 `NativeOnDrop`**이 뜨면 → 후보 1/2 (히트테스트/레이아웃 수정).
- **장비 슬롯 `NativeOnDrop`**이 뜨는데 실패면 → 후보 3 또는 로직.

원인 확정 후 `[EquipDiag]` 로그를 전부 제거하고, 정확히 한 곳만 수정한다.

## 관련 파일

- `Source/LostSignal/UI/Inventory/LSItemSlotWidget.cpp` / `.h` — 드래그·드롭·히트테스트, `HoveredRenderScale`
- `Source/LostSignal/UI/Inventory/LSInventoryWidget.cpp` — 장비 드롭 경로(`HandleInventorySlotDrop` → `HandleEquipmentSlotDrop`), 배경 드롭
- `Source/LostSignal/UI/Inventory/LSInventoryDragDropOperation.cpp` — `Drop` / `DragCancelled`
- `Source/LostSignal/UI/Lobby/LSLoadoutPreparationWidget.cpp` — ContentSwitcher 레이아웃

슬롯 조작·드래그앤드롭 로직 전반은 [../Systems/InventoryLogic.md](../Systems/InventoryLogic.md)가 소유한다.
