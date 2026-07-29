# 칩 시스템 (Chip System)

## 목적

칩 시스템 전반의 기획 의도와 **현재 구현 현황**, 그리고 남은 작업을 단계별로 정리한다.
아이템 데이터/드랍은 [LootDropDataTable.md](LootDropDataTable.md), 저장/네트워크는 [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md), 슬롯 UI는 [InventoryLogic.md](InventoryLogic.md)를 기준으로 본다.

기획서(`Lost _ signal 시스템 정리.xlsx` → `UI 칩 시스템` 시트)와 코드가 다른 부분은 **[기획/코드 차이]**, 기획만으로 확정 불가한 부분은 **[확인 필요]** 로 표시한다.

---

## 1. 기획 개요

칩은 인게임 플레이 UI(체력바·스태미나 등 전투지역 UI)를 **아이템화**한 형태이며, 동시에 **전투 능력치**도 보유한다.
칩은 **등급**에 따라 UI·능력치·메모리 할당량의 최대 수치가 달라진다.

기획서는 4개 블록으로 구성된다.

```
[1] 칩 아이템 / 등급            → 데이터 정의
[2] 칩 설정 인벤토리 UI         → 칩 전용 인벤토리 + 칩 오버랩(정보) UI
[3] 하드웨어 UI (칩 장착 UI)    → 슬롯 10칸 + 메모리 한도 + 신호 게이지
[4] 소프트웨어 UI (칩 능력 예시)→ 프로토콜 4종 + 코어 출력 + 신호 유실
```

### 핵심 개념

- **메모리(Memory)**: 칩마다 메모리 비용(`Item_MemoryCost`)이 있고, 장착 UI에는 `현재 사용량 / 최대치` 한도가 있다. 합이 한도를 넘으면 장착 불가.
- **신호 게이지(Signal Gauge)**: 0~100% 스크롤바. 게이지가 90.0% 이하로 내려갈 때부터 10% 단위로 **칩 번호 순서대로** 비활성화된다. 기본 장착 칩 스탯 합산은 활성·비활성 슬롯을 모두 포함하며, 프로토콜 합산은 비활성 슬롯을 제외한다. 최종 전투 스탯은 전체 장착 칩 합산에 비활성 칩 스탯의 50%를 가산한 값이다(단일 출처: 아래 "칩 전투 스탯 → 캐릭터 GAS 연동"). 칩 스테이션에 이 수치를 보여주던 전투 스탯 칸은 UI 개편으로 제거됐다. 칩 스테이션 프리뷰는 빈 슬롯을 포함한 이 고정 10칸 임계치를 그대로 사용한다.
  - **레이드 중 시간 감소**: 레이드(파밍 레벨) 진입 시 게이지를 100%로 채우고 **60초마다 슬롯 번호순으로 다음 장착 칩 하나를 비활성화**한다. 빈 슬롯은 시간을 소비하지 않고 해당 10% 구간을 건너뛰며, 마지막 장착 칩이 사라질 때 뒤의 빈 구간도 함께 건너뛰어 0%가 된다(예: 9개 장착 시 9분 뒤 0%). 장착 칩이 없으면 진입 즉시 0%로 만들고 타이머를 시작하지 않는다. 레이드 종료(로비 복귀) 시 다시 100%로 복원한다. 감소 주체는 레이드 동안만 존재하는 서버 권한 게임모드 `ALSFarmingGameMode`이며, `LSChipStats::TryResolveNextSignalGaugePercent`로 다음 게이지 단계를 계산한 뒤 `ULSSaveSubsystem::SetChipSignalGaugePercent`를 호출해 기존 GAS 재적용(`OnChipLoadoutChanged` → `ULSChipStatComponent::RefreshChipStats`)과 HUD 갱신 경로를 그대로 탄다. (싱글/Listen 서버 기준 — 데디 MO 동기화는 기존 칩 적용 패턴과 함께 후속.)
  - **로비=항상 100% 불변식 (중요):** 게이지 값은 감소할 때마다 세이브에 저장된다. 정상 종료는 `ALSFarmingGameMode::TravelToResultLevel`이 100%로 되돌리지만, **PIE 강제 종료·크래시로 레이드가 비정상 종료되면 낮은 값이 세이브에 남는다.** 이 상태로 로비에 오면 신호 유실로 적재(Carrying) 칩이 비활성→인벤토리 최대 슬롯 수가 실제 아이템 수보다 작아져, 초과 아이템이 화면에 안 보이는 overflow가 되고 칩 해제 용량 판정(`WouldUnequipChipDropInventoryItems`)이 계속 막히는 버그가 있었다. 이를 막기 위해 `ALSLobbyGameMode::BeginPlay`(→ `RestoreLobbySignalGauge`)가 **로비 진입 시 게이지를 100%로 되돌린다.** 단, 레이드 복구 대기(`IsRaidSaveActive()`) 중이면 재개용 값을 보존한다. 신호 게이지는 레이드 전용 메커니즘이며 로비 인벤토리 용량을 축소해선 안 된다.
- **프로토콜(Protocol)**: 칩이 보유한 4종 시너지 수치. 장착 칩들의 합산값을 그대로 프로토콜 레벨로 보고, `DT_Protocol`의 해금 row를 기준으로 단계와 표시 항목을 계산한다.
  - 생존(Survival): 체력·스태미나 UI 등 긴장감을 낮추는 편의 UI
  - 적재(Carrying): 인벤토리 개수·보호슬롯·퀵슬롯 등 용량 UI
  - 탐색(Navigation): 미니맵·탈출 위치 등 목표 지향 UI
  - 전투(Battle): 스킬 쿨타임·슬롯·데미지 표시 등 전투 효율 UI

### 등급

`Supply / Standard / Precision / Tuning / Prototype / Masterpiece`
> **[기획/코드 차이]** CSV(`DT_Chip.csv`)에는 Supply / Standard / Precision / Prototype 만 존재. Tuning / Masterpiece 행 미입력.

---

## 2. 현재 구현 현황

### ✅ 완료 (데이터 레이어 + 기본 보관/툴팁)

