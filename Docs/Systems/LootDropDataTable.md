# Loot Drop / DataTable 파이프라인

## 목적

루팅 오브젝트를 열었을 때 어떤 아이템이 떨어지는지를 결정하는 데이터 파이프라인을 정리한다. 인벤토리 저장/네트워크는 [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md), 슬롯 UI 조작은 [InventoryLogic.md](InventoryLogic.md)를 기준으로 본다.

기획서와 코드가 다른 부분은 **[기획/코드 차이]** 로 표시한다.

---

## 드랍 파이프라인 한눈에

```
RootingObject (Loot_XXX)
  ├─ Looting_Object_Interaction: 일반(0) / 열쇠(1)
  └─ Drop_Table_Name ──┐
                        ▼
DropTable (Drop_XXX_숫자)
  ├─ Drop_Rate: 확률 판정 (0~100)
  ├─ Drop_Amount: 성공 시 개수
  └─ Group_Table_Name ──┐
                         ▼
GroupTable (Group_XXX_숫자)
  ├─ Group_Weight: 누적 가중치 랜덤
  └─ Item_Name ──┐
                  ▼
아이템 테이블 (접두사로 결정)
  Chip_  → ChipTable
  Weapon_ → WeaponTable
  Armor_  → ArmorTable
  Item_   → ItemTable
                  ▼
출력: FLSDropResult { ItemRowName, Amount, ItemText }
```

---

## DataTable Row 카탈로그

### RootingObject (루팅 오브젝트)

파일: `Source/LostSignal/Data/LSRootingObjectRow.h`

| 변수명 | 타입 | 설명 |
|--------|------|------|
| Name (Row Name) | FName | `Loot_` 접두사. 예: `Loot_Chip_Chest`, `Loot_Monster_rat` |
| Loot_Object_Text | FText | 오브젝트 이름 출력용 |
| Looting_Object_Index | int32 | 고유 인덱스 (도감, 정렬) |
| Looting_Object_Interaction | int32 | 0=일반 오픈, 1=열쇠 오픈 |
| Drop_Table_Name | FName | 참조할 DropTable의 Row Name 접두사. 예: `Drop_Chip_Chest` |

### DropTable (드랍 테이블)

파일: `Source/LostSignal/Data/LSDropTableRow.h`

| 변수명 | 타입 | 설명 |
|--------|------|------|
| Name (Row Name) | FName | `Drop_` 접두사 + `_숫자`. 예: `Drop_Chip_Chest_1` |
| Drop_Table_Text | FText | 드랍 테이블 이름 출력용 |
| Drop_Table_Index | int32 | 같은 인덱스 = 같은 그룹 (도감, 정렬) |
| Group_Table_Name | FName | 참조할 GroupTable의 Row Name 접두사. 예: `Group_Chip_Supply` |
| Drop_Rate | float | 이 항목이 발동될 확률 (0~100). 예: 100이면 확정, 0.1이면 0.1% |
| Drop_Amount | int32 | 한번에 드랍되는 아이템 개수 |

### GroupTable (그룹 테이블)

파일: `Source/LostSignal/Data/LSGroupTableRow.h`

| 변수명 | 타입 | 설명 |
|--------|------|------|
| Name (Row Name) | FName | `Group_` 접두사 + `_숫자`. 예: `Group_Chip_Supply_1` |
| Group_Table_Text | FText | 그룹 이름 출력용 |
| Group_Table_Index | int32 | 그룹 인덱스 (도감, 정렬) |
| Item_Name | FName | 드랍할 아이템의 Row Name. 접두사로 테이블 결정 |
| Group_Weight | int32 | 개별 가중치 (누적 가중치 알고리즘) |

고정 드랍 아이템이 있는 경우 뒷 순서 번호로 만든다. 예: `Group_monster_rat_1`(랜덤), `Group_monster_rat_2`(고정)

---

## 아이템 테이블

아이템은 4개 테이블로 나뉘고, GroupTable의 `Item_Name` 접두사로 어떤 테이블인지 결정된다.

### 공통 필드

