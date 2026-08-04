# 퀵슬롯 시스템 구조

이 문서는 퀵슬롯(소모품 등록·개수 표시)의 단일 출처다. 클래스/필드 구조는 C++ 헤더가 소유하고, 여기엔 의도와 경로만 적는다.

## 핵심 개념

퀵슬롯은 **소모품 RowName 참조를 담는 6칸**이다. 아이템 스택을 옮겨 담지 않는다.
- **등록:** 인벤토리 슬롯에서 퀵슬롯 칸으로 드래그앤드랍하면 그 소모품이 그 칸에 등록된다.
- **개수 표시:** 각 칸은 등록된 소모품(같은 RowName)이 **현재 인벤토리에 몇 개 있는지 실시간 합산**해 표시한다.

퀵슬롯은 참조만 저장하므로 인벤토리에 없는 아이템을 등록하면 개수가 0으로 표시된다. 그래서 **루팅 박스에서 바로 퀵슬롯으로 드래그하면 먼저 인벤토리로 루팅한 뒤 등록**한다(기존 자동-루팅 경로 `ULSLootDropWidget::TransferLootSlotToInventory` 재사용). 인벤토리가 가득 차 루팅이 실패하면 등록하지 않고 아이템은 박스에 그대로 둔다(경고 로그).

칸이 스택을 갖지 않으므로 서버 권한·RPC가 필요 없다. 순수 클라이언트 UI + 클라이언트 세이브다.

## 저장 (영구)

등록 6칸은 [`ULSSaveGame::QuickSlots`](../../Source/LostSignal/Session/LSSaveGame.h)(`TArray<FName>`, `NAME_None`=빈 칸)에 저장돼 로비 편집이 레이드·재시작까지 유지된다.
- 편집 API는 [`ULSSaveSubsystem`](../../Source/LostSignal/Session/LSSaveSubsystem.h): `GetQuickSlots` / `SetQuickSlot` / `ClearQuickSlot`. `SetQuickSlot`은 성공 시 저장 후 `OnQuickSlotsChanged`를 발행한다.
- **소모품만** 등록을 허용한다. RowName에 `"Consumable"`이 포함된 아이템을 소모품으로 판정한다(`SetQuickSlot` 내부). 그 외 아이템 드롭은 등록되지 않고 거부 로그만 남긴다.
- 같은 소모품을 다른 칸에 등록하면 이전 칸을 비운다(스킬 로드아웃과 동일한 이동 시맨틱).
- 칸 수(6)는 `LSInventorySlotUtils::QuickSlotCount`가 단일 출처다.

## 위젯

- [`ULSQuickSlotWidget`](../../Source/LostSignal/UI/QuickSlot/LSQuickSlotWidget.h) — 개별 칸. 인벤토리 바에서만 드롭 타겟(`NativeOnDrop`)이며 **우클릭으로 등록 해제**한다(더블클릭 해제는 두지 않음). 마우스 편집은 바가 부여한 인벤토리 상호작용 컨텍스트와 `IsInventoryUIOpen`을 모두 만족해야 동작한다. 아이콘·합산 개수·등급/빈 슬롯 배경·호버 연출은 내부의 표시 전용 `ULSItemSlotWidget`에 위임한다. 내부 슬롯은 `SetDisplayOnlySlotContext`로 이동 컨텍스트를 제거하고 `HitTestInvisible`로 고정하므로 드래그·드롭과 우클릭 입력은 바깥 퀵슬롯이 소유한다. 퀵슬롯은 외부 호버 상태만 내부 슬롯에 전달하므로 아이템 슬롯의 배경색 우선순위와 연출 값이 단일 출처다. HUD 슬롯은 `HitTestInvisible`이라 인벤토리가 함께 열려 있어도 호버·우클릭·드롭에 반응하지 않는다. **바인딩 키 표시**(`BindKeyText`)는 스킬 슬롯과 동일하게 폰의 `Item1~6Action`을 `IMC_Default` 매핑에서 조회해 실제 키의 표시 이름을 넣는다(키보드 우선, 리바인딩 자동 반영). 소비가 레이드 폰 전용이라 매핑도 폰에서 조회하며, 폰이 없는 로비 인벤토리에선 빈 텍스트로 둔다.
- [`ULSQuickSlotBarWidget`](../../Source/LostSignal/UI/QuickSlot/LSQuickSlotBarWidget.h) — 고정 6칸(`QuickSlot1~6` `BindWidget`) 컨테이너. 로비/레이드 HUD 양쪽에서 재사용하며 기본값은 표시 전용이다. `ULSInventoryWidget`만 자신의 자식 바에 `SetInventoryInteractionEnabled(true)`를 설정한다. 생성 시 스스로 `PlayerController`에 등록하고 `OnQuickSlotsChanged`·`OnChipLoadoutChanged`를 구독한다.