| 영역 | 항목 | 위치 |
|---|---|---|
| 데이터 구조 | `FLSChipRow` — 등급/메모리/프로토콜 4종/스탯 개수 | `Source/LostSignal/Data/LSChipRow.h` |
| 데이터 구조 | `FLSChipStatRow` — 등급 0~5별 스탯 Min/Max 범위 | `Source/LostSignal/Data/LSChipStatRow.h` |
| 데이터 테이블 | `DT_ChipRow`, `DT_ChipStat`, `DT_Protocol` (+ Sandbox CSV) | `Content/LostSignal/Data/DataTables/`, `Content/LostSignal/Sandbox/DT/` |
| 설정 참조 | `ChipTable`, `ChipStatTable` 소프트 참조 | `Source/LostSignal/Data/LSDropSettings.h` |
| 보관함 | 칩 탭 필터(`ELSStorageFilter::Chip`) + `WBP_ChipStorage` | `Source/LostSignal/UI/Storage/LSLobbyStorageWidget.*` |
| 아이콘 | 칩 아이콘 경로(`/Game/LostSignal/UI/Icons/Chips/`) | `Source/LostSignal/UI/Inventory/LSItemSlotWidget.cpp:465` |
| 툴팁 | 칩 툴팁: 이름/등급/설명/가격/메모리/프로토콜 수치/확정 전투 스탯 | `Source/LostSignal/UI/Inventory/LSItemTooltipWidget.cpp:197` |
| 칩 스테이션 목록(부분) | `ChipSlotWrapBox`에 저장 인벤토리/창고의 `Chip_` 아이템을 선택 프로토콜로 필터링한 뒤 가격 높은순으로 `ULSItemSlotWidget` 표시. 리스트는 현재 필터에 해당하는 총 칩 개수(미장착+장착)만큼 고정 크기이며 장착 칩 몫은 빈 칸으로 유지 | `Source/LostSignal/UI/ChipSystem/LSChipStationWidget.*` |
| 하드웨어 슬롯(부분) | `EquipmentSlot_0~9` 내부 `ItemSlot`에 칩 목록 드래그 장착. 장착 슬롯끼리 이동/교환 가능. 장착 칩을 `ChipSlotBorder` 빈 영역에 드롭하면 장착 해제(목적지는 출처 기억 규칙 — 아래 "장착 해제" 참고), 칩 리스트 아이템 위에 드롭하면 해당 저장 슬롯과 교환. 장착 칩의 스탯/프로토콜 합산값과 메모리 사용량(`현재/최대`)을 UI에 표시. 메모리 검증은 미연동 | `Source/LostSignal/UI/ChipSystem/LSChipEquipmentSlotWidget.*` |

### ⚠️ 부분 구현

- **칩 툴팁** (`PopulateChipTooltip`): 메모리 할당량, 칩 행의 **프로토콜 수치 4종**(`Chip_Protocol_*` 중 0이 아닌 것만 `"생존 프로토콜 +1"` 형식으로 전투 스탯보다 먼저 표시), 저장된 `ChipStats`의 **확정 전투 스탯 값**을 표시한다. 프로토콜 이름 텍스트의 단일 출처는 `LSProtocol::GetProtocolDisplayName`(`Source/LostSignal/Data/LSProtocolTypes.*`)이다.
- **칩 스테이션 목록** (`ULSChipStationWidget::RefreshChipSlots`): 저장 인벤토리/창고의 `Chip_` 아이템을 선택 프로토콜로 필터링한 뒤 가격 높은순으로 아이콘/수량/툴팁 슬롯에 표시한다. 슬롯 위젯 수는 **현재 필터에 해당하는 총 칩 개수(미장착 + 장착)로 고정**하고, 해당 프로토콜 수치가 1 이상인 장착 칩 몫만 빈 칸으로 뒤에 남긴다 — 장착은 칸을 비우고 해제는 필터에 맞을 때만 빈 칸을 채울 뿐이라 조작 중 위젯 추가/삭제가 없다. `SignalSlider`와 `SignalProgressBar`는 0~1 값으로 동기화한다. **슬라이더는 순수 프리뷰다** — 여는 시점의 저장 게이지(`ULSSaveGame::ChipSignalGaugePercent`, 로비에서는 100%)로 초기화하지만, 드래그해도 저장 게이지/인벤토리 용량/전투 스탯(GAS)에는 반영하지 않고 칩 스테이션 자체 표시만 바꾼다. 실제 저장 게이지는 레이드에서만 변한다(아래 "레이드 중 시간 감소"). 로비에서 슬라이더가 저장 게이지를 내리면 적재 프로토콜이 줄어 인벤토리 초과분이 월드로 버려지므로(아이템 손실) 저장을 끊었다.
  - **칩 목록 필터 버튼** (`SortButton1~5`, `WBP_SortButton` 5개): `WBP_ChipStation`에 배치된 순서대로 ALL / 생존 / 적재 / 탐색 / 전투를 담당한다(`ULSChipStationWidget::BindSortButtons`). ALL은 모든 칩을 표시하고, 나머지는 고른 프로토콜 수치가 1 이상인 칩만 표시한다. 여러 프로토콜 수치가 1 이상인 칩은 해당하는 각 필터에서 모두 보인다. 필터 안에서는 가격 내림차순 → 인벤토리 우선 → 슬롯 인덱스 순서로 정렬한다. 필터 기준은 SaveGame에 저장하지 않는 위젯 인스턴스의 화면 상태이며(`ChipFilterProtocol`, 미설정=ALL), 위젯 인스턴스가 유지되는 동안에는 닫았다 다시 열어도 마지막 선택을 유지한다. 선택된 버튼은 색으로만 표시한다(`ULSStorageButtonWidget::SetSelected` — 비활성화하면 슬레이트가 채도를 죽여 색이 탁해진다).