모든 아이템 테이블이 공유하는 필드:

| 변수명 | 타입 | 설명 |
|--------|------|------|
| Name (Row Name) | FName | 접두사별 테이블 결정 |
| Item_Text | FText | 아이템 이름 출력용 |
| Item_Type | int32 | 분류 코드 (아래 참고) |
| Item_Grade | FString | 등급 이름 (아래 참고) |
| Item_Max | int32 | 인벤토리 1칸 최대 보유량 |
| Item_Description | FText | 아이템 설명 텍스트 |
| Item_Cost | int32 | 상점 판매 가격 |
| Item_Equipment | FString | 장착 슬롯 (타입별로 다름) |

### Item_Type 분류

| 값 | 분류 | 설명 |
|----|------|------|
| 0 | 칩 | 칩 슬롯 장착 |
| 1 | 무기 | 무기 슬롯 장착 (캐릭터별 다름) |
| 2 | 방어구 | 방어구 슬롯 장착 |
| 3 | 일반 아이템 | 판매용 |
| 4~9 | 소모품 | 퀵슬롯 장착, 회복/일회용 |
| 11~19 | 퀘스트 아이템 | 일회성 수집, 인벤토리 합산 안 됨 |
| 20~ | 재료 아이템 | 합성, 강화 등 |

### Item_Grade 등급

| 값 (FString) | 한국어 | 색상 |
|--------------|--------|------|
| Supply | 보급 | 흰색 |
| Standard | 표준 | 초록색 |
| Precision | 정밀 | 파란색 |
| Tuning | 튜닝 | 보라색 |
| Prototype | 프로토타입 | 노란색 |
| Masterpiece | 마스터피스 | 빨간색 |

### Weapon (무기 테이블)

파일: `Source/LostSignal/Data/LSWeaponRow.h`

접두사: `Weapon_`. Item_Type=1.

Item_Equipment 값: `Weapon_1`(첫번째 캐릭터), `Weapon_2`(두번째), `Weapon_3`(세번째)

| 스탯 변수명 | 설명 |
|------------|------|
| Item_Attack | 일반/스킬 데미지 |
| Item_Attack_Speed | 일반 공격 속도 |
| Item_Skill_Haste | 스킬 쿨타임 감소 |
| Item_Critical_Rate | 크리티컬 확률 |
| Item_Critical_Damage | 크리티컬 데미지 상승률 |
| Item_Defense_Penetration | 방어 관통력 |

### Armor (방어구 테이블)

파일: `Source/LostSignal/Data/LSArmorRow.h`

접두사: `Armor_`. Item_Type=2.

Item_Equipment 값: `Processor`(머리), `Core`(몸), `Actuator`(손), `Frame`(발)

| 스탯 변수명 | 설명 |
|------------|------|
| Item_Health | 체력 |
| Item_Defense | 방어력 |
| Item_Recovery | 자연 회복력 |

### Chip (칩 테이블)

파일: `Source/LostSignal/Data/LSChipRow.h`

접두사: `Chip_`. Item_Type=0. Item_Equipment=`chip`.

| 변수명 | 설명 |
|--------|------|
| Item_Chip_Status_Count | 칩이 가진 전투 스탯 개수 |
| Item_MemoryCost | 칩 메모리 비용 |

칩은 등급별로 랜덤 스탯이 붙는다. 스탯 개수와 수치 범위는 ChipStat 테이블에서 결정.

**칩 프로토콜** (기획 시너지 개념):

| 프로토콜 | 변수명 | 관련 시스템 |
|----------|--------|------------|
| 생존 프로토콜 | Chip_Protocol_Survival | HP, 스태미나 |
| 적재 프로토콜 | Chip_Protocol_Carrying | 인벤토리, 보호슬롯 |
| 전투 프로토콜 | Chip_Protocol_Battle | 몬스터 HP, 스킬 슬롯 |
| 탐색 프로토콜 | Chip_Protocol_Navigation | 미니맵 |


### Item (일반 아이템 테이블)

파일: `Source/LostSignal/Data/LSItemRow.h`

