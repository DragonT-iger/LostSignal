# 캐릭터 강화 노드 시스템 (토폴로직형)

## 목적과 범위

코어 중심 방사형 노드 그래프로 캐릭터를 심화 성장시키는 시스템의 구조 문서다. 기획 출처는 `Lost_Signal 캐릭터 강화 시스템.md`(토폴로직형 기획서)이며, 이 문서는 그 기획을 현재 코드베이스에 앉히는 **구조 결정과 그 근거**를 소유한다.

**이 문서가 소유하는 것:** 노드 그래프 정의, 링 해금 조건, 노드 활성화 비용과 소비 경로, 노드 진행 저장, 노드 해석(resolve) 계층, 노드 UI.

**이 문서가 소유하지 않는 것:** 스킬 런타임 실행(발동·쿨타임·GAS·프리뷰)은 [SkillSystemStructure.md](SkillSystemStructure.md)가 계속 소유한다. 칩 데이터·신호 게이지·프로토콜은 [ChipSystem.md](ChipSystem.md), 아이템 저장 구조는 [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md), 재료 소비 원자성 규칙은 [CraftingSystem.md](CraftingSystem.md)가 소유한다. 여기서는 링크로만 참조하고 값을 복붙하지 않는다.

**현재 구현 상태: 미구현(그린필드).** 노드 그래프 관련 C++·DataTable·위젯이 하나도 없다. 있는 것은 스킬 진화에 재사용할 런타임 뼈대(`ULSSkillDataAsset::EnhancementVariants`, 호출자 없는 `ULSPlayerSkillComponent::ApplySkillEnhancementByIndex`)와 진화용 DataAsset·아이콘 에셋뿐이다.

---

## 용어 정리 (기획 용어 ↔ 코드 용어)

기획서와 기존 코드가 "강화"라는 말을 서로 다른 의미로 쓴다. 혼동 지점이므로 먼저 고정한다.

| 기획 용어 | 의미 | 코드에서의 대응 |
|---|---|---|
| **스킬 강화** (육각형 노드) | 스킬의 계수·범위·쿨타임·타수를 **수치로** 상향 | **신규 경로 — DataTable row 값에 델타 적용** |
| **스킬 진화** (마름모 노드) | 스킬의 **작동 구조**를 변경. 동일 스킬은 1개만 활성(배타) | 기존 `EnhancementVariants` **Skill_ID 스왑** |

`SkillSystemStructure.md`의 "강화 스킬 구조" 섹션이 다루는 것은 기획 용어로는 **스킬 진화**다. 스킬 강화(수치)는 그 문서에 경로가 없다.

기획서 나머지 용어(코어 / 노드 / 핵심 노드 / 링 / 분기)는 기획서 정의를 그대로 쓴다.

---

## 먼저 확인할 것 — 기획서가 전제하지만 코드에 없는 것 3개

아래 셋은 기획서가 전제하지만 코드에 대응이 없는 항목이다. 1번(링 해금)은 **추후 개발 예정으로 확정**해 이번 범위에서 뺐고, 2·3번은 구현 시 우회 또는 추가 작업이 필요하다. 셋 다 그래프 연결·노드 해석 구조를 바꾸지는 않는다.

### 1. 링 해금 조건 — 추후 개발 예정 (퀘스트 시스템이 없다)

기획: *"링의 해금 조건은 특정 퀘스트 클리어를 기준으로 한다"*

퀘스트나 시나리오 진행도를 **기록하거나 판정하는 시스템이 코드·데이터 어디에도 없다.** 존재하는 것은 UI 껍데기 둘뿐이며, 두 헤더 모두 "데이터 소스 연결은 추후 작업하고 현재는 setter만 열어 둔다"고 주석에 적혀 있다.

- `Source/LostSignal/UI/Lobby/LSLobbyQuestWidget.h` — 메인 1 + 서브 3 배치와 접기/펴기 토글만
- `Source/LostSignal/UI/Lobby/LSQuestInfoWidget.h` — TextBlock 2개에 FText를 꽂는 것뿐

`ULSSaveGame`에 진행도·챕터·플래그·레벨·경험치 필드가 하나도 없고, `DT_Quest` 계열 테이블도 없다.

**결정: 링 해금은 추후로 미룬다.** 조건을 정할 근거(퀘스트 시스템)가 없고, 그래프 연결과 노드 해석은 해금 조건에 의존하지 않으므로 지금 막을 이유가 없다.

- 해석 계층은 `RingUnlocked(Ring)`을 **항상 `true`를 반환하는 자리표시자**로 둔다. 조건이 정해지면 그 함수 하나만 채운다.
- `Ring_Unlock_Type` / `Ring_Unlock_Value` 컬럼과 `DT_SkillNodeRing` 테이블은 **미정**이다. 지금 만들지 않는다.
- **`Ring` 컬럼 자체는 만든다.** 해금 판정과 무관하게 세 용도가 있다 — (a) UI 방사형 링 배치 그룹핑, (b) Ring 역행 데이터 검증(§노드 간 연결 구조), (c) 비용 등급 매핑(기획: 1차 링 = 최하급 칩).

> 프로토콜 레벨로 대체하는 안은 비권장이다. 판정 API(`ULSGameDataSubsystem::IsProtocolUnlockVisible` 등)가 완비돼 있어 기술적으로는 쉽지만, 프로토콜 레벨은 **장착 칩 합산**에서 나오는 값이라 칩을 뺐다 끼우면 링이 닫힌다. 기획의 "링 = 시나리오 진행도와 해금 순서" 의도와 성질이 다르다.

### 2. 칩을 재료로 소모하는 경로가 없다 — 기존 함수가 칩을 조용히 무시한다

기획: *"특정 등급의 칩(1차 링 = 최하급 칩, 2차 링 = 중급 칩 등)"* 소모

`LSInventorySlotUtils::RemoveItemsFromSlotArray`(`LSInventorySlotUtils.cpp:586`)가 `Slot.ChipStats.Num() > 0`인 슬롯을 **스킵한다.** 칩은 전부 인스턴스 스탯을 보유하므로(획득 시 1회 롤링, `Item_Max=1`), 이 함수에 칩 RowName을 넘기면 **아무것도 지우지 않고 성공한 것처럼 보인다.** 반환값도 없어서 실패를 알 수 없다.

제작 시스템도 이 사실을 알고 있어 `LSCraftingUtils.cpp:18`에서 `Chip_` 접두 재료를 레시피 무효로 거부한다.

즉 **"RowName + N개" 재료 모델이 칩에는 성립하지 않는다.** 칩마다 인스턴스 스탯이 달라 서로 대체 가능한 수량이 아니기 때문이다.

**해결:** 기획서 UI 문구가 답을 준다 — *"필요 재료 슬롯 클릭 시 슬롯 좌측에 칩 인벤토리 노출, 드래그 앤 드랍 혹은 더블클릭으로 장착"*. 이는 **플레이어가 칩을 지정해 투입하는 모델**이지, 등급 조건에 맞는 칩을 자동으로 골라 차감하는 모델이 아니다. 따라서 소비는 슬롯 인덱스 지정으로 `ULSSaveSubsystem::ClearStoredSlot(Area, Index)`을 쓰면 되고, 인스턴스 스탯 문제가 자연히 사라진다.

**칩 등급은 UPROPERTY 필드가 아니라 Row Name 토큰이다.** `Chip_{Grade}_{Func}` 형식이며, 파싱과 서열의 단일 출처는 `LSInventorySlotUtils::ResolveItemGradeFromRowName` + `GetKnownGrades()`다. 등급 목록과 서열은 그 함수가 소유하므로 이 문서에 나열하지 않는다. `FLSChipRow`에 Grade/Tier/Rarity 필드는 없다.

**"코인"은 `ULSSaveGame::Gold`다.** 게임 내 유일한 통화이며 `Coin`/`Credit`/`Currency` 이름의 필드는 없다. 차감 API는 `ULSSaveSubsystem::TrySpendGold`.

