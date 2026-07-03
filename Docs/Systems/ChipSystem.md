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
- **신호 게이지(Signal Gauge)**: 0~100% 스크롤바. 게이지가 90.0% 이하로 내려갈 때부터 10% 단위로 **칩 번호 순서대로** 비활성화된다. 기본 장착 칩 스탯 합산은 활성·비활성 슬롯을 모두 포함하며, 프로토콜 합산은 비활성 슬롯을 제외한다. 비활성 칩 스탯 값의 50%는 `SignalLossText`에 별도로 표시한다. 최종 전투 스탯은 기본 스탯 표시값과 `SignalLossText` 표시값을 더한 값이다.
  - **레이드 중 시간 감소**: 레이드(파밍 레벨) 진입 시 게이지를 100%로 채우고 **60초마다 10%씩 자동 감소**시키며 0%에서 멈춘다(= 1분에 칩 1칸씩 신호 유실). 레이드 종료(로비 복귀) 시 다시 100%로 복원한다. 감소 주체는 레이드 동안만 존재하는 서버 권한 게임모드 `ALSFarmingGameMode`이며, `ULSSaveSubsystem::SetChipSignalGaugePercent`만 주기적으로 호출해 기존 GAS 재적용(`OnChipLoadoutChanged` → `ULSChipStatComponent::RefreshChipStats`)과 HUD 갱신 경로를 그대로 탄다. (싱글/Listen 서버 기준 — 데디 MO 동기화는 기존 칩 적용 패턴과 함께 후속.)
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
| 칩 스테이션 목록(부분) | `ChipSlotWrapBox`에 저장 인벤토리/창고의 `Chip_` 아이템을 가격 높은순으로 `ULSItemSlotWidget` 표시 | `Source/LostSignal/UI/ChipSystem/LSChipStationWidget.*` |
| 하드웨어 슬롯(부분) | `EquipmentSlot_0~9` 내부 `ItemSlot`에 칩 목록 드래그 장착. 장착 슬롯끼리 이동/교환 가능. 장착 칩을 `ChipSlotBorder` 빈 영역에 드롭하면 창고 이동, 칩 리스트 아이템 위에 드롭하면 해당 저장 슬롯과 교환. 장착 칩의 스탯/프로토콜 합산값과 메모리 사용량(`현재/최대`)을 UI에 표시. 메모리 검증은 미연동 | `Source/LostSignal/UI/ChipSystem/LSChipEquipmentSlotWidget.*` |

### ⚠️ 부분 구현