- **칩 리스트 인덱스 동기화**: 칩 리스트 슬롯 위젯은 만들어질 때의 저장 영역/인덱스(`SourceArea`/`SourceSlotIndex`)를 캐시하므로, 다른 탭에서 창고·인벤토리를 정렬/이동하면 인덱스가 어긋나 장착이 조용히 실패한다(`[Save] Cannot equip chip because source slot is invalid`). 이를 막기 위해 **로비 칩 세팅 패널을 열 때마다**(`ULSLobbyMenuWidget::ShowPanel` → `RefreshPanelOnOpen`) 그리고 **칩 스테이션 액터 상호작용으로 열 때마다**(`ALSPlayerControllerBase::ShowChipStationWidgetLocal`) `RefreshChipStation`으로 풀 리빌드한다. 로비가 배타 패널 구조여도 다른 패널을 거쳐 재진입하면 반드시 이 갱신을 다시 탄다. 로비 전환 계약은 [LobbyScreenStructure.md](LobbyScreenStructure.md)가 소유한다.
- **칩 스테이션 프리뷰** (`ULSChipStationWidget`): `WBP_ChipStation`에 `ULSMinimapWidget` 기반 자식 위젯을 `Minimap` 이름으로 배치하면 실제 월드 데이터 대신 더미 지형/마커 프리뷰를 표시한다. `ULSSurvivalStatusWidget` 기반 자식 위젯을 `SurvivalStatus` 이름으로 배치하면 같은 신호 게이지 테스트 레벨과 더미 체력/스태미나로 생존 UI 프리뷰를 표시한다. 칩 스테이션 프리뷰의 프로토콜 4종 레벨은 기본적으로 장착 칩 프로토콜 합산값(현재=신호 활성 칩, 이전=전체 칩)을 표시한다. 단 프로토콜 디버그 패널이 화면에 떠 있고(`ALSPlayerControllerBase::IsProtocolDebugWidgetVisible`) 해당 프로토콜에 오버라이드 값이 설정돼 있을 때(`HasProtocolTestLevel`)만 그 디버그 값을 따른다. 패널을 닫으면 잔존 오버라이드는 무시하고 칩 합산값으로 복귀한다. 신호 게이지 슬라이더는 프리뷰 상 활성 칩 집합을 바꿔 현재 레벨 표시에 반영하지만, 저장 게이지·용량·GAS는 건드리지 않는다(위 "칩 스테이션 목록" 참고). 미니맵 표시 규칙의 단일 출처는 [MinimapSystem.md](MinimapSystem.md)다.
- **칩 스테이션 전투 스탯 칸 — 제거됨**: `WBP_ChipStation`에 있던 `ChipStat_*` 10칸(`WBP_ChipStat` / `ULSChipStatWidget`) 표시를 UI 개편으로 걷어냈다. C++ 쪽 `BindWidget` 10개와 이를 갱신하던 `SetChipStat`/`GetStatWidget`, 그리고 표시 전용 합산(`AggregateChipStatTotals` 호출·신호유실 50% 계산)도 함께 삭제했다. 스탯 수치 자체는 GAS 적용 경로(아래 "칩 전투 스탯 → 캐릭터 GAS 연동")와 칩 툴팁에서만 쓰인다. `ULSChipStatWidget` 클래스와 `WBP_ChipStat` 에셋은 남아 있으나 현재 참조하는 화면이 없다.
- **칩 스테이션 닫힘** (`ALSChipStationActor`): 칩 설정 상호작용 범위에서 로컬 플레이어가 벗어나면 `ALSPlayerControllerBase::HideChipStationWidget`으로 스테이션 UI를 닫는다.
- **하드웨어 슬롯** (`ULSChipEquipmentSlotWidget`): 칩 스테이션 목록에서 드래그한 칩을 `EquipmentSlot_0~9` 내부 `ItemSlot`에 저장 이동으로 장착할 수 있다. 장착 슬롯끼리 드래그하면 빈 슬롯으로는 이동하고, 이미 장착된 슬롯과는 교환한다. 장착 칩을 `ChipSlotBorder` 빈 영역으로 드래그하면 장착 해제되고(목적지는 아래 "장착 해제"의 출처 기억 규칙), 칩 리스트 아이템 위에 드롭하면 해당 인벤토리/창고 슬롯과 교환한다. 신호 게이지가 90.0% 이하로 내려갈 때부터 1번 슬롯부터 10% 단위로 비활성 처리하며, 장착 칩의 기본 `ChipStats` 10종 합산값은 활성·비활성 슬롯을 모두 포함한다. 활성 100% + 비활성 50% 가산 규칙의 단일 출처는 아래 "칩 전투 스탯 → 캐릭터 GAS 연동"이다. 장착 칩은 SaveGame에 저장되어 칩 스테이션 재오픈 시 복원된다. 신호 게이지는 슬라이더 프리뷰라 저장하지 않으며(위 "칩 스테이션 목록" 참고), 재오픈 시 저장 게이지(로비 100%)로 초기화된다. 장착 칩의 `Item_MemoryCost` 합계는 `MemoryText`에 `현재/최대` 형식으로 표시한다. 메모리 검증은 아직 없다.
  - **드래그 장착/교환 위치 유지**: 리스트→장착칸 드래그 장착, 장착칸끼리 이동/교환, 장착칸→리스트칸 드롭(해제 교환)도 **칩 리스트를 재정렬/리빌드하지 않는다**. 조작으로 내용이 바뀐 리스트 칸 한 개만 저장 슬롯 내용으로 그 자리에서 갱신한다. 장착으로 비면 hole로 남고, 스왑으로 돌아온 칩은 현재 필터에 맞을 때만 표시하며 맞지 않으면 hole로 남긴다. 장착칸·요약·용량은 다음 틱에 경량 갱신한다. (`ULSItemSlotWidget::RefreshChipStationSlotFromStored` + `ULSChipStationWidget::EquipChipToHardwareSlot` / `DropEquippedChipToHardwareSlot` / `SwapEquippedChipWithStoredSlot` → `QueueRefreshEquippedChipState`)
  - **빠른 장착 (Shift+좌클릭)**: 칩 리스트(인벤토리+창고 합친 창) 슬롯을 Shift+좌클릭하면 첫 빈 장착칸에 1개 장착하고, **그 소스 슬롯 한 칸만 그 자리에서 비운다**(칩 리스트는 재정렬/리빌드하지 않음). Shift를 누른 채 커서를 칩들 위로 쓸면 지나가는 칸이 차례로 장착된다(다른 빠른이동과 동일하게 `NativeOnMouseEnter`/`NativeOnMouseMove` 기반, 별도 타이머 없음). 슬롯이 당겨지지 않으므로 같은 칸이 재호출돼도 비어 있어 무해하다. 장착마다 칩 리스트 전체를 다시 그리지 않고 장착칸·요약·용량만 다음 틱에 1회로 합쳐 경량 갱신한다. (`ULSItemSlotWidget::TryHandleChipStationQuickTransfer` → `ULSChipStationWidget::QuickEquipChipToFirstEmptyHardwareSlot` / `QueueRefreshEquippedChipState`)
  - **장착/해제 사운드**: 장착 성공(드래그/빠른 장착/리스트 교환) 시 `ULSChipStationWidget::ChipEquipSound`, 해제 성공(인벤토리/창고 공통) 시 `ChipUnequipSound`를 2D로 재생한다. 두 사운드는 `WBP_ChipStation` 클래스 디폴트에서 매핑하며(`Content/LostSignal/Audio/SFX/Chip/Chip In·Chip Out`), 미할당이면 `LogLS` Warning만 남긴다. 장착칸끼리 이동/교환은 무음.
  - **장착 해제 (Shift+좌클릭 / `ChipSlotBorder` 빈 영역 드래그)**: 해제 목적지는 **출처 기억**으로 정한다. 화면 세션(스테이션이 열려 있는 동안) 중 장착한 칩은 출처(영역+슬롯 인덱스)를 위젯의 transient 맵(`ChipOriginByEquipmentIndex`)에 기억한다 — SaveGame에 저장하지 않으며, 풀 리빌드(`RefreshChipStation` = 재오픈)마다 리셋된다. 장착칸끼리 이동/교환하면 레코드도 따라간다.
    - **인벤토리 출신**: 원래 그 슬롯이 비어 있으면 그 슬롯으로, 아니면 인벤토리 첫 빈 칸으로 복귀. 자리가 없으면 창고로 폴백하고 "창고로 보냈습니다" 알림을 띄운다.
    - **창고 출신**: 기존처럼 창고로(알림 없음).
    - **기억 없음(재오픈 후 등)**: 인벤토리 첫 빈 칸을 먼저 시도하고, 가득이면 창고 폴백+알림.
    - **용량 예측(중요)**: 해제하면 적재(Carrying) 보너스가 줄어 인벤토리 최대 슬롯 수가 줄 수 있으므로, 인벤토리 배치 칸은 반드시 "해제 후 예상 최대 슬롯 수"(가정 배열 `ComputePredictedMaxInventorySlotCount`) 미만에서만 고른다(`ULSSaveSubsystem::UnequipChipToInventory`, 스택 병합 없이 빈 칸 배치라 배치 인덱스가 항상 정확). 자리가 없으면 상태 무변경 실패 → 위젯이 창고 폴백.
    - 칩 리스트는 재정렬/리빌드하지 않고 현재 필터에 맞는 돌아온 칩만 첫 빈 칸(hole)에 넣는다. 필터와 다르면 저장 데이터만 갱신하고 목록에는 넣지 않는다. 인벤토리 복귀는 배치 인덱스를 직접 알아 diff가 필요 없고, 창고 경로만 해제 전/후 "채워진 창고 인덱스" 차이로 위치를 찾는다(스택 병합 예외는 풀 새로고침 폴백). 드래그·Shift 두 해제 경로 공용. (`ULSChipStationWidget::UnequipChipFromSlot` → `TryUnequipChipToInventory` / `UnequipChipToWarehouseWithListUpdate` / `InsertChipListSlot`)
    - Shift 해제 성공 시 장착칸 `ItemSlot`을 즉시 `ClearItem`으로 비운다 — 장착칸 갱신이 다음 틱이라 `bHasItem`이 stale로 남으면 Shift 쓸기(`NativeOnMouseMove`)가 같은 제스처에서 빈 칸 해제를 반복 시도해 `[Save] Cannot unequip chip because equipment slot is empty` 경고가 중복으로 찍힌다.
  - **적재 용량 부족 시 해제 차단**: 칩을 해제하면 적재(Carrying) 프로토콜이 낮아져 인벤토리 최대 슬롯 수가 줄고, 초과분이 월드로 드롭되어 아이템이 손실된다([InventoryLogic.md](InventoryLogic.md) 적재 감소 처리). 이를 막기 위해, 해제 후 예상 최대 인벤토리 슬롯 수보다 현재 채워진 아이템이 많으면 **해제 자체를 막고** 알림 팝업을 띄운다. 판정은 `ULSSaveSubsystem::WouldUnequipChipDropInventoryItems`(해제 대상 칸을 비운 가정 배열로 `ComputeCarryingProtocolSlotBonus`를 재계산 → 실제 해제 후 `GetMaxInventorySlotCount()`와 동일값)가 단일 출처이며, 데이터 반영(`UnequipChipToInventory`/`UnequipChipToWarehouse`) **전에** 검사한다. 드래그·Shift 두 해제 경로 공용(둘 다 `UnequipChipFromSlot`을 통과). 알림은 공용 확인 다이얼로그 `ULSConfirmDialogWidget`를 코드로 띄운다(`ShowCapacityBlockedDialog` → 타이틀/세팅의 알림 팝업과 동일 패턴, 확인/취소/ESC 모두 그냥 닫힘, z-order `LSUILayer::ModalPanelDialog`). `WBP_ChipStation`의 `ConfirmDialogClass` 기본값에 `WBP_ConfirmDialog`를 매핑해야 표시된다(미매핑 시 `LogLS` Warning).
    - **로비 전용 차단**: 이 차단·알림은 로비에서만 동작한다. 레이드 중에는(`IsRaidActive()`) 초과분을 그대로 월드에 버리는 기존 정책([InventoryLogic.md](InventoryLogic.md))을 따른다. 칩 스테이션 자체가 로비 전용이지만 규칙을 코드로 명시해 레이드에서 실수로 막히지 않게 한다.
    - **일반 인벤토리만 판정**: 실제 손실(월드 드롭)이 나는 일반 인벤토리 초과만 차단 기준으로 본다. 보호 슬롯(Safe)은 감소 시 드롭되지 않고 잠기기만 하므로(손실 없음) 이 판정 대상이 아니다.
    - **신호 유실(게이지 자동 감소)**로 인한 감소도 차단 대상이 아니다(레이드 중 발생, 기존 즉시 월드 드랍 유지).
    - **칩↔칩 교체(스왑)도 차단 대상**이다. 장착 칩을 적재 프로토콜이 더 낮은 칩으로 교체하면 용량이 줄어 초과분이 드롭되므로, 스왑으로 들어올 칩을 얹은 가정 배열(`ULSSaveSubsystem::WouldSwapChipDropInventoryItems`)로 판정해 데이터 반영(`EquipChipFromStoredSlot`) **전에** 막고 같은 알림을 띄운다. 해제·스왑 두 판정은 가정 장착 배열로 인벤토리 드롭을 예측하는 공용 코어(`WouldChipEquipmentDropInventoryItems`)를 공유한다. 로비 전용 규칙(레이드 중 미차단)도 해제와 동일하다.