### 3. "타수 강화"를 적용할 코드 표면이 하나뿐이다

기획: 스킬 강화 노드 = *"스킬 계수, 범위, 쿨타임, 타수 강화"*

`Skill_HitCount`/`Skill_HitRate`를 읽는 곳은 **`ALSShortCircuitField`(`LSShortCircuitField.cpp:292-293`) 단 하나**다. Override·Overclock·Execution·Bypass는 전부 단발 판정이라 타수를 늘릴 코드 지점이 존재하지 않는다.

→ **계수·범위·쿨타임 강화는 전 스킬에 적용 가능, 타수 강화는 현재 ShortCircuit 전용이다.** 다른 스킬에 타수 노드를 붙이려면 해당 Ability에 다단 히트 로직을 새로 만들어야 한다(이 시스템의 범위 밖).

---

## 전체 구조 — 5계층

```text
① 정적 정의   DT_SkillNode (FLSSkillNodeRow)
                  ↓
② 세이브      ULSSaveGame::NodeProgressByCharacter { ActivatedNodeIDs }
                  ↓
③ 순수 해석   LSSkillNodeResolve::Resolve() → FLSCharacterNodeTotals   ← 플랫 스냅샷
                  ↓
④ 적용        (a) 스탯   → 어트리뷰트 무한 GE
              (b) 스킬강화 → row 델타 (리졸버 훅)
              (c) 스킬진화 → Skill_ID 스왑
                  ↓
⑤ UI          노드 그래프 위젯 + 칩 투입
```

이 형태는 **칩/프로토콜 시스템이 이미 쓰는 패턴의 복제**다 — `장착 상태 → LSChipStats 순수 집계 → FLSChipProtocolTotals → ULSChipStatComponent 서버 권한 GAS 적용`. 새 패턴을 발명하지 않으므로 테스트 방식(`Tests/LSProtocolUnlockTests.cpp` 형식의 순수 함수 자동화 테스트)까지 그대로 따라온다.

### 핵심 규칙: 런타임은 그래프를 읽지 않는다

전투 코드(`ULSPlayerSkillComponent`, `ULSGA_PlayerSkillBase` 파생)가 노드 테이블을 직접 조회하지 않는다. 그래프는 **로드 시점과 노드 활성화 시점에 한 번** 해석해 플랫 스냅샷(`FLSCharacterNodeTotals`)으로 만들고, 런타임은 스냅샷만 소비한다.

이유:

- **복제가 자동으로 해결된다.** 강화 상태가 클라와 서버에서 어긋나면 프리뷰 범위·쿨타임 숫자·예측 대시 거리가 서버 판정과 불일치한다(§적용 (b) 참고 — 같은 리졸버를 클라 표시와 서버 판정이 공유한다). 스냅샷 계층이 있으면 서버가 세이브에서 해석한 결과 구조체 **하나만** 복제하면 되고, 그래프 전체를 보내지 않으므로 대역폭도 작다.
- 세이브·노드 UI는 로비에, 전투는 레이드에 있다([RaidLevelFlow.md](RaidLevelFlow.md)). 계층을 섞으면 레벨 경계를 넘는 의존이 생긴다.
- 기획자가 트리를 바꿀 때 전투 경로가 깨지지 않는다.
- ③이 순수 함수라 단위 테스트가 붙는다.

---

## 노드 타입 → 적용 경로

기획의 노드 타입 4종은 적용 성질로 셋으로 갈린다.

| 기획 노드 타입 | 표현 | 적용 경로 | 기존 코드 재사용 |
|---|---|---|---|
| 메인 스탯 | 큰 원 | 어트리뷰트 무한 GE | `ULSChipStatComponent` 패턴 |
| 서브 스탯 | 작은 원 | 어트리뷰트 무한 GE | 위와 동일 |
| **스킬 강화** | 육각형 | **row 수치 델타** | **없음 — 신규** |
| 스킬 진화 | 마름모 | Skill_ID 스왑 | `EnhancementVariants` 그대로 |

메인/서브 스탯의 차이는 수치 크기와 비용뿐이라 코드 경로가 같다. 기획의 *"메인 스탯 수치는 서브스탯 3~4개 분"*은 DataTable 값으로 표현하며 이 문서에 수치를 적지 않는다.

---

## 핵심 결정: 스킬 강화는 row 스왑이 아니라 row 델타 훅

### 스킬 진화는 기존 스왑으로 충분하다

기획의 *"동일한 스킬은 1개만 활성화 가능하며, 중복 활성화가 불가능"*이 `EnhancementVariants`의 배타 스왑 계약과 정확히 일치한다. 진화 DataAsset이 자기 `Skill_ID`를 가지므로 스왑 즉시 다른 row가 해석된다. **신규 메커니즘이 0이다.**

### 스킬 강화는 스왑으로 표현할 수 없다

링 경로를 따라 여러 강화 노드를 지나가므로 효과가 **누적**된다. 스왑 방식은 조합마다 DataTable row + DataAsset이 필요해 조합 폭발(2ⁿ)이 난다. 따라서 row 수치를 가로채 델타를 적용해야 한다.

### 이론적 단일 관문은 쓸 수 없다

모든 액티브 스킬 row 읽기는 `ULSGameDataSubsystem::FindActiveSkillRowByID` 하나를 거친다(블루프린트에서 `DT_ActiveSkill`을 직접 참조하는 곳도 없다). 그런데 여기를 가로챌 수 없는 이유가 셋이다.

1. **`const FLSCharacterSkillRow*`를 반환한다** — DataTable 내부 메모리를 가리키므로 수정한 사본을 돌려줄 수 없다(수명 문제).
2. **GameInstance 서브시스템이라 플레이어 컨텍스트가 없다** — 노드 강화는 캐릭터·세이브별인데 이 계층은 소유자를 모른다.
3. **row를 직접 mutate하면** 전역 오염 + 에디터 에셋 dirty + 멀티에서 플레이어별 차등 불가다.

### 실용 훅은 3곳

| 훅 | 위치 | 커버 범위 |
|---|---|---|
| **(가) 주 훅** | `ULSPlayerSkillComponent::ResolveActiveSkillRow` (`LSPlayerSkillComponent.cpp:691`) | 서버 스냅샷 생성(`:654` → **모든 Ability 자동**), 프리뷰 범위(`:707`), 쿨타임 표시+GE(`:724`), 사거리 클램프(`:747`), 클라 예측(`:960`/`:966`) — 이 래퍼의 전 호출지점 |
| (나) | `ULSGA_Bypass::PullTargetsToHologram` (`LSGA_Bypass.cpp:377`, row 조회 `:391`) | `static` + 지연 타이머 람다(`:253`)라 컨텍스트 스냅샷을 못 쓰고 Subsystem을 재조회. `SourceActor`가 있으므로 (가)로 리다이렉트 |
| (다) | `ALSShortCircuitField` (row 조회 `LSShortCircuitField.cpp:289`, `:339`) | 스폰된 액터가 독자 재조회. **`Skill_HitCount`/`Skill_HitRate`의 유일한 소비자.** `SourceActor`가 있으므로 (가)로 리다이렉트 |

**(가)를 `const*` 반환에서 값 반환 리졸버로 바꾸고 델타를 적용하면, 서버 판정·클라 표시·클라 예측이 한 곳에서 일관되게 강화된다.** (나)(다)는 이 리졸버로 리다이렉트한다.

UI 위젯(`ULSSkillLoadoutEntryWidget`, `ULSSkillLoadoutWidget`)은 이름·설명·타입만 읽고 수치를 읽지 않으므로 일관성에 무해하다. 단 툴팁에 상향된 수치를 보여주려면 네 번째 훅이 필요하다.

---

## 함정 (반드시 지킬 계약)

### 1. `> 0.0f` 게이트 — 0값 함정

