# 로비 포커스 회수가 외부 모달의 첫 클릭을 먹는 문제

## 증상

로비에서 뜨는 확인/알림 다이얼로그 — 예: 칩 스테이션의 "인벤토리 용량이 부족합니다" 경고 — 의 **확인/취소 버튼을 두 번 눌러야 닫힌다.** 첫 클릭은 씹히고 두 번째 클릭에야 버튼이 반응한다. 타이틀/세팅 화면에서 같은 `WBP_ConfirmDialog`를 버튼 클릭으로 열면 한 번에 닫히는데, 로비의 슬롯 제스처(더블클릭/Shift/드래그드롭)로 뜨는 다이얼로그만 재현된다.

## 원인

포인터 캡처나 next-tick 생성 타이밍 문제가 **아니다**(그 방향으로 고쳐도 안 고쳐진다). 진짜 원인은 **로비 메뉴의 매 틱 포커스 회수**다.

`ULSLobbyMenuWidget::NativeTick`은 TAB/ESC를 살리기 위해 포커스가 뷰포트로 새면 매 틱 `SetKeyboardFocus()`로 되가져온다. 이때 "로비 메뉴 트리 밖에 뜬 정당한 포커스 위젯"은 예외로 둬야 하는데, 그 예외 목록(`bExternalFocusWidgetOpen`)은 자신이 아는 모달만 본다:

- `ActiveSettingsWidget`
- 로비 메뉴 자신의 `ActiveConfirmDialog`
- `WBP_LoadoutPreparation->HasActiveConfirmDialog()`

칩 스테이션은 개인정비 콘텐츠 트리 안에 중첩돼 있고 **자체 `ActiveConfirmDialog`를 띄우는데**, `LSLoadoutPreparationWidget::HasActiveConfirmDialog()`가 자기 다이얼로그만 보고 이 중첩 다이얼로그는 보고하지 않았다. 그래서 로비 메뉴는 매 틱 `bExternalFocusWidgetOpen == false`로 판단하고, 다이얼로그가 포커스를 받자마자(`OnFocusReceived cause=SetDirectly`) 다음 틱에 로비가 회수한다(`OnFocusLost cause=SetDirectly`). 포커스를 잃은 상태라 첫 클릭은 포커스 복구에 소비되고, 두 번째 클릭이 버튼에 도달한다.

진단은 다이얼로그에 `NativeOnFocusReceived`/`NativeOnFocusLost` 로그를 임시로 달아, **받자마자 SetDirectly로 잃는** 패턴을 확인해 특정했다.

## 수정

1. `ULSLobbyMenuWidget::NativeTick` 가드를 `포커스 보유 검사 먼저 → 없을 때만 외부 모달 판정` 순으로 유지한다.
2. 1단 배타 패널 전환 뒤에는 루트 `HasActivePanelModal()`이 칩 스테이션·보급소·로비 인벤토리의 모달 접근자를 **직접 종합**한다. 중간 `ULSLoadoutPreparationWidget` 위임과 재귀 위젯 탐색은 제거한다.
3. 구 위임 사슬에는 인벤토리의 "가득 찼습니다" 알림이 누락돼 있었다. `ULSInventoryWidget::HasActiveNotificationDialog()`를 합류시켜 가방 패널의 Shift 빠른이동 실패 알림도 첫 클릭에 반응하게 한다.
4. 패널을 바꿀 때는 `ClosePanelModal()`이 떠나는 패널의 별도 레이어 다이얼로그를 닫아 고아 모달을 남기지 않는다.

## 재발 방지 체크

- **로비 메뉴 트리 밖에 새 포커스 위젯/모달을 띄우면, 반드시 `ULSLobbyMenuWidget::HasActivePanelModal()` 또는 루트의 직접 예외에 합류시킨다.** 안 하면 그 위젯이 매 틱 포커스를 뺏겨 첫 입력이 씹힌다.
- 새 패널이 자체 모달을 띄우면 `HasActivePanelModal()` 보고와 `ClosePanelModal()` 정리를 한 쌍으로 추가한다. 체크리스트의 단일 출처는 [../Systems/LobbyScreenStructure.md](../Systems/LobbyScreenStructure.md)다.
- "새로 뜬 위젯의 첫 클릭만 씹힌다"는 증상은 포인터 캡처보다 **포커스 도둑질**을 먼저 의심한다. 다이얼로그에 `NativeOnFocusReceived`/`NativeOnFocusLost`(+`cause`) 로그를 달아 받자마자 잃는지 본다.

## 관련

- 로비 UI 레이어·모달 표시 규칙: [../Systems/UILayerStructure.md](../Systems/UILayerStructure.md)
- 칩 스테이션 다이얼로그(용량 차단) 소유: [../Systems/ChipSystem.md](../Systems/ChipSystem.md)
