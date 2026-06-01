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
- **신호 게이지(Signal Gauge)**: 0~100% 스크롤바. 게이지가 10%씩 낮아질 때마다 **칩 번호 순서대로** 영향을 받아 비활성화된다. 비활성화된 칩은 해당 칩의 전투 능력치가 **50%(임시) 상승**한다. (신호 유실 트레이드오프)
- **프로토콜(Protocol)**: 칩이 보유한 4종 시너지 수치. 장착 칩들의 수치를 합산해 레벨/단계를 계산하고, 단계 달성 시 전투지역 편의 UI가 활성화된다.
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
| 데이터 테이블 | `DT_ChipRow`, `DT_ChipStat` (+ Sandbox CSV) | `Content/LostSignal/Data/DataTables/`, `Content/LostSignal/Sandbox/DT/` |
| 설정 참조 | `ChipTable`, `ChipStatTable` 소프트 참조 | `Source/LostSignal/Data/LSDropSettings.h` |
| 보관함 | 칩 탭 필터(`ELSStorageFilter::Chip`) + `WBP_ChipStorage` | `Source/LostSignal/UI/Storage/LSLobbyStorageWidget.*` |
| 아이콘 | 칩 아이콘 경로(`/Game/LostSignal/UI/Icons/Chips/`) | `Source/LostSignal/UI/Inventory/LSItemSlotWidget.cpp:465` |
| 툴팁(부분) | 칩 툴팁: 이름/등급/설명/가격/메모리/스탯 개수 | `Source/LostSignal/UI/Inventory/LSItemTooltipWidget.cpp:197` |

### ⚠️ 부분 구현

- **칩 툴팁** (`PopulateChipTooltip`): 메모리 할당량과 "전투 스탯 개수"는 표시하지만, **실제 전투 스탯 값**(ChipStat 테이블 참조)과 **프로토콜 수치**는 표시하지 않는다.

### ❌ 미구현

| 블록 | 항목 |
|---|---|
| 로직 | 등급 + `Item_Chip_Status_Count` → ChipStat 테이블에서 **실제 전투 스탯 산출**. 데이터만 있고 런타임 로직 없음 |
| [2] | 칩 설정 전용 인벤토리 UI / 칩 오버랩(정보) UI |
| [3] | 하드웨어 UI — 칩 장착 슬롯(10칸), 메모리 한도 표시·검증, 신호 게이지 스크롤바 |
| [3] 로직 | 신호 게이지 10% 단위 → 칩 번호순 비활성화, 비활성 칩 50% 상승 |
| [4] | 소프트웨어 UI — 프로토콜 UI/레벨/단계, 코어 출력 게이지바, 신호 유실 표시, 프로토콜 오버랩 UI |
| [4] 로직 | 프로토콜 수치 합산 → 레벨/단계 계산 → 전투지역 편의 UI 활성화 연동 |

**한 줄 요약**: 데이터 레이어와 보관/툴팁 기초는 되어 있으나, 기획의 핵심인 **칩 장착·신호 게이지·프로토콜 시스템과 그 UI는 전혀 구현되지 않았다.**

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

> **[기획/코드 차이]** 현재 CSV에서 프로토콜 4종 값이 모두 0, `Item_MemoryCost`가 전부 1로 입력되어 있다. 등급/타입별 밸런싱 데이터 입력이 필요하다.

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

1. ~~**스탯 산출 방식**~~: **확정 — 인스턴스 롤링.** 칩 획득 시 범위 내 랜덤값을 굴려 그 칩에 고정(같은 등급도 개체마다 다름). 확정 스탯을 **칩 인스턴스에 저장**해야 함 → 저장 단위(`FLSSessionItem`)에 인스턴스 데이터 필요. 권장: 전체 스탯을 들지 말고 **시드(int32 StatSeed) 1개만 저장 + 런타임 재계산**(저장 구조 최소 변경). → [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md).

2. ~~**스탯 선택 규칙**~~: **확정 — 랜덤 N개.** 등급별 10개 스탯 풀에서 `Item_Chip_Status_Count`개를 무작위 선택하고, 각 값은 해당 스탯의 `min~max`에서 롤링. (선택된 스탯 종류 + 값 모두 시드로 결정론적 재현)