개수 합산은 툴팁 "현재 아이템 개수"와 동일 소스를 쓴다: 레이드 활성이면 `ULSRaidInventoryComponent`의 세션(일반+Safe), 아니면 `ULSSaveSubsystem`의 세이브(일반+Safe). 합산 함수는 `LSCraftingUtils::CountItem`을 재사용한다.

## 적재 프로토콜 해금 (표시·사용 게이트)

퀵슬롯 사용 가능 칸 수는 **적재 프로토콜(`ELSProtocolType::Carrying`) 레벨**로 해금된다. 인벤토리 슬롯 용량과 **동일 메커니즘**이다: `DT_Protocol`의 `Protocol_Enable_Name = "Quick"`(`UI_Slot`) 행들을 `ULSGameDataSubsystem::GetVisibleProtocolEnableValueSum`으로 합산한 값이 해금 칸 수다. 레벨/칸 수는 **DT가 단일 출처**(수치는 [ChipSystem.md](ChipSystem.md)의 적재 프로토콜 정의와 `DT_Protocol`가 소유) — 코드엔 하드코딩하지 않는다.

- **단일 출처 함수:** [`ALSPlayerControllerBase::GetUnlockedQuickSlotCount`](../../Source/LostSignal/Core/LSPlayerControllerBase.h) — UI 표시(바 가시성)와 사용 게이트 **양쪽이 이 값 하나**를 쓴다(항상 일치). 내부 분기:
  - **디버그 패널 오버라이드 우선:** 프로토콜 디버그 패널(Insert 토글)이 떠 있고 적재 오버라이드가 설정돼 있으면 그 레벨로 계산(`ULSSaveSubsystem::GetUnlockedQuickSlotCountForCarryingLevel`, current==previous). 스킬 바/칩스테이션의 디버그 게이트(`HasProtocolTestLevel`)와 동일.
  - **평상시:** 세이브의 신호-활성 칩 집계 기반 [`ULSSaveSubsystem::GetUnlockedQuickSlotCount`](../../Source/LostSignal/Session/LSSaveSubsystem.h)(`GetCarryingProtocolSlotBonus("Quick")` → `QuickSlotCount` 클램프). 컨트롤러가 없는 경로는 이 세이브 값으로 폴백.
- **사용 게이트:** `TryUseQuickSlot(index)` 상단에서 `index >= 해금 칸 수`면 거부(인덱스 0~5 = 1~6번 칸과 1:1). 등록(드래그앤드랍)은 잠긴 칸에도 허용 — 해금 시 바로 쓰인다.
- **표시 게이트(바별로 다름):** 인벤토리 상호작용 컨텍스트와 `ULSQuickSlotBarWidget::bHideLockedSlots`(`EditAnywhere`)로 분기한다.
  - **HUD 바**(기본 `true`): 잠긴 칸을 `Collapsed`로 접어 표시 영역이 해금 수만큼(예: 3→6칸) 리플로우.
  - **인벤토리 바**(`SetInventoryInteractionEnabled(true)`): `bHideLockedSlots` 값과 레벨에 무관하게 6칸을 항상 표시하고 잠금 표시도 하지 않는다. 프로토콜 레벨이 떨어져도 등록 내용과 칸 표시는 유지된다. (키 입력 사용 자체는 여전히 게이트됨.)
- **실시간 갱신:** 칩 장착/해제·신호 게이지 변경은 `OnChipLoadoutChanged`로 브로드캐스트되어 바의 `RefreshAll`(→ `ApplyProtocolVisibility`)을 태운다. 레이드 중 신호 하락으로 적재 레벨이 떨어지면 `IsProtocolUnlockVisible`의 current/previous·`Protocol_Protected_Level`(정보 유지) 규칙대로 표시가 갱신된다. 디버그 패널에서 적재 레벨을 +/-로 조정하면 `RefreshProtocolTestTargets` → `RefreshRegisteredQuickSlotBars`로 등록된 모든 바가 즉시 다시 그려진다.