접두사: `Item_`. Item_Type=3(판매), 4~9(소모품), 11~19(퀘스트), 20~(재료).

소모품 전용 필드 (Item_Type 4~9):

| 변수명 | 설명 |
|--------|------|
| Item_Target_Status | 영향을 받는 스탯/UI 이름 |
| Item_Target_Status_Value | 스탯 변화 수치 |
| Item_Target_Buff | 상태이상 이름 (회복/디버프) |
| Item_Target_Buff_Value | 상태이상 수치 |
| Item_Cast_Time | 사용까지 걸리는 시간 (초) |
| Item_Duration | 효과 지속 시간 (초) |

### ChipStat (칩 스탯 범위 테이블)

파일: `Source/LostSignal/Data/LSChipStatRow.h`

등급별(Supply~Masterpiece) 각 스탯의 Min/Max 범위를 정의한다. 칩 생성 시 이 범위에서 랜덤 수치가 결정된다.

> 구체적인 스탯별 Min/Max 값과 등급별 스탯 개수(Chip_Setting)는 **`DT_ChipStat` DataTable이 단일 출처**다. 수치가 바뀌면 여기 표가 stale 되므로 문서에 복사하지 않는다. 실제 값은 DataTable을 본다.

---

## Row Name 접두사 규칙

### 테이블 간 참조 (접두사 그룹화)

코드 위치: `LSDropSubsystem.cpp` `ExtractRowNamePrefix`

Row Name의 **마지막 `_숫자`를 떼어내면** 같은 그룹이 된다. DropSubsystem은 로드 시 이 접두사로 `TMap<FName, TArray<Row*>>`를 구성한다.

```
Drop_Chip_Chest_1  →  Drop_Chip_Chest (그룹 키)
Drop_Chip_Chest_2  →  Drop_Chip_Chest
Group_Chip_Supply_1 → Group_Chip_Supply
Group_Chip_Supply_2 → Group_Chip_Supply
```

RootingObject의 `Drop_Table_Name`이 `Drop_Chip_Chest`이면, DropTable에서 `Drop_Chip_Chest_*` 행 전부가 해당 루팅 오브젝트의 드랍 풀이 된다.

### Item_Name → 아이템 테이블 매핑

코드 위치: `LSDropSubsystem.cpp` `FindItemText`

| 접두사 | 대상 테이블 |
|--------|------------|
| `Chip_` | ChipTable |
| `Weapon_` | WeaponTable |
| `Armor_` | ArmorTable |
| `Item_` | ItemTable |

새 카테고리를 추가하려면 `FindItemText`, `LSDropSettings`, `LSInventorySlotUtils`의 정렬 키 3곳을 같이 수정해야 한다.

---

## DropSubsystem 공개 API

파일: `Source/LostSignal/Data/LSDropSubsystem.h`

| 함수 | 입력 | 출력 | 설명 |
|------|------|------|------|
| OpenRootingObject | FName (RootingObject RowName) | TArray\<FLSDropResult\> | 루팅 오브젝트 열기 (진입점) |
| RollDropTable | FName (DropTable 접두사) | TArray\<FLSDropResult\> | 드랍 테이블 전체 롤 |
| RollGroupTable | FName (GroupTable 접두사) | FName | 가중치 랜덤으로 아이템 1개 선택 |
| TestDrop | FName | - | 에디터 디버그용 |

### 드랍 흐름 (의사코드)

```
OpenRootingObject(Loot_Chip_Chest)
  → RootingObjectRow에서 Drop_Table_Name = "Drop_Chip_Chest"
  → RollDropTable("Drop_Chip_Chest")
     → Drop_Chip_Chest_1 ~ _7 각각에 대해:
        Random(0, 100) < Drop_Rate ?
          성공 → RollGroupTable(Group_Table_Name)
          실패 → 스킵
     → RollGroupTable("Group_Chip_Supply")
        → Group_Chip_Supply_1 ~ _5 의 가중치 합 = 50
        → Random(1, 50) = 23
        → 누적: 10, 20, 30... → 23 ≤ 30 → 3번째 선택
        → Item_Name = "Chip_Supply_Minimap"
     → FindItemText로 ChipTable에서 텍스트 조회
     → FLSDropResult 생성
```