**모든 소비 지점이 row 값이 0이면 DataAsset·Ability 폴백으로 넘어간다.** 예: `ResolveSkillCooldownDuration`은 `Row->Skill_Cooldown > 0.0f`일 때만 row를 쓰고, `ALSShortCircuitField`도 `Skill_HitCount > 0`일 때만 쓴다.

따라서 테이블 값이 0인 필드를 **배수**로 강화하면 결과가 여전히 0이라 폴백이 이기고 **강화가 조용히 무효**가 된다.

→ **배수가 아니라 가산 델타를 쓴다.** 그리고 강화 대상 필드는 테이블에 0이 아닌 값을 넣는 것을 기획자 계약으로 둔다. 리졸버에서 "델타는 있는데 row 값이 0"인 상황을 만나면 `UE_LOG(LogLS, Warning)`으로 반드시 남긴다 — 조용히 무효가 되는 것이 이 시스템에서 가장 찾기 어려운 버그다.

### 2. `CooldownReduction` 중첩 순서

`ULSPlayerSkillComponent::ResolveReducedSkillCooldownDuration`이 row 쿨타임 **뒤에** 어트리뷰트를 곱한다. 노드 쿨타임 델타를 그 앞(row 값)에 가산하고 그 뒤 감소율을 곱하는 순서를 쓴다 — 감소율이 강화된 쿨타임에도 비례 적용되는 쪽이 기대에 맞다.

### 3. 시간적 의미 불일치

`ULSGA_Execution`은 발동 시 row를 값복사해 잠그고, `ALSShortCircuitField`는 **펄스마다 재조회**한다(`:339`). 필드가 떠 있는 중에 강화 상태가 바뀌면 이미 뜬 필드의 계수가 소급 변경된다. 노드 조작이 로비 전용이라 실전에서는 발생하지 않지만, 필드 스폰 시 스냅샷을 잠그는 쪽이 안전하다.

### 4. 훅으로 도달할 수 없는 값

강화 노드를 만들어도 다음 값은 바뀌지 않는다. 기획에 이 제약을 알려야 한다.

- **Execution 대시 시간** — `Skill_Time`을 의도적으로 무시하고 `ULSExecutionSkillDataAsset::FallbackDashDuration`만 쓴다.
- **대쉬 쿨타임** — row가 아니라 `DashCooldown` 어트리뷰트 경유다([SkillSystemStructure.md](SkillSystemStructure.md) 쿨타임 구조 참고).
- **ShortCircuit 투사체 비행/아크, 필드 기본 Duration/Interval** — DataAsset이 단일 출처다.

---

## ① 정적 정의

### `FLSSkillNodeRow` / `DT_SkillNode`

```text
Node Key          노드 고유 키 — 타입 미정 (§보류 결정 1)
Char_ID           캐릭터 식별자 (ULSSkillPoolDataAsset::CharacterID와 같은 키)
Ring              소속 링 (0=코어). 해금 판정에는 아직 쓰지 않는다 (§먼저 확인할 것 1)
Prereq            선행 노드 키 배열 — ANY 시맨틱. 연결선 정의도 겸한다
Node_Kind         None / MainStat / SubStat / SkillEnhance / SkillEvolve
bKeyNode          기획의 "핵심 노드" — UI 강조 전용, 해석에 영향 없음
Target_Stat       스탯 노드 전용 — 기획 스탯 어휘(Char_*)
Math_Type         스탯 노드 전용 — Flat / Percent
Target_Skill      스킬 강화·진화 전용 — 기본 Skill_ID
Target_Field      스킬 강화 전용 — None / Multiplier / Range / Cooldown / HitCount
Delta_Value       변화량 (가산). 스탯·스킬강화 전용
Evolve_Skill_ID   스킬 진화 전용 — 교체될 진화 Skill_ID. 배타 그룹은 Target_Skill이 겸한다
Cost_Chip_Grade   요구 칩 등급 (ResolveItemGradeFromRowName이 반환하는 토큰)
Cost_Chip_Count   요구 칩 개수
Cost_Gold         요구 코인
Pos_X / Pos_Y     방사형 표시 좌표
```

**Kind마다 쓰는 컬럼이 다르다.** 한 row가 위 컬럼을 전부 쓰지 않으므로 Kind별 검증이 필요하다. 상세는 §노드 타입별 구현 구조.

**`Prereq`는 배열이고 ANY 시맨틱이다.** 기획의 *"하위 노드가 2개인 상위 노드라면 1개만 활성화 되어도 활성화 가능"*이 이 의미다. 방사형이므로 구조는 트리가 아니라 **DAG**이며, 단일 `Parent_Node_ID`로는 표현할 수 없다. 상세는 §노드 간 연결 구조.

> ⚠️ `Prereq`를 **`FString`으로 만들지 않는다.** `FLSCharacterSkillRow::Skill_Effects`와 `FLSComboAttackRow::Combo_Effects`가 `FString`에 배열 리터럴을 담았는데 파싱하는 코드가 없어 죽은 데이터가 된 선례가 있다. 반드시 진짜 `TArray`로 만든다 — `FLSCharacterPassiveSkillRow::Skill_Effects`가 올바른 선례다.

**새 테이블로 분리한다.** `DT_ActiveSkill`은 "스킬 수치"의 단일 출처이고 노드 그래프는 "진행 구조"다. 스킬 row가 없는 스탯 노드, 링·좌표·비용을 스킬 테이블에 끼우면 컬럼이 계속 붙는다. 기존 `FLSCharacterSkillRow::Parent_Skill_ID`는 **스킬 계보 표기로만 남기고 그래프 판정에는 쓰지 않는다**(진화 대상은 `Evolve_Skill_ID`가 명시한다).

**테이블 참조는 `ULSGameDataSettings`에 추가한다.** `ULSGameDataSubsystem::LoadTables`(`LSGameDataSubsystem.cpp:387`)는 `ULSGameDataSettings`만 읽는다(`:389`). `ULSDropSettings`는 `ULSDropSubsystem`·`ULSSaveSubsystem`·`ULSCraftingWidget`이 `GetDefault<>()`로 직접 읽는 별개 계통이므로, 거기에 넣으면 서브시스템이 처음으로 두 설정 클래스를 교차하고 기존 "한 설정 클래스 → 한 서브시스템" 패턴이 깨진다.

**로드와 미설정 경고를 쌍으로 등록한다.** `LogMissingTables()`(`LSGameDataSubsystem.cpp:412`)에 노드 테이블 경고를 함께 넣는다. `ComboAttackTable`이 로드는 되는데 경고 목록에서 빠져 있는 선례가 있다 — 같은 누락을 반복하지 않기 위한 기록이다.

조회 API는 `ULSGameDataSubsystem`에 `FindSkillNodeRow` / `GetSkillNodeRowsForCharacter`로 붙인다. Subsystem은 조회만 하고 GE를 적용하지 않는다(기존 책임 분리 유지).

### 링 해금 테이블 — 만들지 않는다 (추후)

`DT_SkillNodeRing`과 `Ring_Unlock_*` 컬럼은 **미정**이다. §먼저 확인할 것 1 참고.

---

## 노드 간 연결 구조

### 식별자에 의존하지 않는다

노드 키의 구체 타입은 보류 상태(§보류 결정 1)이므로, 이 절의 판정 로직은 "노드 키"로만 쓴다. 연결 판정은 키가 `FName`이든 `int32`든 동일하다.

### 선행 조건은 ANY다

```text
CanActivateNode(N) =
      RingUnlocked(N.Ring)                        // 현재 항상 true (추후)
   && (N.Prereq 가 비어 있음
       || N.Prereq 중 하나라도 Activated 에 있음)     ← ANY
   && 배타 충돌 없음                                 // 스킬 진화만 (§노드 타입별 구현 구조)
   && 비용 충족                                     // 칩 등급·개수 + 코인
```