### ✅ 칩 전투 스탯 → 캐릭터 GAS 연동

장착 칩 합산 전투 스탯을 캐릭터 GAS 어트리뷰트에 적용한다. 적용값은 **전체 장착 칩 100% + 신호 유실(비활성) 칩 50% 보너스 가산**이다 — 신호 게이지 감소로 깎이는 것은 프로토콜 수치뿐이고, 비활성 칩 스탯 절반은 오히려 보너스로 얹힌다. 계산의 단일 출처는 `LSChipStats::ComputeEffectiveChipStatTotals`다. 수치 변경은 무한 지속 GE `ULSGE_ChipStats`의 가산 모디파이어(SetByCaller)로만 적용한다(직접 SetAttribute 금지).

- 적용 주체: `ULSChipStatComponent` (`Source/LostSignal/Characters/LSChipStatComponent.*`) — `ALSPlayerCharacter`에 부착. 서버 권한에서만 동작.
- 적용 시점: 캐릭터 `BeginPlay`(ASC 초기화 후) 1회 + `ULSSaveSubsystem::OnChipLoadoutChanged`(장착/이동/해제/신호 게이지 변경) 시 remove & reapply.
- **순서 의존(중요):** 칩은 베이스 어트리뷰트 위에 얹는 GE 모디파이어이므로, 베이스 캐릭터 스탯 초기화(`Init*`로 직접 세팅) 뒤에 칩을 적용해야 한다. `ALSPlayerCharacter::BeginPlay`는 `InitializeBaseAttributes()`(파생 클래스가 DataTable 베이스 스탯을 채우는 가상 훅) 호출 **뒤에** `RefreshChipStats()`를 부른다. 순서가 뒤바뀌면 `Init*`가 애그리게이터를 우회해 직접 값을 써서 칩 보정과 풀피를 덮어쓴다.
- 데이터 소스: `ULSSaveSubsystem`의 `GetChipEquipmentSlots()` / `GetChipSignalGaugePercent()` (GameInstance 서브시스템이라 레벨 전환에도 유지 — 프로토콜 적용 패턴과 동일).
- 체력 처리: **초기 적용(캐릭터 스폰 직후, `bRestoreFullHealth=true`)만** `CurrentHealth`를 칩 보정된 `MaxHealth`로 채운다. 이후 델리게이트 경유 갱신(레이드 중 신호 게이지 감소, 장착 변경)은 **잃은 체력량을 보존**한다 — `MaxHealth`가 늘면 증가분만큼 `CurrentHealth`도 같이 올리고(신호 유실 보너스 반영), 줄면 새 `MaxHealth`로 클램프만 해서 재적용이 회복 수단이 되지 않게 한다. (장비 `ULSEquipmentStatComponent`는 기존 체력 보존 + 클램프 규칙)

