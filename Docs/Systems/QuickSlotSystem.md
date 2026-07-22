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
- **소모품만** 등록을 허용한다. RowName에 `"Consumable"`이 포함된 아이템을 소모품으로 판정한다(`SetQuickSlot` 내부). 그 외 아이템 드롭은 등록되지 않고 거부 로그만 남긴다.
- 같은 소모품을 다른 칸에 등록하면 이전 칸을 비운다(스킬 로드아웃과 동일한 이동 시맨틱).
- 칸 수(6)는 `LSInventorySlotUtils::QuickSlotCount`가 단일 출처다.

## 위젯

- [`ULSQuickSlotWidget`](../../Source/LostSignal/UI/QuickSlot/LSQuickSlotWidget.h) — 개별 칸. 드롭 타겟(`NativeOnDrop`)이며 우클릭으로 등록 해제한다. `Refresh`에서 아이콘(공용 `LSInventorySlotUtils::LoadItemIconTexture`)과 인벤토리 합산 개수를 그린다.
- [`ULSQuickSlotBarWidget`](../../Source/LostSignal/UI/QuickSlot/LSQuickSlotBarWidget.h) — 고정 6칸(`QuickSlot1~6` `BindWidget`) 컨테이너. 로비/레이드 HUD 양쪽에서 재사용한다. 생성 시 스스로 `PlayerController`에 등록하고 `OnQuickSlotsChanged`를 구독한다.

개수 합산은 툴팁 "현재 아이템 개수"와 동일 소스를 쓴다: 레이드 활성이면 `ULSRaidInventoryComponent`의 세션(일반+Safe), 아니면 `ULSSaveSubsystem`의 세이브(일반+Safe). 합산 함수는 `LSCraftingUtils::CountItem`을 재사용한다.

## 갱신 경로 (단일 funnel)

- **등록 변경:** `SetQuickSlot`/`ClearQuickSlot` → `OnQuickSlotsChanged` → 바가 `RefreshAll`.
- **개수 변경:** 모든 인벤토리 변경은 [`ALSPlayerControllerBase::RefreshAllInventoryUI`](../../Source/LostSignal/Core/LSPlayerControllerBase.h) funnel을 통과하며, 그 말미에서 **등록된 모든 퀵슬롯 바**의 `RefreshAll`을 호출한다. 낙관적 부분 갱신은 하지 않는다(인벤토리 UI 규칙과 동일, [InventoryLogic.md](InventoryLogic.md)).
- **다중 바 동기화:** 인벤토리 패널의 바와 HUD의 바가 동시에 떠 있을 수 있다. PlayerController는 단일 바가 아니라 `RegisteredQuickSlotBars`(약참조 배열)로 모두 들고, funnel과 `OnQuickSlotsChanged` 양쪽에서 전부 갱신해 두 바가 항상 같은 값을 보이게 한다.

## 사용(소비) — 키 입력

`ALSPlayerCharacter`의 `Item1~6Action`이 퀵슬롯 0~5에 대응하며, 핸들러 `OnItemN`이 `TryUseQuickSlot(N-1)`을 호출한다. 폰은 레이드에만 존재하므로 사용은 레이드 전용이다.

- **클라 진입**(`TryUseQuickSlot`): 퀵슬롯 RowName 조회 → `ULSGameDataSubsystem::FindConsumableRow` → 클라 미러 세션 인벤토리 보유 수량 확인 → 시전 시작.
- **시전**(클라 구동): `Item_Cast_Time`>0이면 HUD 시전 게이지(`ALSPlayerControllerBase::ShowCastGauge`, 스킬 캐스팅 게이지 재사용) + 타이머. 시전 중 `Item_Can_Move=false`인데 이동 입력이 들어오면 취소(`Move` 훅). 완료 후 `Item_Trigger_Delay`만큼 더 대기(이 구간은 취소 불가).
- **확정**(서버 권한, `UseConsumableAuthoritative`): `HasAuthority`면 직접, 아니면 `ServerUseConsumable` RPC. 서버에서 수량 재검증 → `ULSCharacterCombatComponent::ApplyConsumableEffects`(대상=Self) → `ULSRaidInventoryComponent::ConsumeSessionItem(RowName, 1)`(일반 인벤토리만 차감) → `SyncRaidInventoryToClient`로 미러+갱신.

### 투척(Throwable) — 범위 인디케이터 조준

`Item_Use_Type==Throwable`이면 즉시 사용 대신 **조준 모드**로 들어가 지면에 범위를 표시한다. 스킬 인디케이터 [`ULSSkillPreviewComponent`](../../Source/LostSignal/Skills/Preview/LSSkillPreviewComponent.h)를 그대로 재사용한다(캐릭터에 이미 부착).

- **조준 시작**(`BeginThrowAim`): 소모품 Row 도형을 `FLSSkillAreaPreviewSpec`으로 변환(`Sphere→Circle 360°`, `Cone→Circle+Degrees`, `Box→Box`) 후 `BeginAreaPreview`.
- **매 틱**(`UpdateThrowAim`, Tick): 마우스 월드 지점을 `Item_Cast_Range`로 clamp해 착탄 지점/인디케이터를 갱신.
- **확정/취소 입력(스킬과 동일)**: 좌클릭(Attack)=`ConfirmThrowAim`, 취소키(SkillCancel)·우클릭(Skill1)·대시=`CancelThrowAim`, 아이템 키 재입력=취소.
- **확정 시**: 착탄 지점을 확정하고 시전(`Item_Cast_Time`)→발동 지연(`Item_Trigger_Delay`)을 거쳐 `ServerUseThrownConsumable(RowName, 착탄지점)`.
- **서버 처리**(`UseThrownConsumableAuthoritative`): 착탄 지점 기준 도형 안의 적(`ALSEnemyCharacter`)을 수집(`CollectThrowTargets`, 2D 판정) → `ULSCharacterCombatComponent::ApplyConsumableEffectsInArea`(Self 효과는 소유자 1회, Enemy 효과는 대상별) → 수량 차감 → 미러.

효과 적용 규칙·미지원 조합은 [ConsumableSystem.md](ConsumableSystem.md)가 소유한다.

## 에디터/아트 매핑

- `WBP_QuickSlot`(`ULSQuickSlotWidget` 상속): `IconImage`(UImage), `AmountText`(UTextBlock) 바인딩. 루트는 드롭을 받도록 Visible.
- `WBP_QuickSlotBar`(`ULSQuickSlotBarWidget` 상속): `QuickSlot1~QuickSlot6` 배치.
- 바는 두 곳에 둘 수 있고 서로 동기화된다: (1) **`WBP_Inventory` 안에 자식으로 배치**해 인벤토리를 열면 함께 표시·세팅(로비·인게임 공통, 둘 다 같은 `ULSInventoryWidget`), (2) **`WBP_PlayerHUD`에 배치**해 전투 중 상시 표시. 각 바는 생성 시 스스로 등록되며 개수/등록 변경 시 모두 함께 갱신된다.
- `Item1~6Action`(UInputAction)을 `IMC_Default`에 매핑한다(아트/기획).

## 범위 밖 (후속)

- 투척 발사체 비주얼/궤적(현재는 즉시 착탄 판정), 투척 Cone 판정을 착탄 지점 기준으로 두는 근사(인디케이터와 일치), 진영/아군(Friendly/All) 대상.
- 쿨다운, 드래그로 끌어내 등록 해제.