기획의 *"하위 노드가 활성화 되어야 상위노드 활성화 가능"* + *"하위 노드가 2개인 상위 노드라면 1개만 활성화 되어도 활성화 가능"*이 정확히 ANY다. ALL이 아니다.

**코어는 `Prereq`가 비어 있는 유일한 노드**이며 시스템 활성 시 자동 활성이다(기획). 즉 "Prereq 비어 있음"은 코어를 위한 예외가 아니라 코어의 정의다.

### 코어 연결성 불변식은 자동으로 유지된다

기획의 *"모든 활성화되어있는 노드는 코어까지 연결되어있어야한다"*는 **활성화 경로만 존재하는 한 귀납적으로 성립한다.**

- 코어는 항상 활성이고 코어까지의 경로를 자명하게 갖는다.
- 노드 N을 활성화하는 시점에 `N.Prereq` 중 최소 하나(P)가 이미 활성이다(ANY 조건). P는 귀납 가정상 코어까지 경로를 가지므로, N도 P를 거쳐 경로를 갖는다.

→ **따라서 `ValidateCoreConnectivity`는 평시 활성화 판정에 쓰지 않는다.** 이 함수의 역할은 **세이브 로드 시 1회 무결성 검사**다. 방어 대상은 세 가지다.

1. 세이브 손상
2. 테이블 변경으로 활성 노드가 사라졌거나 `Prereq`가 바뀐 경우
3. 리스펙(해제)이 도입된 경우

매 프레임·매 노드 판정에 그래프 순회를 돌리지 않는다. 성능 오해를 막기 위해 이 성격을 명시한다.

**리스펙이 들어오면 불변식이 깨질 수 있다.** 중간 노드를 해제하면 그 뒤 노드들이 고아가 된다. 연쇄 해제인지 해제 거부인지 정책이 필요하다 → 기획 확인 항목 참고.

### 데이터 무결성 검증 — 선례 0건, 전부 신규

프로젝트에 사이클 검출·도달성 검사 코드가 **하나도 없다**(`cycle`/`circular`/`acyclic`/`DFS` 전수 확인). 그래프 검증은 재사용할 것이 없으므로 전부 새로 쓴다.

검사 항목:

| 검사 | 내용 |
|---|---|
| 미존재 선행 | `Prereq`가 테이블에 없는 노드를 가리킨다 |
| 사이클 | 선행 관계를 따라가면 자기 자신에 도달한다 |
| 섬 | 코어에서 `Prereq` 역방향으로 도달할 수 없는 노드가 있다 |
| Ring 역행 | 상위 링 노드가 상위/동일 링만 선행으로 갖는다 (방사형 전제 위반) |

방식은 `ULSDropSubsystem::ValidateGroupReferences`(`LSDropSubsystem.cpp:97`, 선언·호출 양쪽 `#if WITH_EDITOR` 가드) 패턴을 따른다 — **경고 로그만 남기고 동작은 바꾸지 않는다.** 프로젝트에 `check()`/`ensure()`/`UE_LOG(Fatal)` 사용례가 0건이므로 여기서도 쓰지 않는다.

런타임(패키지) 쪽은 그래프 순회 검증을 돌리지 않고, 개별 노드 로드 시 skip-and-warn으로 방어한다(§노드 타입별 구현 구조).

### 연결선은 선행 관계가 겸한다

`Prereq`가 곧 연결선 정의다 — 인접 링 간 연결선 = 선행 관계. 같은 링 안의 장식 연결선은 지금 두지 않고, 필요해지면 컬럼을 추가한다.

---

## 노드 타입별 구현 구조

### 한 row가 모든 컬럼을 쓰지 않는다

이게 이 절의 핵심이다. Kind별로 의미 있는 컬럼이 다르므로 **Kind별 검증**이 필요하다.

| Kind | 사용 컬럼 | 해석 결과 |
|---|---|---|
| MainStat / SubStat | `Target_Stat`, `Math_Type`, `Delta_Value` | `StatModifiers`에 누적 |
| SkillEnhance | `Target_Skill`, `Target_Field`, `Delta_Value` | `SkillDeltas[Target_Skill]`에 누적 |
| SkillEvolve | `Target_Skill`, `Evolve_Skill_ID` | `Evolutions[Target_Skill]` 설정 |

**MainStat과 SubStat은 해석·적용 경로가 완전히 같다.** 차이는 수치 크기·비용·UI 표현(큰 원 / 작은 원)뿐이다. Kind를 둘로 나누는 이유는 UI가 도형을 구분해야 하기 때문이며, 해석 분기는 하나로 합친다.

기획의 **"핵심 노드"**(빌드에 영향이 큰 중요 노드)는 별도 Kind가 아니라 **표시 강조 플래그**(`bKeyNode`)다. 해석에 영향을 주지 않는다.

### Kind별 로드 검증 (skip-and-warn)

`ULSCraftingWidget::LoadRecipes`(`LSCraftingWidget.cpp:139`) 패턴을 따른다 — **잘못된 row는 전체 로드를 실패시키지 않고 그 row만 버리고 경고한다.**

- `Kind == None` → skip
- MainStat/SubStat인데 `Target_Stat`이 비었거나 어트리뷰트로 매핑 불가 → skip + warn
- SkillEnhance인데 `Target_Field == None` 또는 `Delta_Value`가 0 → skip + warn
- SkillEvolve인데 `Evolve_Skill_ID`가 없음 → skip + warn
- SkillEnhance/SkillEvolve인데 `Target_Skill`이 `DT_ActiveSkill`에 없음 → skip + warn
- 스탯 노드의 `Delta_Value`가 음수이거나 비정상적으로 큰 경우 → skip + warn (어트리뷰트에 클램프가 없어 그대로 통과하므로 여기서 막는다)

### 스탯 노드 (MainStat / SubStat)

**해석은 확정이다.** `StatModifiers`에 `{Target_Stat, Math_Type, 합산 Delta}`를 누적하고, 같은 `(Target_Stat, Math_Type)` 조합은 합산한다. `Math_Type`은 기존 `ELSStatusEffectMathType`(`Flat`/`Percent`)을 재사용할 수 있다.

**GAS 적용 방식은 보류다**(§보류 결정 2). 어느 쪽을 택해도 부딪히는 제약 5개:

| 제약 | 내용 |
|---|---|
| 매핑 커버리지가 4개뿐 | `ULSStatusEffectComponent::ResolveAttribute`(`LSStatusEffectComponent.cpp:45`)가 처리하는 것은 Attack / AttackSpeed / MoveSpeed / Defence **4개**다. 체력·치명타·회복·스킬가속은 어트리뷰트가 존재하는데도 매핑에 없어 **확장이 필수**다 |
| Percent 수식이 두 갈래 | StatusEffect는 곱연산(`Multiplicitive`, 1+f), 칩·장비 스탯은 **가산**(`Additive`, ÷100). 같은 "퍼센트"가 두 수식으로 구현돼 있다. 노드는 **칩·장비 쪽(가산)** 을 따르는 것을 권장 — 같은 "로비에서 정한 영구 스탯"이므로 |
| MaxHealth 함정 | 최대 체력을 올리는 노드는 현재 체력 보존·클램프 로직이 필요하다. 선례가 둘 있다 — `ULSChipStatComponent`(`.cpp:110`~`:125`), `ULSEquipmentStatComponent`(`.cpp:99`~`:111`). 없으면 노드 적용이 회복 수단이 되거나 현재 체력이 최대치를 넘는다 |
| 클램프 없음 | 스태미나 외 어트리뷰트에 클램프가 전혀 없다. 음·수과대값이 그대로 통과하므로 로드 검증에서 막아야 한다 |
| 대부분 비복제 | `Attack`/`Defence`/`MoveSpeed` 등은 `DOREPLIFETIME`에 없다(스태미나·체력만 복제). 노드 UI에 "현재 공격력 +15"를 띄우려면 어트리뷰트 복제를 추가하거나 세이브에서 직접 계산해야 한다 |