### 가중치 알고리즘

1. 같은 그룹(접두사가 동일한 row)의 `Group_Weight`를 전부 더한다
2. `Random(1, TotalWeight)` 롤
3. 첫 번째 행부터 가중치를 누적하며, 누적값이 롤 값 이상이 되는 행이 당첨

예시 (기획서):
```
A=50, B=60, C=120 → 총 230
Roll = 137
A: 50 < 137 → X
B: 50+60=110 < 137 → X
C: 50+60+120=230 ≥ 137 → C 당첨
```

### 예외 처리

- Group_Weight 합이 0 이하: 경고 로그 + NAME_None 반환 → 드랍 실패
- Group_Table_Name 미설정: 경고 로그 + 해당 row 스킵
- ValidateGroupReferences (에디터 전용): DropTable이 참조하는 GroupTable이 실제로 존재하는지 체크

---

## LootBox 연동 (서버/클라 경계)

파일: `Source/LostSignal/Gameplay/LSLootBox.cpp` `Interact_Implementation`

1. **서버만** `DropSubsystem->OpenRootingObject()` 호출 (`HasAuthority()` 체크)
2. 전체 결과는 **서버 전용 `PendingLootResults`(복제 안 함)** 에 보관한다. 복제되는 `LootResults`는 빈 배열로 시작한다.
3. 총 드랍 개수는 **`TotalLootCount`(복제)** 로 즉시 내려간다 — 클라가 미공개 placeholder 슬롯을 그리기 위함이며, 개수만 알 뿐 아이템 정체는 모른다.
4. 클라이언트는 `LSLootDropWidget`에서 `LootResults`(공개분) + `TotalLootCount`(총 개수)로 슬롯을 그린다.
5. `RootingObjectRowName`은 LootBox 액터의 에디터 프로퍼티로 설정

### 단계 공개 연출 (서버 주도)

아이템은 한 번에 다 뜨지 않고 **등급에 따른 시간차를 두고 하나씩** 공개된다. 연출이 아니라 신뢰 경계의 문제라 **서버가 공개를 주도한다.**

- 서버 타이머(`ScheduleNextReveal` → `RevealNextLootItem`)가 `PendingLootResults`에서 한 개씩 꺼내 복제되는 `LootResults`에 append하고, 매번 **다음 아이템 등급의 딜레이로 다음 공개를 재예약**한다(고정 간격 아님).
- 공개 전 아이템 정체는 `LootResults`에 없으므로 **클라로 복제되지 않는다** → 미리보기 불가, 공개 전 선취 불가. (총 개수 `TotalLootCount`만 복제 — 정체 아님.) 공개된(배열에 존재하는) 슬롯은 즉시 줍기 가능, 미공개 placeholder는 클릭/드래그/줍기 불가.
- `NotifyLootResultsChanged` / `OnRep_LootResults`가 모든 PlayerController의 위젯을 갱신하므로 여러 플레이어가 같은 공개 진행을 동시에 본다(서버 단일 타이머).
- 기존 transfer/drop 로직은 전부 `IsValidIndex(LootResults)` 기반이라 배열이 append로 자라도 인덱스가 안전하다. placeholder 인덱스는 `LootResults`에 없으므로 자동 차단된다.
- 타이머는 `EndPlay`에서 정리한다.

등급별 공개 딜레이(초)는 **`ULSDropSettings.GradeRevealDelaySeconds`(프로젝트 설정 > LS Drop Settings)** 가 단일 출처다. 등급은 `LSInventorySlotUtils::ResolveItemGradeFromRowName`로 RowName에서 파싱한다(`FLSDropResult`에 등급 필드 없음). 수치는 여기 복붙하지 않는다.

### 표시/연출 (클라, 전부 C++)

