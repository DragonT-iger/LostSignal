# 퀵슬롯 시스템 구조

이 문서는 퀵슬롯(소모품 등록·개수 표시)의 단일 출처다. 클래스/필드 구조는 C++ 헤더가 소유하고, 여기엔 의도와 경로만 적는다.

## 핵심 개념

퀵슬롯은 **소모품 RowName 참조를 담는 6칸**이다. 아이템 스택을 옮겨 담지 않는다.
- **등록:** 인벤토리 슬롯에서 퀵슬롯 칸으로 드래그앤드랍하면 그 소모품이 그 칸에 등록된다.
- **개수 표시:** 각 칸은 등록된 소모품(같은 RowName)이 **현재 인벤토리에 몇 개 있는지 실시간 합산**해 표시한다.

칸이 스택을 갖지 않으므로 서버 권한·RPC가 필요 없다. 순수 클라이언트 UI + 클라이언트 세이브다.

## 저장 (영구)

등록 6칸은 [`ULSSaveGame::QuickSlots`](../../Source/LostSignal/Session/LSSaveGame.h)(`TArray<FName>`, `NAME_None`=빈 칸)에 저장돼 로비 편집이 레이드·재시작까지 유지된다.
- 편집 API는 [`ULSSaveSubsystem`](../../Source/LostSignal/Session/LSSaveSubsystem.h): `GetQuickSlots` / `SetQuickSlot` / `ClearQuickSlot`. `SetQuickSlot`은 성공 시 저장 후 `OnQuickSlotsChanged`를 발행한다.
- **소모품만(`Item_Type` 4~9)** 등록을 허용한다(검증은 `LSInventorySlotUtils::ResolveItemTradeInfo`). 그 외 아이템 드롭은 등록되지 않고 거부 로그만 남긴다.
- 같은 소모품을 다른 칸에 등록하면 이전 칸을 비운다(스킬 로드아웃과 동일한 이동 시맨틱).
- 칸 수(6)는 `LSInventorySlotUtils::QuickSlotCount`가 단일 출처다.

## 위젯

- [`ULSQuickSlotWidget`](../../Source/LostSignal/UI/QuickSlot/LSQuickSlotWidget.h) — 개별 칸. 드롭 타겟(`NativeOnDrop`)이며 우클릭으로 등록 해제한다. `Refresh`에서 아이콘(공용 `LSInventorySlotUtils::LoadItemIconTexture`)과 인벤토리 합산 개수를 그린다.
- [`ULSQuickSlotBarWidget`](../../Source/LostSignal/UI/QuickSlot/LSQuickSlotBarWidget.h) — 고정 6칸(`QuickSlot1~6` `BindWidget`) 컨테이너. 로비/레이드 HUD 양쪽에서 재사용한다. 생성 시 스스로 `PlayerController`에 등록하고 `OnQuickSlotsChanged`를 구독한다.

개수 합산은 툴팁 "현재 아이템 개수"와 동일 소스를 쓴다: 레이드 활성이면 `ULSRaidInventoryComponent`의 세션(일반+Safe), 아니면 `ULSSaveSubsystem`의 세이브(일반+Safe). 합산 함수는 `LSCraftingUtils::CountItem`을 재사용한다.

## 갱신 경로 (단일 funnel)

- **등록 변경:** `SetQuickSlot`/`ClearQuickSlot` → `OnQuickSlotsChanged` → 바가 `RefreshAll`.
- **개수 변경:** 모든 인벤토리 변경은 [`ALSPlayerControllerBase::RefreshAllInventoryUI`](../../Source/LostSignal/Core/LSPlayerControllerBase.h) funnel을 통과하며, 그 말미에서 활성 퀵슬롯 바의 `RefreshAll`을 호출한다. 낙관적 부분 갱신은 하지 않는다(인벤토리 UI 규칙과 동일, [InventoryLogic.md](InventoryLogic.md)).

## 에디터/아트 매핑

- `WBP_QuickSlot`(`ULSQuickSlotWidget` 상속): `IconImage`(UImage), `AmountText`(UTextBlock) 바인딩. 루트는 드롭을 받도록 Visible.
- `WBP_QuickSlotBar`(`ULSQuickSlotBarWidget` 상속): `QuickSlot1~QuickSlot6` 배치.
- 바를 `WBP_PlayerHUD`(레이드)와 로비 화면에 배치한다.

## 범위 밖 (후속)

- 키 입력으로 소모품 사용/소비, 수량 차감, 서버 RPC. 소비 시 효과 적용은 [`ULSCharacterCombatComponent::ApplyConsumableEffects`](../../Source/LostSignal/Combat/LSCharacterCombatComponent.h)로 연결한다 — 소모품 데이터·효과 경로는 [ConsumableSystem.md](ConsumableSystem.md)가 소유.
- 쿨다운/사용 딜레이 표시, 드래그로 끌어내 등록 해제.