**적용 순서**: `ALSPlayerCharacter::BeginPlay`의 `InitializeBaseAttributes`(`LSPlayerCharacter.cpp:131`) → `RefreshChipStats`(`:136`) → `RefreshEquipmentStats`(`:142`) 체인에 끼운다. 노드는 영구 스탯이므로 **칩보다 앞**이 논리적이다. (컴포넌트 자체 `BeginPlay`에서는 못 한다 — ASC `InitAbilityActorInfo` 완료 후여야 한다.)

### 스킬 강화 (SkillEnhance)

`Target_Field` enum:

```text
None / Multiplier / Range / Cooldown / HitCount
```

**`Range`의 의미를 정의한다.** 실제 판정·프리뷰에서 쓰이는 것은 `Range_X`(반경·길이)와 `Range_Y`(각도·폭)이고, `Range_Z`는 ShortCircuit 투사체 아크 높이 전용이다. → **`Range`는 X·Y를 비례 확대하고 Z는 건드리지 않는다.** 기획의 "범위 강화" 의도에 맞다.

**누적**: 같은 `(Target_Skill, Target_Field)`는 합산한다.

`HitCount`는 **ShortCircuit 스킬에만 적용된다** — §먼저 확인할 것 3 참고. 다른 스킬에 HitCount 노드를 만들면 로드는 되지만 효과가 없으므로, 기획 데이터 단계에서 막아야 한다.

가산 규칙과 0값 함정은 §함정 1을 따른다.

### 스킬 진화 (SkillEvolve)

**`Target_Skill`이 곧 배타 그룹이다.** 같은 `Target_Skill`을 가진 SkillEvolve 노드 중 1개만 활성 가능하다(기획: *"동일한 스킬은 1개만 활성화 가능하며, 중복 활성화가 불가능"*). 따라서 **별도 `Exclusive_Group` 컬럼이 필요 없다.**

기획서의 **"분기"** 정의(*"둘 이상의 노드 중 선택하는 지점 — 능력치, 스킬 특화 선택 유도"*)가 스탯에도 하드 배타를 요구하는지는 애매하다. **현재 해석: 하드 배타는 스킬 진화뿐이고, 나머지 분기는 재화 제약에 의한 사실상의 선택이다.** 기획서가 배타를 명시한 곳은 진화 항목 하나뿐이다. 스탯에도 하드 배타가 필요하면 `Exclusive_Group` 컬럼을 되살린다 → 기획 확인 항목 참고.

---

## ② 세이브

```text
FLSCharacterNodeProgress
└── ActivatedNodeIDs   활성화한 노드 집합

ULSSaveGame
└── NodeProgressByCharacter   키 = CharacterID
```

`SkillLoadoutsByCharacter`와 같은 캐릭터 키 구조를 쓴다. 조작 API는 `ULSSaveSubsystem`에 두고, 기존 `OnSkillLoadoutChanged` 패턴대로 변경 시 델리게이트를 발행한다. 저장 구조의 단일 출처는 [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md)다.

### 결정: 로드아웃에는 기본 스킬 ID를 저장한다

`FLSSkillLoadout::SkillIDs`에 진화 ID를 직접 넣지 않는다. 기본 ID를 저장하고, 어느 진화가 활성인지는 노드 진행이 소유한다.

두 축을 직교로 유지하기 위한 결정이다. 진화 ID를 슬롯에 저장하면 (a) 진화가 바뀔 때마다 로드아웃 슬롯을 다시 써야 하고, (b) `ULSSkillLoadoutWidget::IsSelectableSkillType`이 `Skill_Type`의 Active/Ultimate만 보므로 진화 row가 로드아웃 후보로 노출된다. 진화는 노드로 얻는 것이고 로드아웃에서 고르는 것이 아니다.

### 노드 활성화 = 칩 + 골드 복합 소비

칩과 골드를 함께 차감하므로 **반드시 원자적이어야 한다.** `ULSSaveSubsystem::TryCraft`가 쓰는 **"복사본에 시뮬레이션 → 성공했을 때만 `MoveTemp` 커밋"** 패턴을 따른다. 규칙의 단일 출처는 [CraftingSystem.md](CraftingSystem.md)다.

순서:

```text
1. ③의 CanActivateNode로 활성 가능 여부 검증
2. Inventory / WarehouseItems / Gold 를 복사본에 담기
3. 지정된 칩 슬롯의 등급이 Cost_Chip_Grade 조건을 만족하는지 확인 → 복사본에서 제거
4. Gold >= Cost_Gold 확인 → 복사본에서 차감
5. 전부 성공했을 때만 커밋 + ActivatedNodeIDs 추가 + Save() + 델리게이트 발행
```

칩 제거는 슬롯 인덱스 지정이므로 `RemoveItemsFromSlotArray`를 쓰지 않는다(칩을 스킵한다 — §먼저 확인할 것 2).

---

## ③ 순수 해석 — `LSSkillNodeResolve`

`LSChipStats`와 같은 위치·같은 성격(정적 함수 모음, UObject 아님)으로 둔다.

```text
FLSSkillRowDelta            스킬 강화 누적 결과
├── MultiplierDelta
├── RangeDelta
├── CooldownDelta
└── HitCountDelta

FLSCharacterNodeTotals      ← 복제 대상 플랫 스냅샷
├── StatModifiers           메인/서브 스탯 → GE
├── SkillDeltas             키 = 기본 Skill_ID
└── Evolutions              기본 Skill_ID → 진화 Skill_ID (배타이므로 1:1)

Resolve(Activated, Rows) -> FLSCharacterNodeTotals
CanActivateNode(Node, Activated, OwnedChips, Gold, OutBlockedReason) -> bool
RingUnlocked(Ring) -> bool                   // 자리표시자, 현재 항상 true (추후)
ValidateCoreConnectivity(Activated, Rows) -> bool   // 세이브 로드 시 1회
```

**`CanActivateNode`는 UI와 서버 검증이 같은 함수를 쓴다.** 기획의 노드 상태 3종(잠김 / 활성 가능 / 활성)과 *"필요 조건 표기 예) 선행 노드를 활성화 해야합니다"*가 `OutBlockedReason`(`FText`)으로 나온다. 칩 프로토콜의 `IsProtocolUnlockVisible`이 UI 가시성과 판정을 공유하는 것과 같은 구조다. 판정 규칙은 §노드 간 연결 구조가 소유한다.

**`RingUnlocked`는 지금 항상 `true`를 반환하는 자리표시자다.** 링 해금 조건이 미정이므로(§먼저 확인할 것 1) 판정 지점만 만들어 두고 내용은 나중에 채운다. 이렇게 해야 조건이 정해질 때 `CanActivateNode`의 구조를 건드리지 않는다.

**진화와 강화가 함께 걸릴 때 순서:** 진화로 최종 Skill_ID를 확정한 뒤, **기본 Skill_ID를 키로 하는 델타**를 적용한다. 즉 진화해도 강화가 유지된다. (기획 확인 항목 — 진화 시 강화를 초기화하려면 델타 키를 최종 ID로 바꾼다.)

**`ValidateCoreConnectivity`는 평시 판정이 아니라 세이브 로드 시 1회 무결성 검사다.** 활성화 경로만 있으면 불변식이 귀납적으로 자동 유지되기 때문이다 — 근거와 방어 대상은 §노드 간 연결 구조가 소유한다.

---

## ④ 적용

### (a) 스탯 노드 → 어트리뷰트 무한 GE (**적용 방식 보류**)

수치를 직접 대입하지 않고 무한 지속 GE를 거친다는 것은 확정이다. 구체적인 적용 방식은 §보류 결정 2에서 다룬다. 제약 5개와 적용 순서는 §노드 타입별 구현 구조가 소유한다. → [ChipSystem.md](ChipSystem.md)

