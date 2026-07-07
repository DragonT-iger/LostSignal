# 로비 포커스 회수가 외부 모달의 첫 클릭을 먹는 문제

## 증상

로비(개인정비)에서 뜨는 확인/알림 다이얼로그 — 예: 칩 스테이션의 "인벤토리 용량이 부족합니다" 경고 — 의 **확인/취소 버튼을 두 번 눌러야 닫힌다.** 첫 클릭은 씹히고 두 번째 클릭에야 버튼이 반응한다. 타이틀/세팅 화면에서 같은 `WBP_ConfirmDialog`를 버튼 클릭으로 열면 한 번에 닫히는데, 로비의 슬롯 제스처(더블클릭/Shift/드래그드롭)로 뜨는 다이얼로그만 재현된다.

## 원인

포인터 캡처나 next-tick 생성 타이밍 문제가 **아니다**(그 방향으로 고쳐도 안 고쳐진다). 진짜 원인은 **로비 메뉴의 매 틱 포커스 회수**다.

`ULSLobbyMenuWidget::NativeTick`은 TAB/ESC를 살리기 위해 포커스가 뷰포트로 새면 매 틱 `SetKeyboardFocus()`로 되가져온다. 이때 "로비 메뉴 트리 밖에 뜬 정당한 포커스 위젯"은 예외로 둬야 하는데, 그 예외 목록(`bExternalFocusWidgetOpen`)은 자신이 아는 모달만 본다:

- `ActiveSettingsWidget`
- 로비 메뉴 자신의 `ActiveConfirmDialog`
- `WBP_LoadoutPreparation->HasActiveConfirmDialog()`

칩 스테이션은 개인정비 콘텐츠 트리 안에 중첩돼 있고 **자체 `ActiveConfirmDialog`를 띄우는데**, `LSLoadoutPreparationWidget::HasActiveConfirmDialog()`가 자기 다이얼로그만 보고 이 중첩 다이얼로그는 보고하지 않았다. 그래서 로비 메뉴는 매 틱 `bExternalFocusWidgetOpen == false`로 판단하고, 다이얼로그가 포커스를 받자마자(`OnFocusReceived cause=SetDirectly`) 다음 틱에 로비가 회수한다(`OnFocusLost cause=SetDirectly`). 포커스를 잃은 상태라 첫 클릭은 포커스 복구에 소비되고, 두 번째 클릭이 버튼에 도달한다.

진단은 다이얼로그에 `NativeOnFocusReceived`/`NativeOnFocusLost` 로그를 임시로 달아, **받자마자 SetDirectly로 잃는** 패턴을 확인해 특정했다.

## 수정

1. `ULSChipStationWidget::HasActiveConfirmDialog()` public 접근자 추가 — `ActiveConfirmDialog && IsInViewport()`.
2. `ULSLoadoutPreparationWidget::HasActiveConfirmDialog()`가 자기 다이얼로그뿐 아니라 **콘텐츠에 중첩된 칩 스테이션 다이얼로그도 함께 보고**하도록 확장(`FindLoadoutChipStationWidget`으로 활성 콘텐츠에서 칩 스테이션을 찾아 위임). 이걸로 로비 메뉴 가드의 기존 예외 계약에 칩 다이얼로그가 합류한다.
3. `ULSLobbyMenuWidget::NativeTick` 가드를 `포커스 보유 검사 먼저 → 없을 때만 외부 모달 판정` 순으로 재배치. 새로 추가된 위젯 트리 탐색이 매 틱 돌지 않게 하는 최적화(포커스를 이미 쥐고 있으면 회수가 불필요하므로 판정 생략).

## 재발 방지 체크

- **로비 메뉴 트리 밖에 새 포커스 위젯/모달을 띄우면, 반드시 `ULSLobbyMenuWidget::NativeTick`의 `bExternalFocusWidgetOpen` 예외에 합류시킨다.** 안 하면 그 위젯이 매 틱 포커스를 뺏겨 첫 입력이 씹힌다. (코드 주석에도 이 계약이 명시돼 있다.)
- 중첩 위젯(개인정비 콘텐츠 안의 칩 스테이션 등)이 자체 모달을 띄우면, 그 부모(`LoadoutPreparation`)의 `HasActiveConfirmDialog()`가 중첩 모달까지 보고하는지 확인한다.
- "새로 뜬 위젯의 첫 클릭만 씹힌다"는 증상은 포인터 캡처보다 **포커스 도둑질**을 먼저 의심한다. 다이얼로그에 `NativeOnFocusReceived`/`NativeOnFocusLost`(+`cause`) 로그를 달아 받자마자 잃는지 본다.

## 관련

- 로비 UI 레이어·모달 표시 규칙: [../Systems/UILayerStructure.md](../Systems/UILayerStructure.md)
- 칩 스테이션 다이얼로그(용량 차단) 소유: [../Systems/ChipSystem.md](../Systems/ChipSystem.md)