- `LSLootDropWidget::RebuildLootSlots`는 **`max(공개수, TotalLootCount)`개 슬롯 프레임**을 그린다. 공개된 인덱스는 `SetItem`, **바로 다음에 공개될 한 칸만** `ULSItemSlotWidget::SetPlaceholder()`(미확인 아이콘 + 펄스)로 스캔 연출하고, 그 이후 칸은 빈 기본 배경 프레임(`ClearItem`)만 둔다. 즉 동시에 펄스하는 슬롯은 항상 하나뿐이다.
- 등장 연출은 **`ULSItemSlotWidget`의 C++ `NativeTick`** 이 구동한다(BP 타임라인 아님). placeholder는 미확인 아이콘 알파를 sin으로 펄스(슬롯 프레임 배경은 또렷 유지). `SetItem`이 placeholder→아이템 전환을 감지하면 pop-in(RenderScale `PopInStartScale`→1.0 + RenderOpacity 0→1, `InterpEaseInOut`)을 재생. 등장 연출 수치는 슬롯 위젯의 `PopIn*`/`Placeholder*` UPROPERTY로 조정한다. 등급 배경색은 기존 `*GradeColor` 필드 재사용.
- 박스 메시 오픈·발광·오픈 SFX만 3D 에셋/사운드라 `OnLootBoxOpenedVisual()`(BlueprintImplementableEvent, `OnRep_IsOpened` + 호스트 오픈 시점 호출)로 BP가 담당한다. 미확인 아이콘 텍스처(`UnconfirmedIconTexture`)도 WBP 기본값으로 에셋만 매핑한다(로직 아님).
- **등장(공개) 사운드**: 공개 갱신(`RefreshLootItemsFromSource`)에서 슬롯 개수가 늘어난 순간에만, 새로 공개된 아이템의 등급(`ResolveItemGradeFromRowName`)으로 `ULSLootDropWidget::GradeRevealSounds`(등급명 → `USoundBase`, WBP 기본값 매핑)를 찾아 2D로 재생한다(`PlayRevealSoundForNewItems`). 박스 재오픈 시 초기 표시는 `SetLootItems` 직행이라 소리가 나지 않고, 매핑 없는 등급은 `LogLS` Warning만 남긴다. 사운드 에셋은 `Content/LostSignal/Audio/SFX/ItemDrop/drop1~6`(숫자가 클수록 높은 등급, drop1=Supply … drop6=Masterpiece).

---

## DataTable 등록과 로드

### 경로 등록

파일: `Source/LostSignal/Data/LSDropSettings.h`

UE DeveloperSettings(`config=Game`)로 프로젝트 설정 > "LS Drop Settings"에서 등록.

실제 에셋 경로는 `Config/DefaultGame.ini`의 `[/Script/LostSignal.LSDropSettings]` 섹션에 기록된다.

| 설정 | 대상 에셋 |
|------|------------|
| RootingObjectTable | DT_RootingObject |
| DropTable | DT_DropTable |
| GroupTable | DT_GroupTable |
| ChipTable | DT_ChipRow |
| WeaponTable | DT_Weapon |
| ArmorTable | DT_Armor |
| ItemTable | DT_Item |
| ChipStatTable | DT_ChipStat |

### 로드 흐름

`LSDropSubsystem::LoadTables`에서 RootingObject / Drop / Group / Chip / Weapon / Armor / Item **7개** SoftObjectPtr를 `LoadSynchronous()`로 로드한 뒤, `ExtractRowNamePrefix`로 그룹화하여 `DropTableMap` / `GroupTableMap`에 캐싱.

`ChipStatTable`은 `LSDropSettings`에 등록되어 있지만 `LSDropSubsystem`은 로드하지 않는다(칩 스탯 생성 로직 미구현). 칩 스탯 시스템을 붙일 때 로드 대상에 추가해야 한다.

---

## CSV 데이터 워크플로우

### 기본 흐름

1. **기획자**: 엑셀에서 수치 작성 → CSV로 내보내기
2. **프로그래머**: CSV를 언리얼 DataTable로 임포트
3. 런타임에 DataTable 값 로드

### Row 구조체 변경 시 체크리스트