**스탯 키 → 어트리뷰트 매핑 원장**

| 칩 스탯 키 | 어트리뷰트 | AttributeSet | 적용 규칙 | 상태 |
|---|---|---|---|---|
| Chip_Attack | Attack | Character | 평탄 가산 | ✅ 연동 |
| Chip_Health | MaxHealth | Combat | 평탄 가산 | ✅ 연동 |
| Chip_Defense | Defence | Character | 평탄 가산 | ✅ 연동 |
| Chip_Recovery | Recovery | Character | 평탄 가산 | ✅ 연동(초당 체력 회복 소비) |
| Chip_Attack_Speed | AttackSpeed | Character | 퍼센트 ÷100 가산 | ✅ 연동(기본공격 재생속도 소비) |
| Chip_Move_Speed | MoveSpeed | Character | 퍼센트 ÷100 가산 | ✅ 연동(이동속도+애님 재생속도 소비) |
| Chip_Critical_Damage | CritDamage | Character | 퍼센트 ÷100 가산 | ✅ 연동(데미지 계산 소비) |
| Chip_Critical_Rate | CritChance | Character | 퍼센트 ÷100 가산 | ✅ 연동(데미지 계산 소비) |
| Chip_Defense_Penetration | ArmorPenetration | Character | 퍼센트 ÷100 가산 | ✅ 연동(데미지 계산 소비) |
| Chip_Skill_Haste | CooldownReduction | Character | 퍼센트 ÷100 가산 | ✅ 연동(스킬 쿨다운 소비) |

> Attack / CritDamage / CritChance / Defence / ArmorPenetration은 `LSDamageExecutionCalculation`이 데미지 계산에 소비한다. CooldownReduction은 `LSPlayerSkillComponent`가 스킬 쿨다운에, AttackSpeed는 `LSGA_PlayerBasicAttack`이 콤보 몽타주 재생속도에, Recovery는 `ALSPlayerCharacter::UpdateHealthRecovery`가 초당 체력 회복에 소비한다. MoveSpeed는 `ALSPlayerCharacter::RefreshMaxWalkSpeed`가 `MaxWalkSpeed`(걷기/뛰기 × 배수)에 반영하고, 발 미끄러짐 방지를 위해 로코모션 AnimBP가 `ULSAnimInstanceBase::MoveSpeedMultiplier`를 BlendSpace 재생속도(Play Rate)에, `GaitSpeed`(=Speed÷배수)를 속도 축에 연결한다.

### ❌ 미구현

| 블록 | 항목 |
|---|---|
| [2] | 칩 장착/이동 가능한 전용 인벤토리 UI / 칩 오버랩(정보) UI |
| [3] | 하드웨어 UI — 메모리 초과 검증 |
| [4] | 소프트웨어 UI — 코어 출력 게이지바, 신호 유실 표시, 프로토콜 오버랩 UI |
| [4] 로직 | 프로토콜 합산값 이후 레벨/단계 산식 확정, 전투지역 편의 UI 활성화 연동 |

**한 줄 요약**: 데이터 레이어와 보관/툴팁/칩 스테이션 목록, 칩 장착/이동/교환, 신호 게이지 기반 장착 칩 비활성화, 장착 칩 스탯·프로토콜 합산 표시와 `DT_Protocol` 기반 단계 표시가 구현되어 있다. 칩 전투 스탯 10종을 모두 캐릭터 GAS 어트리뷰트에 적용(활성 100% + 비활성 50%)하고, 10종 전부 실제 게임플레이(데미지·쿨다운·공격속도·체력 회복·이동속도)에 소비된다. 이동속도는 애니메이션 재생속도까지 동기화해 발 미끄러짐을 막는다(BlendSpace Play Rate·속도 축 연결은 아트 담당). 메모리 초과 검증은 아직 미구현이다.

---

## 3. 데이터 Row 카탈로그 (현행)

### FLSChipRow (`DT_ChipRow`)

| 필드 | 타입 | 설명 |
|---|---|---|
| `Item_Text` | FText | 아이템 이름 |
| `Item_Type` | int32 | 0 = 칩 |
| ~~`Item_Grade`~~ | — | **제거됨** — Name(`Chip_{Grade}_{Func}`)에서 파싱. 아래 "중복 제거 원칙" 참고 |
| `Item_Max` | int32 | 스택 최대 (칩은 1) |
| `Item_Description` | FText | 설명 |
| `Item_Cost` | int32 | 가격 |
| `Item_Equipment` | FString | 장착 슬롯 종류(`chip`) |
| `Item_Chip_Status_Count` | int32 | 이 칩이 가진 전투 스탯 개수 (등급 범위에서 생성) |
| `Item_MemoryCost` | int32 | 메모리 비용 |
| `Chip_Protocol_Survival` | int32 | 생존 프로토콜 수치 |
| `Chip_Protocol_Carrying` | int32 | 적재 프로토콜 수치 |
| `Chip_Protocol_Battle` | int32 | 전투 프로토콜 수치 |
| `Chip_Protocol_Navigation` | int32 | 탐색 프로토콜 수치 |

> **[임시 값]** 현재 CSV의 프로토콜 4종 값은 UI 확인용 임시값이다. `Item_MemoryCost`는 전부 1로 입력되어 있다. 등급/타입별 밸런싱 데이터 입력이 필요하다.
> **[임시 적용]** 프로토콜 컬럼 합계가 0인 칩은 `Item_Chip_Status_Count`를 임시 프로토콜 파워로 사용한다. RowName 기능 토큰 기준으로 HP/SP는 생존, Inventory는 적재, Minimap/Exitpoint는 탐색에 합산한다. 프로토콜 컬럼에 명시값이 입력되면 명시값을 우선 사용한다.

### FLSProtocolUnlockRow (`DT_Protocol`)