- **칩 툴팁** (`PopulateChipTooltip`): 메모리 할당량, 칩 행의 **프로토콜 수치 4종**(`Chip_Protocol_*` 중 0이 아닌 것만 `"생존 프로토콜 +1"` 형식으로 전투 스탯보다 먼저 표시), 저장된 `ChipStats`의 **확정 전투 스탯 값**을 표시한다. 프로토콜 이름 텍스트의 단일 출처는 `LSProtocol::GetProtocolDisplayName`(`Source/LostSignal/Data/LSProtocolTypes.*`)이다.
- **칩 스테이션 목록** (`ULSChipStationWidget::RefreshChipSlots`): 저장 인벤토리/창고의 `Chip_` 아이템을 가격 높은순으로 아이콘/수량/툴팁 슬롯에 표시한다. `SignalSlider`와 `SignalProgressBar`는 0~1 값으로 동기화하며, 슬라이더 값은 `ULSSaveGame::ChipSignalGaugePercent`에 저장한다.
- **칩 스테이션 프리뷰** (`ULSChipStationWidget`): `WBP_ChipStation`에 `ULSMinimapWidget` 기반 자식 위젯을 `Minimap` 이름으로 배치하면 실제 월드 데이터 대신 더미 지형/마커 프리뷰를 표시한다. `ULSSurvivalStatusWidget` 기반 자식 위젯을 `SurvivalStatus` 이름으로 배치하면 같은 신호 게이지 테스트 레벨과 더미 체력/스태미나로 생존 UI 프리뷰를 표시한다. 칩 스테이션 프리뷰의 프로토콜 4종 레벨은 기본적으로 장착 칩 프로토콜 합산값(현재=신호 활성 칩, 이전=전체 칩)을 표시한다. 단 프로토콜 디버그 패널이 화면에 떠 있고(`ALSPlayerControllerBase::IsProtocolDebugWidgetVisible`) 해당 프로토콜에 오버라이드 값이 설정돼 있을 때(`HasProtocolTestLevel`)만 그 디버그 값을 따른다. 패널을 닫으면 잔존 오버라이드는 무시하고 칩 합산값으로 복귀한다. 신호 게이지 슬라이더는 활성 칩 집합을 바꿔 현재 레벨에 반영된다. 미니맵 표시 규칙의 단일 출처는 [MinimapSystem.md](MinimapSystem.md)다.
- **칩 스테이션 닫힘** (`ALSChipStationActor`): 칩 설정 상호작용 범위에서 로컬 플레이어가 벗어나면 `ALSPlayerControllerBase::HideChipStationWidget`으로 스테이션 UI를 닫는다.
- **하드웨어 슬롯** (`ULSChipEquipmentSlotWidget`): 칩 스테이션 목록에서 드래그한 칩을 `EquipmentSlot_0~9` 내부 `ItemSlot`에 저장 이동으로 장착할 수 있다. 장착 슬롯끼리 드래그하면 빈 슬롯으로는 이동하고, 이미 장착된 슬롯과는 교환한다. 장착 칩을 `ChipSlotBorder` 빈 영역으로 드래그하면 장착 해제되어 창고로 이동하고, 칩 리스트 아이템 위에 드롭하면 해당 인벤토리/창고 슬롯과 교환한다. 신호 게이지가 90.0% 이하로 내려갈 때부터 1번 슬롯부터 10% 단위로 비활성 처리하며, 장착 칩의 기본 `ChipStats` 10종 합산값은 활성·비활성 슬롯을 모두 포함한다. 비활성 슬롯의 `ChipStats` 50%는 스탯 UI의 `SignalLossText`에 표시하고, 최종 스탯은 기본 표시값과 `SignalLossText` 표시값을 합산한다. 장착 칩과 신호 게이지 값은 SaveGame에 저장되어 칩 스테이션 재오픈 시 복원된다. 장착 칩의 `Item_MemoryCost` 합계는 `MemoryText`에 `현재/최대` 형식으로 표시한다. 메모리 검증은 아직 없다.
  - **빠른 장착 (Shift+좌클릭)**: 칩 리스트(인벤토리+창고 합친 창) 슬롯을 Shift+좌클릭하면 첫 빈 장착칸에 1개 장착하고, **그 소스 슬롯 한 칸만 그 자리에서 비운다**(칩 리스트는 재정렬/리빌드하지 않음 — 정렬은 스테이션을 다시 열 때만). Shift를 누른 채 커서를 칩들 위로 쓸면 지나가는 칸이 차례로 장착된다(다른 빠른이동과 동일하게 `NativeOnMouseEnter`/`NativeOnMouseMove` 기반, 별도 타이머 없음). 슬롯이 당겨지지 않으므로 같은 칸이 재호출돼도 비어 있어 무해하다. 장착마다 칩 리스트 전체를 다시 그리지 않고 장착칸·요약·용량만 다음 틱에 1회로 합쳐 경량 갱신한다. (`ULSItemSlotWidget::TryHandleChipStationQuickTransfer` → `ULSChipStationWidget::QuickEquipChipToFirstEmptyHardwareSlot` / `QueueRefreshEquippedChipState`)
  - **장착/해제 사운드**: 장착 성공(드래그/빠른 장착/리스트 교환) 시 `ULSChipStationWidget::ChipEquipSound`, 창고 해제 성공 시 `ChipUnequipSound`를 2D로 재생한다. 두 사운드는 `WBP_ChipStation` 클래스 디폴트에서 매핑하며(`Content/LostSignal/Audio/SFX/Chip/Chip In·Chip Out`), 미할당이면 `LogLS` Warning만 남긴다. 장착칸끼리 이동/교환은 무음.
  - **장착 해제 (Shift+좌클릭 / `ChipSlotBorder` 빈 영역 드래그)**: 장착칸에서 창고로 해제할 때도 칩 리스트를 재정렬/리빌드하지 않는다. 돌아온 칩을 **칩 리스트의 첫 빈 칸(빠른 장착으로 생긴 hole)에 넣거나, 없으면 맨 뒤에 새 슬롯으로 추가**한다. 돌아온 칩의 창고 슬롯 위치는 해제 전/후 "채워진 창고 인덱스" 차이로 찾는다(기존 스택에 합쳐져 새 인덱스를 못 찾는 예외 케이스만 풀 새로고침으로 폴백). 드래그·Shift 두 해제 경로 공용. (`ULSChipStationWidget::UnequipChipFromSlotToWarehouse` / `InsertChipListSlot`)

