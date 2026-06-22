# UMG 드래그앤드롭 입력 죽음 (패키지 빌드)

아이템 슬롯(`ULSItemSlotWidget`) 드래그가 패키지 빌드에서 간헐적으로 죽는 문제의 원인·진단·수정 기록. 같은 증상이 다시 보이면 여기부터 읽는다.

## 증상

- **PIE는 정상**, 자립형 윈도우 패키지 빌드에서만 발생.
- 칩 스테이션/인벤토리/창고에서 드래그로 교환·장착·이동을 **반복하다 보면 특정 슬롯만 입력이 죽는다.** 전부가 아니라 일부.
- 죽은 슬롯도 **Shift+클릭(빠른 이동)은 계속 동작**한다. 드래그만 안 된다.

## 원인

`ULSItemSlotWidget::NativeOnDragDetected`에서 드래그 오퍼레이션의 비주얼을 **소스 슬롯 위젯 자신**으로 지정했던 것:

```
DragOperation->DefaultDragVisual = this;   // ← 문제
```

살아있는(위젯 트리에 붙어 있는) 위젯을 드래그 데코레이터로 재사용하면, 드래그가 끝난 뒤 그 위젯 인스턴스의 Slate 드래그 감지가 영구히 죽는다. 슬롯 위젯은 `LSSlotWidgetSync::SyncSlotWidgets`로 **풀링 재사용**되므로, 한 번 드래그 비주얼로 쓰인 위젯이 다른 칩 슬롯에 재배정되면 "그 슬롯이 죽은" 것처럼 보인다.

- Shift+클릭이 멀쩡한 이유: `NativeOnMouseButtonDown`에서 버튼 다운 한 번으로 처리가 끝나(빠른 이동), 드래그 감지 경로를 안 탄다.
- PIE에서 안 드러난 이유: Slate 위젯 정리/리페어런트와 풀링 타이밍이 에디터와 패키지 standalone에서 다르다.

코드 위치/구조는 헤더가 단일 출처다 → [LSItemSlotWidget.h](../../Source/LostSignal/UI/Inventory/LSItemSlotWidget.h), [LSItemSlotWidget.cpp](../../Source/LostSignal/UI/Inventory/LSItemSlotWidget.cpp).

## 진단 과정 (로그 기반)

각 단계에 임시 로그를 박아 "막힌 슬롯에서 어디까지 도달하는지"를 좁혔다. 결정적 관찰:

1. 막힌 슬롯도 `MouseEnter`/`MouseButtonDown(CanDrag=1)`/`MouseMove`가 정상 도달 → hit-test·데이터 정상.
2. `DetectDragIfPressed`가 `Handled=1`(드래그 감지 등록 성공)인데도 `OnDragDetected`가 안 뜸 → Slate가 그 위젯의 드래그를 무시.
3. 마우스 캡처 보유자(`Captor`)는 막힌 시점에도 비어 있음 → 캡처 누수 아님.
4. **위젯 인스턴스 포인터(`Ptr`)가 결정타**: 한 번 드래그 비주얼(`DefaultDragVisual`)로 쓰인 바로 그 인스턴스가, 이후 풀링 재사용됐을 때 `OnDragDetected`가 안 떴다. 정상 슬롯과 죽은 슬롯의 차이는 "그 인스턴스가 이전에 드래그 비주얼로 쓰였는가" 하나였다.

> 진단 로그(`[DragDiag]`)는 원인 확정 후 전부 제거했다.

## 수정

원인이 단일했으므로 수정도 한 곳이다. `LSItemSlotWidget.cpp`의 `NativeOnDragDetected`에서 드래그 비주얼을 **별도 인스턴스**로 만들어 넣는다(소스 위젯 `this`는 절대 비주얼로 쓰지 않는다):

```
ULSItemSlotWidget* DragVisual = CreateWidget<ULSItemSlotWidget>(OwningPlayer, GetClass());
DragVisual->SetItem(DragItemRowName, DragAmount, DragChipStats);
DragOperation->DefaultDragVisual = DragVisual;
```

별도 비주얼 인스턴스에는 `SetItem`만 호출한다. `SetDisplayOnlySlotContext`를 부르면 `bHasItem=false`가 되어 데코레이터 렌더 시 `NativePreConstruct → ClearItem`이 아이콘을 지워버리므로 호출하지 않는다.

> 진단 중 함께 시도했던 마우스 캡처 `NoCapture` 전환, 장착 슬롯 `ResetTransientSlotState` 추가, 드래그 소스 dim을 `HitTestInvisible` 대신 opacity로 바꾸기 등은 실제 원인이 아니어서 모두 원복했다(불필요한 diff 제거). 핵심 수정만으로 해결된다.

## 교훈 / 주의사항

- **UMG `UDragDropOperation::DefaultDragVisual`에 위젯 트리에 살아 있는 위젯(슬롯 본체 등)을 절대 넣지 말 것.** 항상 드래그용 별도 인스턴스를 만들어 넣는다. 특히 슬롯을 풀링 재사용하는 UI에서는 그 인스턴스가 영구히 드래그 불가가 된다.
- PIE에서만 검증하지 말 것. 입력/Slate 관련 버그는 패키지 standalone에서만 드러나는 경우가 있다. 패키지 검증 시 `LostSignal.exe -log`로 실행하고 `...\Windows\LostSignal\Saved\Logs\LostSignal.log`를 확인한다.