`DT_Protocol`은 프로토콜 해금 항목의 단일 출처다. 첫 컬럼 RowName의 접두사로 생존/적재/전투/탐색 타입을 판정하고, `Protocol_Required_Level` 이하의 수치는 코드나 문서에 중복 저장하지 않는다.
프로토콜 위젯의 단계 스트립은 `Protocol_Required_Level` 종류 수와 무관하게 순번 7칸 고정이며, 순번이 현재 프로토콜 레벨 이하인 칸만 해금 색으로 표시한다. 해금 항목별 이름과 보호 표시 여부는 호버 툴팁이 각 row의 `Protocol_Protected_Level`로 별도 판정해 보여준다.
프로토콜 이름 이미지는 `ULSProtocolWidget`의 `ProtocolNameImage`(BindWidget)에 표시하며, 텍스처는 배치한 WBP(WBP_ChipStation 등)에서 인스턴스별 `ProtocolNameTexture`로 4종을 각각 지정한다. 호버 툴팁 아이콘도 같은 `ProtocolNameTexture`를 쓴다(별도 툴팁 아이콘 지정값을 두지 않는다). 툴팁이 항목을 조회할 때 쓰는 `ProtocolType`은 배치한 부모 위젯이 `SetProtocolType`으로 채우므로 WBP에서 지정하지 않는다. 프로토콜 칸 배경/테두리 Border는 `ProtocolBorder`(BindWidget)로 바인드하고, 색은 `ProtocolBorderColor`에서 조정한다. 프로토콜 툴팁 항목 색은 `ULSProtocolTooltipTextWidget`이 관리하며, 미해금 항목은 낮은 명도와 투명도의 `LockedColor`로 배경에 물러나게 표시한다.

#### 단계 스트립 위젯 구조 (`WBP_Protocol` / `WBP_ProtocolStage`)

`ULSProtocolWidget`은 단계 칸을 `ProtocolStage_1`~`ProtocolStage_7` 이름의 필수 BindWidget 7개로 잡는다. 각 칸의 부모 클래스는 `ULSProtocolStageWidget`(`WBP_ProtocolStage`)이며, 필수 바인딩은 상태 박스 이미지 `StageImage`와 순번 숫자 `StageText` 둘이다. 순번 숫자와 해금 여부 색은 C++이 채우고, 해금/미해금 색은 `ULSProtocolStageWidget`의 `UnlockedBoxColor`/`LockedBoxColor`/`UnlockedTextColor`/`LockedTextColor`에서 조정한다. 데이터 단계 수가 7보다 적어도 남는 칸은 숨기지 않고 미해금 색으로 남긴다.

| 필드 | 타입 | 설명 |
|---|---|---|
| `Protocol_Required_Level` | int32 | 해금에 필요한 현재 프로토콜 레벨 |
| `Protocol_Enable_Type` | FName | UI, Protection 등 해금 항목 분류 |
| `Protocol_Enable_Name` | FName | 코드에서 찾는 해금 항목 이름. `Stemina_*` 입력은 조회 시 `Stamina_*`로 정규화 |
| `Protocol_Enable_Value` | int32 | 항목별 보조 값. `Protocol_Protected_Level`과 같은 값을 중복 저장하지 않는다 |
| `Protocol_Protected_Level` | int32 | 이전에 해금된 항목을 신호 유실 후에도 유지할 최소 현재 레벨. 0이면 보호 없음 |

#### 중복 제거 원칙 (Name 파싱) — ✅ 적용 완료

등급은 Row Name에서 파싱하며, **`Item_Grade` 컬럼/필드는 전 테이블(Chip/Weapon/Armor/Item)에서 제거**했다.

- 파서: `LSInventorySlotUtils::ResolveItemGradeFromRowName(FName)` — Name을 `_`로 분리해 **알려진 등급 토큰**(Supply/Standard/Precision/Tuning/Prototype/Masterpiece)을 찾아 반환. **위치 무관**.
- Name 규칙(등급 토큰 포함):
  - Chip: `Chip_{Grade}_{Func}` (예: `Chip_Supply_HP`) — 기존 그대로
  - Armor: `Armor_{Func}_{Grade}` (예: `Armor_Frame_Supply`) — 기존 그대로
  - Weapon: `Weapon_{...}_{Grade}` — **등급 토큰 없어서 끝에 추가** (`Weapon_Alloy_Slab` → `Weapon_Alloy_Slab_Supply`)
  - Item: `Item_{...}_{Grade}` — **등급 토큰 없어서 끝에 추가** (`Item_Electricwire` → `Item_Electricwire_Supply`)
- Weapon/Item Name 변경에 따라 `DT_GroupTable.csv`의 `Item_Name` 참조(9건)도 동시 갱신함.
- 코드: 구조체 4개에서 `Item_Grade` 제거, 툴팁 `SetCommonTexts` 4곳이 파서를 호출하도록 변경. 등급 표시 매핑(`GetGradeText`)은 그대로.
- 일반 원칙: **데이터가 다른 컬럼/규칙에서 유도 가능하면 중복 저장하지 말고 파싱/계산으로 대체**한다.

> **에디터 후속(사용자 작업)**: 모든 DataTable Reimport. row 이름이 바뀐 기존 아이콘 에셋은 파일명 기준으로 갱신 완료(`Item_Electricwire` → `Item_Electricwire_Supply`, `Chip_Presision_SP/Inventory` → `Chip_Precision_*`). 필요 시 에디터에서 리다이렉터 정리.

### FLSChipStatRow (`DT_ChipStat`)

**행 Name = 등급명**(`Supply` / `Standard` / `Precision` / `Tuning` / `Prototype` / `Masterpiece`). 각 스탯은 `min~max` **범위 한 칸**으로 표현한다(아래 표 셀이 곧 한 스탯의 범위). — ✅ 적용 완료

- 공격(Attack) 계열: Attack, Attack_Speed, Skill_Haste, Critical_Rate, Critical_Damage, Defense_Penetration
- 방어(Defense) 계열: Health, Defense, Recovery
- 생존(Survival) 계열: Move_Speed

> **실제 등급별 수치는 [DT_ChipStat.csv](../../Content/LostSignal/Sandbox/DT/DT_ChipStat.csv)가 단일 출처다.** 문서에 수치를 중복 기재하지 않는다(밸런싱으로 자주 바뀜).