### (b) 스킬 강화 → 리졸버 델타

`ULSPlayerSkillComponent::ResolveActiveSkillRow`를 값 반환 리졸버로 바꾸고, 테이블 row를 복사한 뒤 `SkillDeltas`를 가산 적용해 반환한다. (나)(다) 두 우회 지점을 이 리졸버로 리다이렉트한다. §함정의 4개 계약을 모두 지킨다.

### (c) 스킬 진화 → Skill_ID 스왑

**`ApplySkillEnhancementByIndex`를 쓰지 않는다.** RPC가 아니고 `SkillSlots`가 Replicated도 아니라 로컬 호출만 반영되며, 멀티에서 깨진다. 대신 기존 `ApplyEquippedSkillLoadout`(`LSPlayerSkillComponent.cpp:490`, `BeginPlay`에서 호출, 서버 권한/로컬 조종 가드가 이미 있음)에 해석 결과를 끼운다.

```text
BeginPlay
-> 세이브에서 SkillPool->CharacterID로 로드아웃(기본 ID 4칸) 조회
-> 같은 CharacterID로 노드 진행 조회 → Resolve
-> 각 슬롯 SkillID를 Totals.Evolutions로 치환          ← 신규
-> SkillPool->FindSkillByID로 DataAsset 해석 → SetSkillData
```

검증된 경로 안에서 ID만 바뀌므로 복제·권한 문제가 새로 생기지 않는다.

**`ULSSkillPoolDataAsset::FindSkillByID` 확장이 필요하다.** 진화 DataAsset은 `SelectableSkills`에 없어 현재 해석이 실패한다. 각 항목의 `EnhancementVariants`까지 **재귀 탐색**하도록 확장한다. 진화 DA 목록용 새 배열을 만들지 않는다 — `EnhancementVariants`가 이미 "이 스킬의 강화 목록"의 단일 출처이고, 같은 목록을 두 번 채우게 하면 안 된다. 후보 나열은 `SelectableSkills` 순회 그대로이므로 진화가 로드아웃 후보로 새지 않는다.

---

## ⑤ UI

### 진입 경로 — `ELSLobbyPanel`에 값 추가 (최상위 배타 패널)

기획: *"캐릭터 탭에서 OS활성화 탭 진입"*

현재 **캐릭터 탭 하위에 서브탭이 없다.** `CharacterTab` 클릭이 곧바로 `ShowPanel(ELSLobbyPanel::SkillLoadout)`이다. 또 2단 구조를 담당했던 `ULSLoadoutPreparationWidget` / `WBP_LoadoutPreparation`은 **죽은 코드**다(참조자 없음 — 중첩 스위처 한 단계를 제거하는 리팩터가 이미 끝났고, `LSLobbyPanelTypes.h` 주석이 이를 명시한다).

**권장: `ELSLobbyPanel`에 값을 추가하고 최상위 배타 패널로 둔다.** 캐릭터 탭 아래에 서브탭 바를 그리더라도 패널 전환 자체는 최상위에서 처리한다. 패널 등록은 `ResolvePanelPage` / `RefreshPanelOnOpen` / `ValidatePanelBindings` 세 곳에 **쌍으로** 넣는다. → [LobbyScreenStructure.md](LobbyScreenStructure.md)

대안(캐릭터 래퍼 패널 + 내부 스위처)은 기획 문구와 일치하지만 [UIEquipDropWidgetSwitcher.md](../Troubleshooting/UIEquipDropWidgetSwitcher.md)가 다루는 **미해결(조사 중) 드롭 실패 버그**를 재도입한다. 이 화면의 핵심 인터랙션이 재료 슬롯에 칩을 드롭하는 것이므로 택하지 않는다.

### 노드 그래프 위젯 — 재사용 지점

방사형 노드 그래프나 연결선 위젯은 프로젝트에 **없다.** 다음 조각들의 패턴을 복사한다.

| 필요 | 재사용 대상 |
|---|---|
| 연결선 | `ULSMinimapWidget::DrawPolyline` — `NativePaint` override + `FSlateDrawElement::MakeLines` + `Geometry.ToPaintGeometry()` |
| 노드 원 / 외곽선 | 같은 파일 `DrawCircleOutline` / `DrawFilledCircle` |
| 노드 아이콘 | 같은 파일 `DrawMarkerTexture` |
| 선택 하이라이트 | `ULSCraftingRowWidget::NativePaint` — `FSlateRoundedBoxBrush` + `MaxLayer + 1` 레이어 처리 |
| 노드 좌표 배치 | 동적 `UCanvasPanel` + `AddChildToCanvas` / `SetPosition` (선례: `ULSProtocolDebugWidget`) |
| 우측 상세 패널 | `ULSSkillLoadoutWidget::SelectedSlotEntry` 패턴(선택 항목을 전용 엔트리 위젯에 표시) |
| 칩 인벤토리 목록 | `ULSChipStationWidget::RefreshChipSlots`(인벤토리+창고에서 칩만 수집·정렬) + 등급 필터 추가 |
| 첫 프레임 레이아웃 튐 방지 | 루트를 `ULSLayoutRevealWidget` 상속 |

미니맵 헬퍼들은 전부 `private`이고 "미니맵 원 안"이라는 좌표계를 전제하므로, 직접 호출이 아니라 **패턴 복사**가 현실적이다.

> `Upgrade_Line1~4` / `Upgrade_Cuser` / `Upgrade_NoCuser` 텍스처는 노드 연결선용이 아니다. 죽은 탭 위젯(`WBP_LoadoutPreparation` / `WBP_LoadoutPreparationTab`)의 탭 장식·버튼 브러시이며, 두 에셋 모두 참조자가 없다. `Upgrade_BG`는 완전 미사용. 노드 그래프에 재사용할 수 있는 기존 텍스처는 없다고 본다.

### 재료 슬롯 (칩 드롭 대상) — 반드시 지킬 계약

**구조:** `ULSItemSlotWidget`을 표시 전용(`SetDisplayOnlySlotContext` + `HitTestInvisible`)으로 두고, 감싸는 래퍼 `UUserWidget`이 `NativeOnDrop`을 소유한다. 선례 둘이 동일 구조다 — `ULSQuickSlotWidget`, `ULSChipEquipmentSlotWidget`.

| 계약 | 근거 |
|---|---|
| `DefaultDragVisual`에 `this`를 넣지 않는다. `CreateWidget`으로 별도 인스턴스를 만든다 | [UIDragDropPackagedBuild.md](../Troubleshooting/UIDragDropPackagedBuild.md) — 패키지 빌드에서 그 위젯의 드래그 감지가 영구히 죽는다 |
| 노드 그래프 배경·연결선 레이어는 `SelfHitTestInvisible` | [LobbyScreenStructure.md](LobbyScreenStructure.md) — 겹친 히트테스트 패널이 드롭을 먹는다 |
| 더블클릭 성공 시 마우스 캡처를 해제하고 억제 플래그를 세운다 | [UIDoubleClickDragRace.md](../Troubleshooting/UIDoubleClickDragRace.md) |
| 드래그 진행 중에는 위젯 트리를 리빌드하지 않는다(다음 틱으로 미룸) | [InventoryLogic.md](InventoryLogic.md) |
| 다이얼로그는 다음 프레임에 생성하고, 로비의 모달 예외 목록에 **보고·정리 한 쌍**으로 등록한다 | [UILobbyModalFocusReclaim.md](../Troubleshooting/UILobbyModalFocusReclaim.md) — 로비가 매 틱 포커스를 회수해 첫 클릭이 씹힌다 |

---

## 보류 결정

아래 두 항목은 아직 결정하지 않았다. **문서 본문은 어느 쪽에도 의존하지 않게 쓰여 있다** — 결정이 나면 해당 절만 채운다.

### 1. 노드 식별자

