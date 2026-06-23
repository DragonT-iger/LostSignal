# UI 레이어 / 공유 블러 구조

## 목적

이 문서는 뷰포트 최상위 UMG 위젯의 **Z-order 레이어 규칙**과, 모달 패널 뒤에 깔리는 **공유 풀스크린 블러**의 표시 규칙을 정리한다.

각 패널의 슬롯/조작 로직은 [InventoryLogic.md](InventoryLogic.md)가 소유한다. 이 문서는 "여러 패널이 동시에 떴을 때 누가 위에 그려지고, 블러는 언제 깔리는가"만 다룬다.

## 배경 (왜 필요한가)

인벤토리·창고·칩스테이션·루트드랍은 **서로 독립된 최상위 위젯**이고, 루팅 중에는 인벤토리와 루트드랍이 **동시에** 떠서 서로 드래그/Shift-이동한다. 과거에는 풀스크린 블러가 루트드랍 WBP 안에 들어 있어서, 같은 Z(0)로 올라간 인벤토리를 루트드랍의 블러가 덮어 인벤토리가 뿌옇게 보였다.

해결은 두 축으로 나눠 본다.

- **순서(깊이):** 누가 위에 그려지는가 → Z-order로만 결정.
- **표시:** 블러를 언제 켜고 끄는가 → 패널 표시 상태에 따라 토글.

## Z-order 레이어

레이어 값의 단일 출처는 `Source/LostSignal/UI/LSUILayer.h`다. 문서엔 수치를 복붙하지 않으며, 순서 관계만 적는다.

```text
HUD  <  BackgroundBlur  <  ModalPanel  <  ProtocolDebug  <  Tooltip
```

- `HUD`: 상시 게임플레이 HUD. 블러보다 아래라 패널이 열리면 HUD도 함께 블러된다.
- `BackgroundBlur`: 공유 풀스크린 블러. 모든 모달 패널이 공유한다.
- `ModalPanel`: 인벤토리/창고/칩스테이션/루트드랍 본체. 전부 같은 레이어라 블러 위에 선명하게 그려진다.
- `ProtocolDebug`: 시연용 디버그 패널.
- `Tooltip`: 커서를 따라다니는 호버 툴팁(프로토콜 등). 모든 패널/디버그 위에 떠야 하므로 최상단. 입력을 가로채지 않도록 `HitTestInvisible`로 띄운다.

새 최상위 위젯을 `AddToViewport` 할 때는 리터럴 Z 대신 `LSUILayer`의 값을 쓴다.

## 공유 블러 표시 규칙

- 위젯: `ULSBackgroundBlurWidget`(C++) → `WBP_BackgroundBlur`(풀스크린 `BackgroundBlur`, 이름 `BlurEffect`로 `BindWidget` 강제). 클래스 구조는 헤더가 단일 출처다.
- 생성: 로컬 컨트롤러 `BeginPlay`에서 1회 생성해 `BackgroundBlur` 레이어에 상주시키고, 시작 상태는 `Collapsed`.
- 토글: `ALSPlayerControllerBase::UpdateBackgroundBlurVisibility()`가 **매번 현재 상태를 재계산**한다. 인벤토리/창고/칩스테이션/루트드랍 중 하나라도 표시 중이면 `HitTestInvisible`로 켜고(입력은 위 패널이 받음), 전부 닫히면 `Collapsed`로 끈다.
- 호출 지점: 각 패널 show/hide 직후. 컨트롤러 소유 패널은 `Show/Hide*Local`에서, 인벤토리는 Pawn(`ALSPlayerCharacter`)의 show/hide에서 컨트롤러로 호출한다.

재계산 방식이라 중복 show / 이미 닫힌 hide에도 상태가 어긋나지 않는다(단순 증감 카운터의 drift 회피).

## 주의점

- 블러는 공유 블러 WBP에만 둔다. 개별 패널 WBP에 풀스크린 블러를 다시 넣으면 이중 블러가 되므로 금지.
- HUD를 패널 위에서 또렷하게 유지하고 싶으면 HUD 레이어를 `BackgroundBlur`보다 큰 값으로 올린다(현재는 의도적으로 함께 블러).
- 블러는 컨트롤러 소유라 Pawn 교체에도 유지된다. 인벤토리 위젯만 Pawn 소유다.