> **[해소됨]** 등급 ↔ 행 매핑은 행 Name을 등급명으로 통일. 칩 등급은 Name에서 파싱.
> **[표기 변경 완료]** 스탯당 `_Min`·`_Max` 두 컬럼 → **`min~max` 단일 표현**으로 변경 완료.
> - 코드: `FLSChipStatRow`가 스탯당 `FLSStatRange{int32 Min; int32 Max;}` 1개를 가짐. `FLSStatRange`는 `ImportTextItem`/`ExportTextItem`으로 `"10~15"` 텍스트를 직접 import/export (`LSChipStatRow.h`).
> - CSV: 스탯당 한 컬럼, 값 `10~15`.
> - ⚠️ **검증 필요**: `CreateTableFromCSVString` Reimport 시 `10~15` 셀이 `FLSStatRange`로 정상 파싱되는지 에디터에서 확인. (만약 import가 안 되면 대안: 필드를 `FString`으로 두고 런타임 파싱)
> **[확인 필요 → 해소]** 스탯 선택 = 랜덤 N개 (4절 #2).

---

## 4. 확인 필요 사항 (기획 결정 대기)

1. ~~**스탯 산출 방식**~~: **확정 — 인스턴스 롤링 + 스냅샷 저장.** 칩 획득 시 범위 내 랜덤값을 굴려 그 칩에 고정(같은 등급도 개체마다 다름). 확정 스탯은 `FLSSessionItem` / `FLSDropResult`의 `ChipStats` 배열에 저장하며, DataTable 변경 후에도 기존 칩 값은 바뀌지 않는다. → [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md).

2. ~~**스탯 선택 규칙**~~: **확정 — 랜덤 N개.** 등급별 10개 스탯 풀에서 `Item_Chip_Status_Count`개를 무작위 선택하고, 각 값은 해당 스탯의 `min~max`에서 롤링. (선택된 스탯 종류 + 값 모두 시드로 결정론적 재현)

3. ~~**등급 ↔ ChipStat 행 매핑**~~: **해소** — ChipStat 행 Name을 등급명으로 통일(`Supply`~`Masterpiece`). 칩 등급은 Name(`Chip_{Grade}_{Func}`)에서 파싱 → `Item_Grade` 컬럼 제거 예정.

4. ~~**신호 게이지 비활성 경계**~~: **90.0% 이하부터 1번 슬롯 비활성, 이후 10% 단위로 2~10번 슬롯을 순서대로 비활성화한다.** 기본 장착 칩 스탯 합산은 활성·비활성 칩을 모두 포함하고, 프로토콜 합산은 비활성 칩을 제외한다. 최종 전투 스탯은 기본 합산에 비활성 칩 스탯의 50%를 가산한다. (결정 당시 이 값을 표시하던 칩 스테이션 전투 스탯 칸은 이후 UI 개편으로 제거됐다 — 수치 규칙만 유효)

5. ~~**프로토콜 레벨/단계 산식**~~: **합산 수치를 그대로 프로토콜 레벨로 사용하고, 단계별 활성화 임계값은 `DT_Protocol`의 `Protocol_Required_Level`을 기준으로 판정한다.**

---

## 5. 단계별 구현 계획

의존성 순서. 각 Phase는 독립적으로 검증 가능하도록 구성한다.

### Phase 1 — 칩 스탯 산출 로직 + 툴팁 보강

**1-A 롤링 본체 (백엔드) — ✅ 완료**
- `FLSSessionItem` / `FLSDropResult`에 `TArray<FLSChipResolvedStat> ChipStats` 저장. 값 복사로 이동·저장·복제 시 보존, `ToSessionItem`/`SetDropResultFromSessionItem`이 배열을 전달.
- 드랍 생성(`ULSDropSubsystem::RollDropTable`)에서 칩(`Chip_` 접두사)에 `LSChipStats::RollChipStats(RowName)` 결과를 부여 → **서버 권위 지점에서 1회 롤**.
- 롤러 `LSChipStats::RollChipStats(RowName)` (`Source/LostSignal/Data/LSChipStats.{h,cpp}`):
  - 등급 = Name 파싱, 개수 = `Item_Chip_Status_Count`, 범위 = ChipStat 등급 행(`FLSStatRange`).
  - 10개 스탯 풀에서 **N개 무작위 선택(Fisher-Yates) + 각 값 범위 내 롤**. 결과는 칩 인스턴스의 `ChipStats` 배열에 스냅샷으로 저장.
  - 표시용 `GetChipStatLabel(StatKey)` 동봉.

**1-B 툴팁 표시 (UI 배선) — ⬜ 다음 슬라이스**
- 슬롯/툴팁이 `ChipStats`를 받도록 시그니처 확장(`ULSItemSlotWidget::SetItem` → `ULSItemTooltipWidget::SetItem`)하고 호출부 갱신.
- 칩 툴팁이 저장된 `ChipStats` 결과(확정 전투 스탯)와 프로토콜 수치를 표시.
- **검증**: 칩 드랍/보관 시 개체별로 다른 스탯이 툴팁에 노출, 같은 칩은 재오픈해도 동일.

### Phase 2 — 칩 장착 데이터 모델 (하드웨어 백엔드)
- 칩 장착 슬롯(10칸 임시) 보유 컴포넌트/구조.
- 메모리 초과 검증 로직.
- 신호 게이지(0~100%) 상태 + 10% 단위 → 칩 번호순 비활성화 규칙.
- **검증**: 장착/해제, 메모리 초과 거부, 게이지 변동 시 활성/비활성 칩 전환 로직 단위 테스트.

### Phase 3 — 프로토콜 집계 (소프트웨어 백엔드)
- 장착 칩들의 프로토콜 4종 합산 → 레벨/단계 계산.
- 단계 달성 시 전투지역 편의 UI 활성화 훅(인터페이스만이라도 정의).
- **검증**: 칩 조합별 프로토콜 레벨/단계 계산 결과 검증.

### Phase 4 — UI 구현
- (2) 칩 설정 인벤토리 + 칩 오버랩 정보 UI.
- (3) 하드웨어 UI: 슬롯/메모리 한도/신호 게이지 스크롤바.
- (4) 소프트웨어 UI: 프로토콜/코어 출력 게이지바/신호 유실/프로토콜 오버랩.
- **검증**: 실제 장착 플로우를 UI로 조작하며 백엔드 상태와 동기화 확인.

---

## 6. 관련 파일

- 데이터: `Source/LostSignal/Data/LSChipRow.h`, `LSChipStatRow.h`, `LSDropSettings.h`, `LSChipStats.{h,cpp}` (집계/`ComputeEffectiveChipStatTotals`)
- GAS 연동: `Source/LostSignal/Characters/LSChipStatComponent.{h,cpp}`, `Source/LostSignal/GAS/Effects/LSGE_ChipStats.{h,cpp}`, `Source/LostSignal/GAS/LSGameplayTags.*` (`LS.Data.Chip.*`)
- 테이블: `Content/LostSignal/Data/DataTables/DT_ChipRow.uasset`, `DT_ChipStat.uasset`
- CSV: `Content/LostSignal/Sandbox/DT/DT_Chip.csv`, `DT_ChipStat.csv`
- 툴팁: `Source/LostSignal/UI/Inventory/LSItemTooltipWidget.cpp` (`PopulateChipTooltip`)
- 보관함: `Source/LostSignal/UI/Storage/LSLobbyStorageWidget.cpp`
- 슬롯 아이콘: `Source/LostSignal/UI/Inventory/LSItemSlotWidget.cpp`
- 칩 UI 에셋: `Content/LostSignal/UI/Storage/WBP_ChipStorage.uasset`

## 7. 생존 프로토콜 UI 연동

- `ULSSurvivalStatusWidget`은 HUD 고정 생존 UI를 담당한다. `WBP_PlayerHUD`에는 `SurvivalStatus` 이름의 `ULSSurvivalStatusWidget` 자식 위젯을 배치해야 한다.
- `ULSSurvivalStatusWidget`의 `HealthPreviewProgressBar`는 현재 체력 바 뒤의 배경/예상 체력 레이어다. `SetHealthPreview`는 붕대처럼 지속 회복/감소가 끝났을 때 도달할 체력 비율을 이 레이어에 표시하고, 예측이 없을 때도 바 자체는 숨기지 않는다.
- `ULSSurvivalStatusWidget`은 `SurvivalCooldownRingImage`의 UI 머티리얼 `Progress` 파라미터로 생존 상태 원형 링을 갱신한다.
- `ULSSurvivalStatusWidget`은 `ChipImage`의 `M_UI_CircleIcon` UI 머티리얼 `IconTexture` 파라미터에 다음으로 사라질 칩의 아이콘을 출력한다. 신호 비활성화는 슬롯 번호 순이라 게이지가 가리키는 "다음 비활성 슬롯"이 비어 있으면 **그 뒤로 채워진 첫 슬롯까지 건너뛰어** 실제로 다음에 사라질 칩을 표시한다. `SurvivalCooldownRingImage`의 `Progress`는 그 칩이 사라지기까지 남은 비율(1.0→0.0)을 표시한다.
  - `ALSFarmingGameMode`가 신호 드레인을 도는 레벨(실제 레이드/디버그 테스트 맵)에서는 클라이언트 자체 추정이 아니라 드레인 타이머 실제 잔여시간(`GetSignalGaugeDrainRemainingSeconds` / `GetSignalGaugeDrainInterval`)을 매 틱 읽어 링을 갱신한다. 판정 기준은 정식 레이드 세션 플래그가 아니라 **현재 게임모드가 `ALSFarmingGameMode`이고 타이머가 도는지**다. 게이지는 다음 장착 칩의 물리 슬롯 임계치까지 단계적으로 떨어지므로 빈 슬롯 구간에서는 10%를 초과해 점프할 수 있지만, 링은 다음 칩이 사라질 때까지의 실제 남은 시간을 그대로 따른다. HUD를 재초기화하는 이벤트(예: 프로토콜 디버그 토글)가 있어도 링이 리셋되지 않는다. 다른 게임모드(로비/결과)거나 타이머가 멈춘 경우(게이지 0%)에는 로비/프리뷰와 동일하게 게이지 구간 위치를 정적으로 표시한다.
  - 더 이상 사라질 칩이 없으면(장착 칩이 없거나 전부 비활성화됨) 칩 아이콘을 숨기고, 프리뷰 데모 카운트다운(`bStartPreviewRingCooldownOnConstruct`)은 끈 뒤 **시간이 다 지난 링(Progress 0)만** 남긴다. 단 링/아이콘 표시 자체는 현재 생존 프로토콜 레벨 ≥ 1(`ShouldShowSignalIndicator`) 게이팅을 따른다.
- 현재 생존 프로토콜 레벨이 0(미해금)이면 신호 유실 원형 링(`SurvivalCooldownRingImage`)과 그 안의 칩 아이콘(`ChipImage`)을 함께 숨긴다. HP/스태미나 바 게이팅(현재 레벨 ≥ 1)과 동일 기준이며, 칩 스테이션 프리뷰·실제 HUD 공용이다. `WBP_ChipStation` 프리뷰는 슬라이더 값을, 실제 HUD는 저장된 `ChipSignalGaugePercent` 값을 기준으로 갱신한다.
- `ULSSurvivalOverheadWidget`은 플레이어 캐릭터 주변 생존 UI를 담당한다. `ALSPlayerCharacter`의 `SurvivalOverheadWidgetClass`에 `WBP_SurvivalOverhead`를 지정하면 `WidgetComponent`가 로컬 표시를 초기화한다.
- 현재 생존 레벨은 신호 게이지로 비활성화된 칩을 제외한 장착 칩의 `Survival` 합산값이고, 이전 생존 레벨은 전체 장착 칩의 `Survival` 합산값이다. 표시 여부는 `DT_Protocol`의 `Protocol_Survival_*` row와 `Protocol_Protected_Level`을 `ULSGameDataSubsystem::IsProtocolUnlockVisible`로 판정한다.
- HP는 `ULSCombatAttributeSet`의 `CurrentHealth/MaxHealth`, 스태미나는 `ULSCharacterAttributeSet`의 `CurrentStamina/MaxStamina`를 ASC attribute delegate로 구독해 `현재/최대` 텍스트와 게이지로 갱신한다. 생존 프로토콜 현재 레벨이 1 이상이면 HP/스태미나 프로그레스바 셸을 표시하되, 스태미나는 1에서 채움값을 0으로 유지해 배경만 표시하고 현재 레벨이 2 이상일 때 실제 스태미나 비율을 반영한다. 달리기/대시 소모와 비전투 회복은 `ULSGE_StaminaChange`가 `CurrentStamina`에 적용한 결과를 그대로 반영한다. 달리기 지속 소모는 남은 스태미나를 0까지 차감한 뒤 달리기 상태를 종료한다.
- 테스트 콘솔 명령 `LSTestSurvivalProtocol <Level>`, `LSTestCarryingProtocol <Level>`, `LSTestBattleProtocol <Level>`, `LSTestNavigationProtocol <Level>`은 로컬 프로토콜 레벨을 임시 오버라이드한다. `LSTestAllProtocols <Survival> <Carrying> <Battle> <Navigation>`은 4종을 한 번에 지정한다. 오버라이드는 프로토콜 디버그 패널(Insert)이 화면에 떠 있을 때만 적용되고, 패널을 닫으면 저장 데이터 기반 계산으로 복귀한다. `LSClearProtocolTest`는 4종 오버라이드를 모두 0으로 지정한다.
- 프로토콜 디버그 패널(Insert)의 **"테스트 맵 가기"** 버튼은 정식 레이드 진입 시퀀스(`ALSLobbyGameMode::StartRaid` — 로드아웃 자동 제출·레이드 인벤토리·`BeginRaidSave`·세션 미러링)를 **그대로 실행하되 목적지만 `ULSSessionSettings::TestRaidLevel`로 바꿔** `ServerTravel`한다(`StartRaidToTestLevel` → `RaidLevelOverride` 1회 소비). 따라서 레이드 인벤토리·세이브·결과 저장(`레이드 종료`→탈출 플로우)까지 실제 레이드와 100% 동일하다. **로비에서만** 동작하므로(진입 셋업이 로비 게임모드 권한에서 이뤄짐) 버튼도 로비에서만 표시된다. 테스트 레벨의 World Settings GameMode는 `FarmingLevel`과 동일하게 `BP_LSFarmingGameMode`로 지정해야 한다. (레벨 레이아웃/스폰/배치는 별도 에디터 작업)