### ✅ 칩 전투 스탯 → 캐릭터 GAS 연동

장착 칩 합산 전투 스탯을 캐릭터 GAS 어트리뷰트에 적용한다. 적용값은 **활성 칩 100% + 비활성 칩 50%**(신호 유실 규칙)이며, UI 표시 로직과 동일한 단일 출처 `LSChipStats::ComputeEffectiveChipStatTotals`를 쓴다. 수치 변경은 무한 지속 GE `ULSGE_ChipStats`의 가산 모디파이어(SetByCaller)로만 적용한다(직접 SetAttribute 금지).

- 적용 주체: `ULSChipStatComponent` (`Source/LostSignal/Characters/LSChipStatComponent.*`) — `ALSPlayerCharacter`에 부착. 서버 권한에서만 동작.
- 적용 시점: 캐릭터 `BeginPlay`(ASC 초기화 후) 1회 + `ULSSaveSubsystem::OnChipLoadoutChanged`(장착/이동/해제/신호 게이지 변경) 시 remove & reapply.
- **순서 의존(중요):** 칩은 베이스 어트리뷰트 위에 얹는 GE 모디파이어이므로, 베이스 캐릭터 스탯 초기화(`Init*`로 직접 세팅) 뒤에 칩을 적용해야 한다. `ALSPlayerCharacter::BeginPlay`는 `InitializeBaseAttributes()`(파생 클래스가 DataTable 베이스 스탯을 채우는 가상 훅) 호출 **뒤에** `RefreshChipStats()`를 부른다. 순서가 뒤바뀌면 `Init*`가 애그리게이터를 우회해 직접 값을 써서 칩 보정과 풀피를 덮어쓴다.
- 데이터 소스: `ULSSaveSubsystem`의 `GetChipEquipmentSlots()` / `GetChipSignalGaugePercent()` (GameInstance 서브시스템이라 레벨 전환에도 유지 — 프로토콜 적용 패턴과 동일).
- 칩은 로비에서만 변경되므로, 칩 적용/갱신 때마다 `CurrentHealth`를 칩 보정된 `MaxHealth`로 맞춘다(현재 체력 = 최대 체력).

**스탯 키 → 어트리뷰트 매핑 원장**

| 칩 스탯 키 | 어트리뷰트 | AttributeSet | 적용 규칙 | 상태 |
|---|---|---|---|---|
| Chip_Attack | Attack | Character | 평탄 가산 | ✅ 연동 |
| Chip_Health | MaxHealth | Combat | 평탄 가산 | ✅ 연동 |
| Chip_Defense | Defence | Character | 평탄 가산 | ✅ 연동 |
| Chip_Recovery | Recovery | Character | 평탄 가산 | ✅ 연동(어트리뷰트만, 다운스트림 소비 미연결) |
| Chip_Attack_Speed | AttackSpeed | Character | 퍼센트 ÷100 가산 | ✅ 연동(다운스트림 소비 확인 필요) |
| Chip_Move_Speed | MoveSpeed | Character | 퍼센트 ÷100 가산 | ✅ 연동(다운스트림 소비 확인 필요) |
| Chip_Critical_Damage | CritDamage | Character | 퍼센트 ÷100 가산 | ✅ 연동(데미지 계산 소비) |
| Chip_Critical_Rate | CritChance | Character | 퍼센트 ÷100 가산 | ✅ 연동(데미지 계산 소비) |
| Chip_Defense_Penetration | ArmorPenetration | Character | — | ⬜ 보류 |
| Chip_Skill_Haste | CooldownReduction | Character | — | ⬜ 보류(LoL식 스킬가속 시스템 조사 후 결정) |