`FLS*Row` 구조체에 필드를 추가/제거하면 CSV도 반드시 같이 수정해야 Reimport가 올바르게 동작한다.

1. `Source/LostSignal/Data/FLS*Row.h` 필드 추가/제거
2. `Content/LostSignal/Sandbox/DT/DT_*.csv` 헤더 컬럼 동기화 (추가 시 기본값 열도 채울 것)
3. 에디터에서 Reimport DataTables

### 파일 위치

| 구분 | 경로 |
|------|------|
| CSV 원본 | `Content/LostSignal/Sandbox/DT/*.csv` |
| DataTable 에셋 | `/Game/LostSignal/Data/DataTables/DT_*.uasset` |

### 에디터 재임포트 도구

**메뉴**: `Tools > LostSignal > Reimport DataTables`

`LostSignalEditorDataToolsModule.cpp`에 등록된 DataTable을 CSV에서 일괄 재임포트.

**Python 에디터 스크립트**: `tools/reimport_datatables.py`

Unreal Editor Python 환경에서 실행하면 스크립트의 `TARGETS`에 등록된 DataTable을 CSV에서 재임포트한다. 누락된 `DT_Protocol` asset은 `FLSProtocolUnlockRow` 구조체로 생성한 뒤 import를 시도한다.

**MCP 명령** (에디터가 켜진 상태에서 깨진 테이블 복구용):

| 명령 | 용도 |
|------|------|
| `fix_datatable_row_struct` | HOTRELOAD로 깨진 RowStruct 복구 |
| `reimport_datatable_from_csv` | CSV에서 안전하게 재임포트 |

에디터를 끄고 reimport 하는 것이 정석이지만, 에디터가 켜진 상태에서 테이블 수정이 막혔을 때 MCP 명령으로 복구할 수 있다.

---

## 예시: 칩 상자 드랍 전체 흐름

```
1. 플레이어가 칩 상자(BP_LootBox, RootingObjectRowName=Loot_Chip_Chest)에 상호작용

2. 서버: OpenRootingObject("Loot_Chip_Chest")
   → Looting_Object_Interaction = 0 (일반 오픈)
   → Drop_Table_Name = "Drop_Chip_Chest"

3. RollDropTable("Drop_Chip_Chest")
   → Drop_Chip_Chest_1: Group_Chip_Supply,    Rate=100  → 확정
   → Drop_Chip_Chest_2: Group_Chip_Standard,  Rate=75   → 75% 확률
   → Drop_Chip_Chest_3: Group_Chip_Precision,  Rate=25   → 25% 확률
   → Drop_Chip_Chest_4: Group_Chip_Tuning,     Rate=5    → 5% 확률
   → Drop_Chip_Chest_5: Group_Chip_Prototype,  Rate=1    → 1% 확률
   → Drop_Chip_Chest_6: Group_Chip_Masterpiece, Rate=0.1 → 0.1% 확률
   → Drop_Chip_Chest_7: Group_Junk,            Rate=100  → 확정, Amount=3

4. 각 성공 항목에 대해 RollGroupTable
   예: Group_Chip_Supply에서 가중치 균등(10:10:10:10:10)
   → 5종 중 1개 랜덤 선택

5. 결과: [Chip_Supply_HP x1, Material_Electricwire x3, ...]
   → LootResults에 Replicated → 클라 UI 표시
```

---

## 주의점

- Row Name 끝의 `_숫자`는 그룹화에 쓰이므로, 단일 row만 있는 그룹도 `_1`을 붙인다
- 새 아이템 카테고리 추가 시 `FindItemText` + `LSDropSettings` + 정렬 키(`LSInventorySlotUtils`) 3곳 수정
- 가중치 합이 0이면 드랍이 조용히 실패한다 — CSV에서 Group_Weight를 비워두면 발생
- `LSCharacterStatRow`와 `LSMonsterArchetypeRow`는 드랍 시스템과 무관 (각각 캐릭터 스탯, AI 감지용)
- 소모품 시스템은 기획 진행 중이며 현재 테이블에는 예시 데이터만 있다