3. ~~**등급 ↔ ChipStat 행 매핑**~~: **해소** — ChipStat 행 Name을 등급명으로 통일(`Supply`~`Masterpiece`). 칩 등급은 Name(`Chip_{Grade}_{Func}`)에서 파싱 → `Item_Grade` 컬럼 제거 예정.

4. **신호 게이지 규칙 수치 확정**: 비활성 칩 상승률 50%(임시), 슬롯 10칸(임시), "10%마다 1칩"이 정확히 어떤 순서·경계로 적용되는지.

5. **프로토콜 레벨/단계 산식**: 합산 수치 → 레벨 변환식, 단계별 활성화 임계값, 활성화 시 전투지역 UI와의 연동 방식.

---

## 5. 단계별 구현 계획

의존성 순서. 각 Phase는 독립적으로 검증 가능하도록 구성한다.

### Phase 1 — 칩 스탯 산출 로직 + 툴팁 보강

**1-A 롤링 본체 (백엔드) — ✅ 완료**
- `FLSSessionItem` / `FLSDropResult`에 `int32 StatSeed` 추가 (0 = 비칩/미롤). 값 복사로 이동·저장·복제 시 보존, `ToSessionItem`/`SetDropResultFromSessionItem`이 시드 전달.
- 드랍 생성(`ULSDropSubsystem::RollDropTable`)에서 칩(`Chip_` 접두사)에 비-0 시드 부여 → **서버 권위 지점에서 1회 롤**.
- 리졸버 `LSChipStats::ResolveChipStats(RowName, StatSeed)` (`Source/LostSignal/Data/LSChipStats.{h,cpp}`):
  - 등급 = Name 파싱, 개수 = `Item_Chip_Status_Count`, 범위 = ChipStat 등급 행(`FLSStatRange`).
  - `FRandomStream(seed)`로 10개 스탯 풀에서 **N개 무작위 선택(Fisher-Yates) + 각 값 범위 내 롤**. 같은 (RowName, Seed)는 항상 동일 결과(결정론적). Seed==0이면 RowName 해시 폴백.
  - 표시용 `GetChipStatLabel(StatKey)` 동봉.

**1-B 툴팁 표시 (UI 배선) — ⬜ 다음 슬라이스**
- 슬롯/툴팁이 `StatSeed`를 받도록 시그니처 확장(`ULSItemSlotWidget::SetItem` → `ULSItemTooltipWidget::SetItem`)하고 호출부 갱신.
- 칩 툴팁이 `ResolveChipStats` 결과(확정 전투 스탯)와 프로토콜 수치를 표시.
- **검증**: 칩 드랍/보관 시 개체별로 다른 스탯이 툴팁에 노출, 같은 칩은 재오픈해도 동일.

### Phase 2 — 칩 장착 데이터 모델 (하드웨어 백엔드)
- 칩 장착 슬롯(10칸 임시) 보유 컴포넌트/구조.
- 메모리 합산·한도 검증 로직.
- 신호 게이지(0~100%) 상태 + 10% 단위 → 칩 번호순 비활성화 규칙 + 비활성 칩 50% 상승.
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

- 데이터: `Source/LostSignal/Data/LSChipRow.h`, `LSChipStatRow.h`, `LSDropSettings.h`
- 테이블: `Content/LostSignal/Data/DataTables/DT_ChipRow.uasset`, `DT_ChipStat.uasset`
- CSV: `Content/LostSignal/Sandbox/DT/DT_Chip.csv`, `DT_ChipStat.csv`
- 툴팁: `Source/LostSignal/UI/Inventory/LSItemTooltipWidget.cpp` (`PopulateChipTooltip`)
- 보관함: `Source/LostSignal/UI/Storage/LSLobbyStorageWidget.cpp`
- 슬롯 아이콘: `Source/LostSignal/UI/Inventory/LSItemSlotWidget.cpp`
- 칩 UI 에셋: `Content/LostSignal/UI/Storage/WBP_ChipStorage.uasset`