> Attack / CritDamage / CritChance / Defence는 `LSDamageExecutionCalculation`이 데미지 계산에 소비한다. Recovery는 현재 다운스트림 소비처가 없고, MoveSpeed / AttackSpeed는 실제 이동/공격 속도로의 소비 연결을 후속으로 확인한다(어트리뷰트 값 반영까지는 완료).

### ❌ 미구현

| 블록 | 항목 |
|---|---|
| [2] | 칩 장착/이동 가능한 전용 인벤토리 UI / 칩 오버랩(정보) UI |
| [3] | 하드웨어 UI — 메모리 초과 검증 |
| 로직 | 방어 관통(ArmorPenetration), 스킬 가속(CooldownReduction) 칩 스탯 연동 (보류) |
| 로직 | Recovery / MoveSpeed / AttackSpeed 어트리뷰트의 실제 게임플레이 소비 연결 |
| [4] | 소프트웨어 UI — 코어 출력 게이지바, 신호 유실 표시, 프로토콜 오버랩 UI |
| [4] 로직 | 프로토콜 합산값 이후 레벨/단계 산식 확정, 전투지역 편의 UI 활성화 연동 |

**한 줄 요약**: 데이터 레이어와 보관/툴팁/칩 스테이션 목록, 칩 장착/이동/교환, 신호 게이지 기반 장착 칩 비활성화, 장착 칩 스탯·프로토콜 합산 표시와 `DT_Protocol` 기반 단계 표시가 구현되어 있다. 칩 전투 스탯 10종 중 8종을 캐릭터 GAS 어트리뷰트에 적용(활성 100% + 비활성 50%)하며, 방어 관통·스킬 가속 2종과 메모리 검증은 아직 미연동이다.

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
프로토콜 위젯의 숫자 스트립은 해당 타입의 최대 `Protocol_Required_Level`까지 표시하고, 볼드 처리는 현재 프로토콜 레벨 숫자만큼만 적용한다. 개별 항목의 보호 표시 여부는 각 row의 `Protocol_Protected_Level`로 별도 판정한다.
프로토콜 이름 이미지는 `ULSProtocolWidget`의 `ProtocolNameImage`(BindWidget)에 표시하며, 텍스처는 배치한 WBP(WBP_ChipStation 등)에서 인스턴스별 `ProtocolNameTexture`로 4종을 각각 지정한다. 프로토콜 칸 배경/테두리 Border는 `ProtocolBorder`(BindWidget)로 바인드하고, 색은 `ProtocolBorderColor`에서 조정한다. 프로토콜 툴팁 항목 색은 `ULSProtocolTooltipTextWidget`이 관리하며, 미해금 항목은 낮은 명도와 투명도의 `LockedColor`로 배경에 물러나게 표시한다.

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

4. ~~**신호 게이지 비활성 경계**~~: **90.0% 이하부터 1번 슬롯 비활성, 이후 10% 단위로 2~10번 슬롯을 순서대로 비활성화한다.** 기본 장착 칩 스탯 합산은 활성·비활성 칩을 모두 포함하고, 프로토콜 합산은 비활성 칩을 제외한다. 비활성 칩 스탯 값의 50%를 `SignalLossText`에 표시한다. 최종 전투 스탯은 기본 표시값과 `SignalLossText` 표시값을 합산한다.

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
  - `ALSFarmingGameMode`가 신호 드레인을 도는 레벨(실제 레이드/디버그 테스트 맵)에서는 클라이언트 자체 추정이 아니라 드레인 타이머 실제 잔여시간(`GetSignalGaugeDrainRemainingSeconds` / `GetSignalGaugeDrainInterval`)을 매 틱 읽어 링을 갱신한다. 판정 기준은 정식 레이드 세션 플래그가 아니라 **현재 게임모드가 `ALSFarmingGameMode`이고 타이머가 도는지**다. 게이지가 10% 단계로만 떨어져도 링은 실제 남은 시간을 그대로 따르며, HUD를 재초기화하는 이벤트(예: 프로토콜 디버그 토글)가 있어도 링이 리셋되지 않는다. 다른 게임모드(로비/결과)거나 타이머가 멈춘 경우(게이지 0%)에는 로비/프리뷰와 동일하게 게이지 구간 위치를 정적으로 표시한다.
  - 더 이상 사라질 칩이 없으면(장착 칩이 없거나 전부 비활성화됨) 칩 아이콘을 숨기고, 프리뷰 데모 카운트다운(`bStartPreviewRingCooldownOnConstruct`)은 끈 뒤 **시간이 다 지난 링(Progress 0)만** 남긴다. 단 링/아이콘 표시 자체는 현재 생존 프로토콜 레벨 ≥ 1(`ShouldShowSignalIndicator`) 게이팅을 따른다.