> **아트 매핑:** HUD 인스턴스는 `bHideLockedSlots=true`를 유지한다. 인벤토리 인스턴스는 C++ 상호작용 컨텍스트가 항상 6칸 표시를 강제하므로 별도 설정이 필요 없다.
> **기획 데이터:** `DT_Protocol`에 적재 행 추가 필요 — 예) 레벨 2에서 `Quick` +3, 레벨 5에서 +3(합산 0/3/6). 값·`Protocol_Protected_Level`은 기획 조정.

## 갱신 경로 (단일 funnel)

- **등록 변경:** `SetQuickSlot`/`ClearQuickSlot` → `OnQuickSlotsChanged` → 바가 `RefreshAll`.
- **개수 변경:** 모든 인벤토리 변경은 [`ALSPlayerControllerBase::RefreshAllInventoryUI`](../../Source/LostSignal/Core/LSPlayerControllerBase.h) funnel을 통과하며, 그 말미에서 **등록된 모든 퀵슬롯 바**의 `RefreshAll`을 호출한다. 낙관적 부분 갱신은 하지 않는다(인벤토리 UI 규칙과 동일, [InventoryLogic.md](InventoryLogic.md)).
- **다중 바 동기화:** 인벤토리 패널의 바와 HUD의 바가 동시에 떠 있을 수 있다. PlayerController는 단일 바가 아니라 `RegisteredQuickSlotBars`(약참조 배열)로 모두 들고, funnel과 `OnQuickSlotsChanged` 양쪽에서 전부 갱신해 두 바가 항상 같은 값을 보이게 한다.

## 사용(소비) — 키 입력

`ALSPlayerCharacter`의 `Item1~6Action`이 퀵슬롯 0~5에 대응하며, 핸들러 `OnItemN`이 `TryUseQuickSlot(N-1)`을 호출한다. 폰은 레이드에만 존재하므로 사용은 레이드 전용이다.

- **클라 진입**(`TryUseQuickSlot`): 퀵슬롯 RowName 조회 → `ULSGameDataSubsystem::FindConsumableRow` → 클라 미러 세션 인벤토리 보유 수량 확인 → 로컬 게이지/조준 표시 시작과 함께 고유 `UseID`로 `ServerBeginConsumableUse` 요청. 클라 검사는 빠른 피드백용이며 판정 권한은 없다.
- **서버 트랜잭션**: 서버는 동시 사용이 없는지와 Row·사용 타입·효과 사전·레이드 상태·실제 보유 수량을 검증하고 자체 시전 타이머를 시작한다. 로컬 타이머는 HUD만 담당하며, 이동 취소는 시전 구간에만 `ServerCancelConsumableUse(UseID)`로 전달한다. 서버가 차감한 뒤의 발동 지연 구간은 취소할 수 없고, `ClientEndConsumableUse`를 받을 때까지 새 소모품 사용을 막는다.
- **차감(시전 완료 시점, 모든 소모품 공통)**: 서버 시전 타이머가 끝나면 `ULSRaidInventoryComponent::ConsumeSessionItem(RowName, 1)`의 실제 반환값이 정확히 1인지 확인한다. 성공한 경우에만 `SyncRaidInventoryToClient`로 미러+갱신하고 발동 단계로 넘어간다. 차감 실패 트랜잭션에는 효과를 적용하지 않는다.
- **효과 적용(발동 지연 뒤)**: 차감에 성공한 서버 트랜잭션만 서버 발동 타이머 뒤 직접 사용은 `UseConsumableAuthoritative` → `ApplyConsumableEffects`, 투척은 `UseThrownConsumableAuthoritative` → `ApplyConsumableEffectsInArea`로 적용한다. 완료·거부·시전 취소 모두 같은 `UseID`를 종료해 오래된 응답이 새 사용 상태를 지우지 않게 한다.

### 투척(Throwable) — 범위 인디케이터 조준

`Item_Use_Type==Throwable`이면 즉시 사용 대신 **조준 모드**로 들어가 지면에 범위를 표시한다. 스킬 인디케이터 [`ULSSkillPreviewComponent`](../../Source/LostSignal/Skills/Preview/LSSkillPreviewComponent.h)를 그대로 재사용한다(캐릭터에 이미 부착).