| 후보 | 장점 | 단점 |
|---|---|---|
| **`FName`** (RowName 자체) | RowName 보정 패턴 복붙을 피한다. CSV에서 어느 노드가 어느 노드를 선행으로 갖는지 눈으로 보인다. 조회가 `FindRow` fast path만으로 끝난다. 새 ID를 부여하다 겹치는 실수가 없다 | 다른 테이블 키(`Skill_ID` 등)가 `int32`라 어휘가 섞인다 |
| `int32` (숫자 RowName + 보정 패턴) | 기존 `Skill_ID`/`Combo_ID`와 어휘가 통일된다 | 보정 패턴이 4번째 복붙이 된다. **RowName이 전부 숫자가 아니면 조용히 0**이 되고 경고도 없다(아래) |
| `int32` (`Name` 헤더 + 별도 ID 컬럼) | 프로젝트 다수 관례(`DT_Protocol`·`DT_StatusEffect`·`DT_CraftingRecipe`). 보정 불필요, RowName은 읽기 쉬운 이름 | ID와 RowName이 이원화돼 어긋나면 추적이 어렵다 |

> ⚠️ **RowName 보정 패턴의 함정.** `FLSCharacterSkillRow`·`FLSCharacterPassiveSkillRow`·`FLSComboAttackRow`가 쓰는 `Normalize*IDFromRowName` 헬퍼는 **RowName에 비숫자 문자가 하나라도 있으면 조용히 return**한다(로그 없음). `Node_CH01_R1_Atk` 같은 읽기 쉬운 RowName을 쓰면 ID가 영구히 0으로 남는다. 세 구조체가 각 28줄 완전 복붙이며 공유 헬퍼가 없어, 노드가 이 패턴을 택하면 4벌이 된다.
>
> 배경: UE CSV 임포터는 **헤더 이름과 무관하게 0번 컬럼을 RowName으로 소비**하므로, 0번 컬럼 헤더를 `Node_ID`로 써도 ID 프로퍼티에는 아무 값도 들어오지 않는다. 그래서 보정이 필요한 것이다.

### 2. 스탯 노드의 어트리뷰트 적용 방식

| 후보 | 내용 | 트레이드오프 |
|---|---|---|
| `ResolveAttribute` 추출 후 공유 | private static인 `ULSStatusEffectComponent::ResolveAttribute`를 `Data/` 아래 공용 네임스페이스로 추출하고 StatusEffectComponent도 그것을 호출하게 바꾼다. 노드는 기획 어휘(`Char_*`)를 그대로 쓰고 동적 무한 GE로 적용 | 매핑이 하나로 준다 — 새 스탯은 한 곳만 확장하면 된다. 공용 코드를 건드리는 리팩터가 들어간다 |
| ChipStat 패턴 복제 | 사전 정의 무한 GE + SetByCaller. 칩·장비 스탯과 완전히 동일한 관용구 | 가장 안전하고 기존 코드를 안 건드린다. 매핑이 **5개**로 늘고, 스탯을 추가할 때 GE 생성자와 컴포넌트 양쪽을 수정해야 한다 |

배경 사실: **프로젝트에 병렬 어트리뷰트 매핑이 이미 4개 있다.**

1. `ULSStatusEffectComponent::ResolveAttribute` — `Char_*`/`Mon_*` → 어트리뷰트 (private static, if-else 체인)
2. `ULSGE_ChipStats` 생성자 + `ULSChipStatComponent`의 SetByCaller 호출 — `Chip_*` → 태그
3. `ULSGE_EquipmentStats` 생성자 + `ULSEquipmentStatComponent` — 동일 패턴
4. `ULSGA_CombatAccelerationPassive::AccumulateCombatAccelerationModifier` — **`ResolveAttribute`와 같은 `Char_Attack`/`Char_Atkspeed` 문자열을 독립적으로 재구현**했고 정규화 함수까지 복제돼 있다

노드가 5번째를 만들지 여부가 이 결정의 실질이다.

---

## 코드 관례

새 Row·네임스페이스를 만들 때 따를 것. AGENTS.md가 이미 규정한 일반 규칙은 반복하지 않고 **이 테이블에 특유한 부분만** 적는다.

- Row는 `Source/LostSignal/Data/LSSkillNodeRow.h` **단일 헤더, `.cpp` 없음**(`Data/` 18개 Row가 예외 없이 그렇다). include 순서 `CoreMinimal.h` → `Engine/DataTable.h` → `.generated.h`
- 모든 UPROPERTY는 예외 없이 `EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode[/서브]"` + **스칼라 기본값 초기화자**. `Data/*Row.h`에 `EditDefaultsOnly`/`BlueprintReadWrite` 사용례가 0건이다
- ⚠️ **Category 구분자**: UE는 `|`만 계층으로 처리하고 `/`는 카테고리 이름의 리터럴 문자로 본다. 즉 `"LS/SkillNode/Cost"`는 평평한 이름 하나가 된다. 그래도 `Data/*Row.h` 이웃 다수가 `/`를 쓰므로 **이웃 일관성을 따른다**(Diff 최소화). 계층이 실제로 만들어지지 않는다는 사실만 알고 쓴다
- enum(`ELSSkillNodeKind`, `ELSSkillNodeTargetField`)은 **같은 헤더 최상단**, `UENUM(BlueprintType)` + `: uint8` + **`None`을 첫 값으로**. CSV 셀이 빌 수 있는 컬럼 enum은 `None`으로 시작하는 것이 프로젝트 관례다(`Target_Field`는 스탯 노드 row에서 실제로 비므로 `None` 필수)
- `LSSkillNodeResolve`는 `Data/` 아래 **`namespace`**(프로젝트에 `class { static }` 방식이 0건), 함수마다 `LOSTSIGNAL_API`. `.cpp`에서는 `namespace`를 다시 열어 정의한다(한정자 방식이 아니다). 파일 로컬 헬퍼는 명명된 네임스페이스 앞의 익명 `namespace`에 두고 `SkillNode` 도메인 접두사를 붙인다
  - ⚠️ `LSDropSubsystem.cpp`에 접두사 없는 `ExtractRowNamePrefix`가 있다. 유사한 RowName 파싱 헬퍼를 만들 때 같은 이름을 쓰면 유니티 빌드에서 중복 정의로 터진다
- `FLSCharacterNodeTotals`만 `USTRUCT`(복제 대상). `FLSSkillRowDelta`는 plain struct로 충분하다(`FLSChipProtocolTotals`가 같은 선례)
- 테스트는 `Tests/LSSkillNodeTests.cpp`, `IMPLEMENT_SIMPLE_AUTOMATION_TEST` + 경로 `"LostSignal.SkillNode.<동작>"` + `EditorContext | EngineFilter`. **build.cs 수정 불필요** — `Tests/`의 `.cpp`는 일반 모듈 소스로 컴파일된다. 테스트 대상 헤더 include는 `#if WITH_DEV_AUTOMATION_TESTS` **밖**에 둔다(TU가 비지 않게)
- **로그는 한국어**, 접두사 `[SkillNode]`. 최근 작성된 제작·세이브 코드가 영어로 드리프트한 상태이므로 명시해 둔다

---

## 구현 순서

계층 순으로 간다. 각 단계가 앞 단계의 계약만 알면 되도록 한다.

