# 더블클릭 빠른이동과 드래그 감지의 경쟁 (슬롯 구조 꼬임)

## 증상

인벤토리/창고 슬롯을 **빠르게 더블클릭**해 빠른이동(창고↔인벤토리, 장착 등)을 하면, 직후 드래그가 엉뚱한 슬롯을 잡거나 방금 옮긴 아이템을 다시 끌어 슬롯 구조가 꼬인다. 단일 클릭 드래그나 Shift+클릭 빠른이동만 쓸 때는 재현되지 않는다.

## 원인

한 번의 물리적 더블클릭 제스처가 Slate에서 `Down → Up → DoubleClick → Up` 순으로 들어온다. `ULSItemSlotWidget`에서:

1. 첫 `NativeOnMouseButtonDown`이 `UWidgetBlueprintLibrary::DetectDragIfPressed`로 **드래그 감지를 무장**한다.
2. 이어 `NativeOnMouseButtonDoubleClick`이 `TryHandleQuickTransfer`로 **데이터를 바꾸고 `RefreshAllInventoryUI`로 슬롯 풀을 리빌드**한다. 리빌드는 슬롯 위젯 인스턴스를 파괴하지 않고 재사용(`LSSlotWidgetSync::SyncSlotWidgets`)하므로, 같은 위젯이 **다른 아이템/인덱스**를 표시하게 된다.
3. 더블클릭 핸들러가 무장된 드래그를 취소하지 않으면, 리빌드 후 뒤늦게 `NativeOnDragDetected`가 발화해 **재사용돼 내용이 바뀐 슬롯**의 데이터로 드래그 오퍼레이션을 만든다 → 꼬임.

Shift+클릭이 멀쩡한 이유: `NativeOnMouseButtonDown`에서 빠른이동에 성공하면 `DetectDragIfPressed`에 도달하기 전에 `Handled`로 반환해 드래그가 애초에 무장되지 않기 때문이다.

## 수정

`ULSItemSlotWidget` (`Source/LostSignal/UI/Inventory/LSItemSlotWidget.cpp`):

1. `NativeOnMouseButtonDoubleClick`에서 빠른이동 성공 시 `FReply::Handled().ReleaseMouseCapture()`를 반환해 무장된 드래그 감지를 푼다. `DetectDragIfPressed`는 마우스 캡처에 의존하므로 캡처를 놓으면 드래그가 발화하지 못한다.
2. 안전망으로 `bSuppressNextDragDetect` 플래그를 둔다. 더블클릭 빠른이동 성공 시 세우고, `NativeOnDragDetected` 진입 시 이 플래그가 서 있으면 드래그 생성을 건너뛰고 플래그를 해제한다.
3. 플래그가 stale로 남아 정상 드래그를 막지 않도록, 새 클릭 제스처의 시작인 `NativeOnMouseButtonDown` 진입에서 플래그를 해제한다.
4. 플래그는 슬롯 재사용 시 지워지면 안 되므로 `ResetTransientSlotState`에서 건드리지 않는다(리빌드가 플래그를 초기화하면 안 됨).

## 재발 방지 체크

- 슬롯 위젯에 "제스처 중 데이터를 바꾸고 리빌드하는" 새 경로를 추가하면, 그 경로가 무장했을 수 있는 드래그 감지를 반드시 취소하는지 확인한다.
- 진단이 필요하면 `NativeOnDragDetected`/`NativeOnMouseButtonDoubleClick`에 `UE_LOG(LogLS, ...)`로 발화 순서를 찍어 캡처 해제가 드래그를 막는지 확인한다.

## 관련

- 슬롯 풀 재사용·드래그 비주얼 함정: [UIDragDropPackagedBuild.md](UIDragDropPackagedBuild.md)
- 데이터↔UI 단일 갱신 funnel 원칙: [../Systems/InventoryLogic.md](../Systems/InventoryLogic.md)