- 현재 생존 프로토콜 레벨이 0(미해금)이면 신호 유실 원형 링(`SurvivalCooldownRingImage`)과 그 안의 칩 아이콘(`ChipImage`)을 함께 숨긴다. HP/스태미나 바 게이팅(현재 레벨 ≥ 1)과 동일 기준이며, 칩 스테이션 프리뷰·실제 HUD 공용이다. `WBP_ChipStation` 프리뷰는 슬라이더 값을, 실제 HUD는 저장된 `ChipSignalGaugePercent` 값을 기준으로 갱신한다.
- `ULSSurvivalOverheadWidget`은 플레이어 캐릭터 주변 생존 UI를 담당한다. `ALSPlayerCharacter`의 `SurvivalOverheadWidgetClass`에 `WBP_SurvivalOverhead`를 지정하면 `WidgetComponent`가 로컬 표시를 초기화한다.
- 현재 생존 레벨은 신호 게이지로 비활성화된 칩을 제외한 장착 칩의 `Survival` 합산값이고, 이전 생존 레벨은 전체 장착 칩의 `Survival` 합산값이다. 표시 여부는 `DT_Protocol`의 `Protocol_Survival_*` row와 `Protocol_Protected_Level`을 `ULSGameDataSubsystem::IsProtocolUnlockVisible`로 판정한다.
- HP는 `ULSCombatAttributeSet`의 `CurrentHealth/MaxHealth`, 스태미나는 `ULSCharacterAttributeSet`의 `CurrentStamina/MaxStamina`를 ASC attribute delegate로 구독해 `현재/최대` 텍스트와 게이지로 갱신한다. 생존 프로토콜 현재 레벨이 1 이상이면 HP/스태미나 프로그레스바 셸을 표시하되, 스태미나는 1에서 채움값을 0으로 유지해 배경만 표시하고 현재 레벨이 2 이상일 때 실제 스태미나 비율을 반영한다. 달리기/대시 소모와 비전투 회복은 `ULSGE_StaminaChange`가 `CurrentStamina`에 적용한 결과를 그대로 반영한다. 달리기 지속 소모는 남은 스태미나를 0까지 차감한 뒤 달리기 상태를 종료한다.
- 테스트 콘솔 명령 `LSTestSurvivalProtocol <Level>`, `LSTestCarryingProtocol <Level>`, `LSTestBattleProtocol <Level>`, `LSTestNavigationProtocol <Level>`은 로컬 프로토콜 레벨을 임시 오버라이드한다. `LSTestAllProtocols <Survival> <Carrying> <Battle> <Navigation>`은 4종을 한 번에 지정한다. `LSClearProtocolTest`를 실행하면 저장 데이터 기반 계산으로 복귀한다.
- 프로토콜 디버그 패널(Insert)의 **"테스트 맵 가기"** 버튼은 정식 레이드 진입 시퀀스(`ALSLobbyGameMode::StartRaid` — 로드아웃 자동 제출·레이드 인벤토리·`BeginRaidSave`·세션 미러링)를 **그대로 실행하되 목적지만 `ULSSessionSettings::TestRaidLevel`로 바꿔** `ServerTravel`한다(`StartRaidToTestLevel` → `RaidLevelOverride` 1회 소비). 따라서 레이드 인벤토리·세이브·결과 저장(`레이드 종료`→탈출 플로우)까지 실제 레이드와 100% 동일하다. **로비에서만** 동작하므로(진입 셋업이 로비 게임모드 권한에서 이뤄짐) 버튼도 로비에서만 표시된다. 테스트 레벨의 World Settings GameMode는 `FarmingLevel`과 동일하게 `BP_LSFarmingGameMode`로 지정해야 한다. (레벨 레이아웃/스폰/배치는 별도 에디터 작업)