- **조준 시작**(`BeginThrowAim`): 소모품 Row 도형을 `FLSSkillAreaPreviewSpec`으로 변환(`Sphere→Circle 360°`, `Cone→Circle+Degrees`, `Box→Box`) 후 `BeginAreaPreview`.
- **매 틱**(`UpdateThrowAim`, Tick): 마우스 월드 지점을 `Item_Cast_Range`로 clamp해 착탄 지점/인디케이터를 갱신.
- **확정/취소 입력(스킬과 동일)**: 좌클릭(Attack)=`ConfirmThrowAim`, 취소키(SkillCancel)·우클릭(Skill1)·대시=`CancelThrowAim`, 아이템 키 재입력=취소.
- **확정 시**: 착탄 지점을 포함해 서버 사용 트랜잭션을 시작하고 시전(`Item_Cast_Time`) → 발동 지연(`Item_Trigger_Delay`) 순으로 진행한다. 서버는 `Item_Use_Type==Throwable`을 재검증하고 전달된 착탄 지점을 서버 캐릭터 기준 `Item_Cast_Range` 안으로 다시 clamp한다.
- **수량 차감은 시전 완료 시점**(모든 소모품 공통, 아래 "사용(소비)" 절 참고) — 발동 지연 시점이 아니다.
- **효과 적용은 발동 지연 뒤**(`TriggerConsumableAuthoritative` → `UseThrownConsumableAuthoritative`): 서버가 확정한 착탄 지점 기준 도형 안의 적(`ALSEnemyCharacter`)을 수집(`CollectThrowTargets`, 2D 판정) → `ULSCharacterCombatComponent::ApplyConsumableEffectsInArea`(Self 효과는 소유자 1회, Enemy 효과는 대상별). 이 단계는 재차감하지 않는다.

효과 적용 규칙·미지원 조합은 [ConsumableSystem.md](ConsumableSystem.md)가 소유한다.

## 에디터/아트 매핑

- `WBP_QuickSlot`(`ULSQuickSlotWidget` 상속): 루트 `Overlay` 안에 `WBP_ItemSlot` 인스턴스를 `ItemSlot` 이름으로 배치하고 `BindKeyText`(UTextBlock, 소비 바인딩 키 표시)를 그 위에 둔다. 내부 `ItemSlot`과 장식 자식은 입력을 가로채지 않도록 C++가 `HitTestInvisible`로 강제한다. 바의 `ApplyProtocolVisibility`는 인벤토리 슬롯만 `Visible`, HUD 슬롯은 `HitTestInvisible`로 설정한다.
- `WBP_QuickSlotBar`(`ULSQuickSlotBarWidget` 상속): `QuickSlot1~QuickSlot6` 배치.
- 바는 두 곳에 둘 수 있고 서로 동기화된다: (1) **`WBP_Inventory` 안에서는** 배경 블러와 바를 함께 감싸는 PanelWidget 계열 부모를 `QuickSlotPanel`, 그 안의 바를 `QuickSlotBar`로 이름 맞춰 배치한다(둘 다 `ULSInventoryWidget`의 강제 `BindWidget`), (2) **`WBP_PlayerHUD`에 배치**해 전투 중 상시 표시. 각 바는 생성 시 스스로 등록되며 개수/등록 변경 시 모두 함께 갱신된다.
- **인벤토리 자식 패널 표시 규칙:** `QuickSlotPanel`이 배경 블러와 `QuickSlotBar`를 함께 소유하며 여는 경로에 따라 켜지고 꺼진다(`ULSInventoryWidget::SetQuickSlotPanelVisible`). 기본값은 **표시**(`NativeConstruct`에서 `SelfHitTestInvisible`). 폰이 있는 레이드는 `ALSPlayerCharacter::ShowInventoryWidgetInternal`이 **매 오픈 시** 값을 덮어써 **Tab 단독**(`ShowInventoryWidgetStandalone`)·**로비 창고 동반**(`ALSLobbyStorageActor`)은 표시, **레이드 루팅 박스**(`ALSLootBox`)만 부모째 숨김(`Collapsed`)으로 만든다. 폰 없이 열리는 로비 인벤토리는 이 폰 경로를 타지 않으므로 기본 표시로 남아 Tab/메뉴로 열면 블러와 바가 함께 보인다. HUD 상시 바는 이 규칙과 무관하다.
- `Item1~6Action`(UInputAction)을 `IMC_Default`에 매핑한다(아트/기획).

## 범위 밖 (후속)

- 투척 발사체 비주얼/궤적(현재는 즉시 착탄 판정), 투척 Cone 판정을 착탄 지점 기준으로 두는 근사(인디케이터와 일치), 진영/아군(Friendly/All) 대상.
- 쿨다운, 드래그로 끌어내 등록 해제.