```text
Phase 1  데이터·해석 (게임플레이 영향 0)
  1. FLSSkillNodeRow + CSV, ULSGameDataSettings에 테이블 참조 (+ LogMissingTables 경고 쌍)
  2. ULSGameDataSubsystem 조회 API + Kind별 로드 검증(skip-and-warn)
  3. 그래프 무결성 검증 (WITH_EDITOR — 미존재 선행 / 사이클 / 섬 / Ring 역행)
  4. LSSkillNodeResolve + 단위 테스트

Phase 2  세이브·소비
  5. ULSSaveGame / ULSSaveSubsystem 노드 진행 저장
  6. 칩 + 골드 원자적 소비 (칩은 슬롯 인덱스 지정)

Phase 3  적용
  7. 스킬 진화: FindSkillByID 재귀 탐색 + ApplyEquippedSkillLoadout 치환
  8. 스킬 강화: ResolveActiveSkillRow 값 반환 전환 + 델타, (나)(다) 리다이렉트
  9. 스탯 노드: 무한 GE 적용 (§보류 결정 2 확정 후)
  10. FLSCharacterNodeTotals 복제 배선 (MO 대비)

Phase 4  UI
  11. ELSLobbyPanel 값 추가 + 패널 등록(3곳 쌍)
  12. 노드 그래프 위젯 (연결선 + 좌표 배치 + 상태 3종 시각화)
  13. 노드 클릭 확대 + 우측 상세 패널 + 재료 슬롯 칩 드롭
  14. 진화 노드 마우스오버 스킬 정보/영상 (영상 재생 방식은 별도 확인)

추후  링 해금 조건 (RingUnlocked 자리표시자를 채운다)
```

Phase 1~2는 게임플레이에 영향을 주지 않으므로 먼저 넣고 테스트로 굳힐 수 있다. **Phase 1은 §보류 결정 1(식별자)이 정해져야 시작할 수 있고, Phase 3-9는 §보류 결정 2가 정해져야 한다.** 나머지는 보류와 무관하게 진행 가능하다.

---

## 검증 계획

**③ 해석 계층 — 자동화 테스트** (`Tests/LSProtocolUnlockTests.cpp` 형식)

연결 판정:
- 선행 ANY 시맨틱: 선행 2개 중 1개만 활성 → **통과**해야 한다(ALL로 잘못 구현하면 여기서 걸린다)
- 선행이 하나도 활성이 아니면 거부. `Prereq`가 빈 노드(코어)는 선행 검사 없이 통과
- 칩 등급 부족 거부 / 골드 부족 거부 / 이미 활성인 노드 재활성 거부
- 같은 `Target_Skill`의 진화 노드 2개 동시 활성 거부

노드별 해석:
- 강화 델타 누적: 같은 `(Target_Skill, Target_Field)` 노드 여러 개 → 합산
- 스탯 누적: 같은 `(Target_Stat, Math_Type)` 여러 개 → 합산. 다른 `Math_Type`은 분리 유지
- Kind별 검증: 필수 컬럼이 빈 row가 skip되고 나머지 row는 정상 로드되는지(전체 실패가 아니어야 한다)

그래프 무결성:
- 사이클이 있는 데이터 → 검출
- 코어에서 도달 불가한 섬 → 검출
- 미존재 노드를 선행으로 참조 → 검출
- Ring 역행 → 검출
- 코어 연결성: 정상 활성 집합은 통과, 중간 노드를 강제로 빼면 위반 검출

**스킬 강화 일관성 — 가장 중요한 수동 검증.** 훅이 (가) 한 곳으로 모였는지를 실제로 확인하는 테스트다.

- 범위 강화 노드 활성 후 **프리뷰 메시 크기와 실제 판정 범위가 일치**하는가
- 쿨타임 강화 후 **UI 숫자와 실제 GE 지속시간이 일치**하는가
- ShortCircuit 필드 반경·타수, Bypass 홀로그램 풀 반경도 바뀌는가((나)(다) 리다이렉트 검증)

**0값 함정 재현.** 테이블 값이 0인 필드에 델타를 건 노드를 일부러 만들어, `LogLS` 경고가 뜨고 강화가 무효화되는 것을 눈으로 확인한다. 조용히 무효가 되지 않는 것이 핵심이다.

**원자성 — 자동화 테스트.** 골드는 충분하지만 칩이 없는 상태로 활성화 시도 → 골드가 차감되지 않아야 한다. 반대 경우도 확인한다. 복사본 시뮬레이션 방식의 소비를 테스트하는 선례가 `Tests/LSCraftingTests.cpp`에 있으므로 그 형식을 따른다.

**세이브 영속.** 노드 활성 → 게임 종료 → 재시작 후 유지. `ULSSaveSubsystem`의 JSON 디버그 export에 노드 진행을 추가해 눈으로 확인한다.

**UI 드롭은 패키지 빌드에서 확인한다.** `UIDragDropPackagedBuild.md`의 문제는 에디터에서 재현되지 않는다.

---

## 기획 확인 항목

1. **"분기"가 스탯에도 하드 배타인가** — 기획서의 *"분기 = 둘 이상의 노드 중 선택하는 지점, 능력치·스킬 특화 선택 유도"*를 현재는 "재화 제약에 의한 사실상의 선택"으로 해석했다(배타를 명시한 곳은 스킬 진화뿐). 스탯 분기도 하드 배타여야 하면 `Exclusive_Group` 컬럼을 되살려야 한다.
2. **노드로 올릴 스탯 목록** — 현재 어트리뷰트 매핑이 4개(공격력·공격속도·이동속도·방어력)뿐이다. 체력·치명타·회복·스킬가속 노드를 원하면 매핑 확장 작업이 추가로 붙고, 최대 체력은 체력 보존 로직까지 필요하다.
3. **"노드 교체"의 의미** — 리스펙(해제)이 있는가? 있으면 환불 규칙과, 해제로 생긴 고아 노드 처리 정책(연쇄 해제 vs 해제 거부)이 필요하다. 기획서 비용 섹션에 환불 언급이 없다.
4. **타수 강화 범위** — 현재 ShortCircuit 전용이다. 다른 스킬은 계수·범위·쿨타임만 가능하다.
5. **진화 후 강화 유지 여부** — 기본 스킬에 걸린 강화가 진화 후에도 유지되는가(권장), 초기화되는가? 델타 키를 기본 ID로 둘지 최종 ID로 둘지가 갈린다.
6. **칩 소모 모델** — 등급 조건만 맞으면 어떤 칩이든 되는가, 특정 기능(HP/공격 등) 제약도 있는가?
7. **노드 구조 이미지 미확인** — 기획서에 임베드된 구조 이미지 2장과 링크된 「노드 구조.pptx」를 읽지 못했다. 링별 노드 개수, 연결선 형태, 같은 링 내 형제 연결 유무가 `Prereq` 설계에 영향을 줄 수 있다. 좌표를 데이터 컬럼으로 열어뒀고 `Prereq`가 배열이므로 구조 자체는 바뀌지 않을 것으로 본다.

> **링 해금 조건**은 확인 항목이 아니라 **추후 개발 예정**으로 확정했다. §먼저 확인할 것 1 참고.

---

## 금지 / 주의 사항

- 전투 런타임에서 노드 테이블을 직접 조회하지 않는다. 스냅샷만 소비한다.
- 선행 조건을 **ALL로 구현하지 않는다.** ANY다(선행 여러 개 중 하나만 충족하면 활성 가능).
- `ValidateCoreConnectivity`를 평시 활성화 판정에 쓰지 않는다. 세이브 로드 시 1회다.
- `Prereq`를 `FString`으로 만들지 않는다. 진짜 `TArray`로 만든다(죽은 데이터가 된 선례가 둘 있다).
- 스킬 강화를 **배수**로 적용하지 않는다(0값 함정).
- 칩 소비에 `RemoveItemsFromSlotArray`를 쓰지 않는다(칩을 스킵한다).
- 칩과 골드를 비원자적으로 차감하지 않는다.
- `ApplySkillEnhancementByIndex`를 진화 적용 경로로 쓰지 않는다(복제되지 않는다).
- 진화 DataAsset을 `SelectableSkills`에 등록해 로드아웃 후보로 노출하지 않는다.
- 노드 그래프 배경·연결선 레이어를 히트테스트 가능하게 두지 않는다(칩 드롭을 먹는다).
- 수치(스탯 증가폭·비용·칩 등급별 요구량)를 이 문서에 적지 않는다. DataTable이 단일 출처다.
