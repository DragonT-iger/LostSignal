# 캐릭터 강화 노드 시스템 (토폴로직형)

## 목적과 범위

코어 중심 방사형 노드 그래프로 캐릭터를 심화 성장시키는 시스템의 구조 문서다. 이 문서는 기획을 현재 코드베이스에 앉히는 **구조 결정과 그 근거**를 소유한다.

기획 출처 둘:

- `Lost_Signal 캐릭터 강화 시스템.md` — 토폴로직형 기획서(개념·규칙)
- `LostSignal_노드시스템 데이터테이블.xlsx` — 노드 데이터 밸런싱 초안(컬럼·수치·토폴로지). 검증 결과는 §노드 데이터 실측

**이 문서가 소유하는 것:** 노드 그래프 정의, 링 해금 조건, 노드 활성화 비용과 소비 경로, 노드 진행 저장, 노드 해석(resolve) 계층, 노드 UI.

**이 문서가 소유하지 않는 것:** 스킬 런타임 실행(발동·쿨타임·GAS·프리뷰)은 [SkillSystemStructure.md](SkillSystemStructure.md)가 계속 소유한다. 칩 데이터·신호 게이지·프로토콜은 [ChipSystem.md](ChipSystem.md), 아이템 저장 구조는 [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md), 재료 소비 원자성 규칙은 [CraftingSystem.md](CraftingSystem.md)가 소유한다. 여기서는 링크로만 참조하고 값을 복붙하지 않는다.

**현재 구현 상태: 미구현(그린필드).** 노드 그래프 관련 C++·DataTable·위젯이 하나도 없다. 있는 것은 스킬 진화에 재사용할 런타임 뼈대(`ULSSkillDataAsset::EnhancementVariants`, 호출자 없는 `ULSPlayerSkillComponent::ApplySkillEnhancementByIndex`)와 진화용 DataAsset·아이콘 에셋뿐이다.

---

## 용어 정리 (기획 용어 ↔ 코드 용어)

기획서와 기존 코드가 "강화"라는 말을 서로 다른 의미로 쓴다. 혼동 지점이므로 먼저 고정한다.

| 기획 용어 | 의미 | 코드에서의 대응 |
|---|---|---|
| **스킬 강화** (육각형 노드) | 스킬의 계수·범위·쿨타임을 **수치로** 상향 | **신규 경로 — DataTable row 값에 비율 적용** |
| **스킬 진화** (마름모 노드) | 스킬의 **작동 구조**를 변경. 동일 스킬은 1개만 활성(배타) | 기존 `EnhancementVariants` **Skill_ID 스왑** |

`SkillSystemStructure.md`의 "강화 스킬 구조" 섹션이 다루는 것은 기획 용어로는 **스킬 진화**다. 스킬 강화(수치)는 그 문서에 경로가 없다.

기획서 나머지 용어(코어 / 노드 / 핵심 노드 / 링 / 분기)는 기획서 정의를 그대로 쓴다.

---

## 먼저 확인할 것 — 기획서가 전제하지만 코드에 없는 것 3개

아래 셋은 기획서가 전제하지만 코드에 대응이 없는 항목이다. 1번(링 해금)은 **추후 개발 예정으로 확정**해 이번 범위에서 뺐고, 2번은 구현 시 우회가 필요하다. 3번은 실제 데이터에 해당 노드가 없어 **현재 무효**다(§노드 데이터 실측 참고). 셋 다 그래프 연결·노드 해석 구조를 바꾸지는 않는다.

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

### 3. "타수 강화"를 적용할 코드 표면이 하나뿐이다 — 데이터가 쓰지 않아 현재 무효

기획: 스킬 강화 노드 = *"스킬 계수, 범위, 쿨타임, 타수 강화"*

`Skill_HitCount`/`Skill_HitRate`를 읽는 곳은 **`ALSShortCircuitField`(`LSShortCircuitField.cpp:292-293`) 단 하나**다. Override·Overclock·Execution·Bypass는 전부 단발 판정이라 타수를 늘릴 코드 지점이 존재하지 않는다.

**다만 실제 노드 데이터에 타수 강화 노드가 하나도 없다.** 스킬 강화 12개 노드가 쓰는 필드는 `Skill_Multiplier` / `Range_X` / `Skill_Cooldown` 3종뿐이다(§노드 데이터 실측). 따라서 이 제약은 지금 걸리지 않는다.

→ 기록으로만 남긴다. **나중에 타수 노드를 추가하면 ShortCircuit 외에는 효과가 없다.** 다른 스킬에 붙이려면 해당 Ability에 다단 히트 로직을 새로 만들어야 한다(이 시스템의 범위 밖).

---

## 노드 데이터 실측 (기획 산출물 `LostSignal_노드시스템 데이터테이블.xlsx`)

기획이 제출한 밸런싱 초안을 코드와 대조 검증한 결과다. **수치는 여기에 적지 않는다** — DataTable이 단일 출처다. 여기 남기는 것은 구조 결정에 영향을 준 사실뿐이다.

### 규모와 토폴로지

캐릭터 3명 × 45노드 = **135노드**, 엣지 **192개**. 캐릭터당 코어 1 / 서브스탯 24 / 메인스탯 12 / 스킬강화 4 / 스킬진화 4.

링 배치는 4섹터 반복이다 — `코어 → 1차 메인 4 → (1차 서브 4는 인접 1차 메인 2개 사이의 분기) → 2차 서브 12 → 2차 메인 8 → 2차 스킬강화 4 → 3차 서브 8 → 3차 스킬진화 4`.

### 무결성 검증 — 전부 통과

§노드 간 연결 구조가 정의한 검사 6종에 데이터 정합성 2종을 더해 실제 데이터에 돌린 결과다.

| 검사 | 결과 |
|---|---|
| 노드 키 중복 (5시트 통합) | 0 |
| 미존재 노드 참조 | 0 |
| 사이클 | 0 |
| 코어 도달성 | 3캐릭터 전부 45/45, 섬 0 |
| Ring 역행 | 0 |
| 캐릭터 교차 연결 | 0 |
| 선행 없는 노드(코어 외) | 0 |
| `Slot` 중복(캐릭터 내) | 0 |

같은 링 안의 연결이 120건 있는데 **정상이다** — 1차 서브 분기(1차 메인 ↔ 1차 서브)와 2차 머지(2차 서브 → 2차 메인)가 동일 링 안에서 일어난다. 따라서 Ring 역행 검사는 **동일 링을 허용하고 역행만 잡아야 한다.**

### 스킬 테이블과의 정합성 — 전부 일치

- 스킬강화 12건의 `Before` 값이 `DT_ActiveSkill`의 현재 값과 **전부 일치**한다.
- `Multiply` 검산(`Before × (1 + Value) == After`)도 12건 전부 맞는다.
- 스킬진화 12건의 `Base_Skill_ID` → `Evolution_Skill_ID` 대응이 `DT_ActiveSkill::Parent_Skill_ID`와 **전부 일치**한다.
- 칩 등급 6종(`Supply`/`Standard`/`Precision`/`Tuning`/`Prototype`/`Masterpiece`)이 `DT_Chip.csv` 24행의 두 번째 토큰과 **전부 일치**한다.

### 코드와 어긋나는 것 2건

| 어긋남 | 상세 | 처리 |
|---|---|---|
| `Char_HP_Recovery` | 코드 필드명은 **`Char_Recovery`**(회복력)다 | 테이블을 코드 이름에 맞춘다 |
| 단위 `정수`인데 값이 소수 16건 | 어트리뷰트가 `float`이라 런타임은 통과한다. 표기와 모순이므로 UI 표시 규칙(내림·반올림)이 필요하다 | 기획 확인 항목 |

### 강인도(Tenacity)는 수치 어트리뷰트다

코드 어휘가 **Tenacity**다(`Poise`로 검색하면 안 나온다). 구현 실태:

- **게이트는 공용이다.** `ULSCharacterCombatComponent::ResolveIncomingImpact`(`LSCharacterCombatComponent.cpp:84`)가 `bCrowdControlBlocked = BreakPower < TargetTenacity`로 판정하고, 이 컴포넌트는 `ALSCharacterBase` 계열 공용이라 **플레이어와 몬스터 양쪽**에 붙는다. 몬스터 공격도 이 경로를 탄다(`LSMonsterCombatComponent.cpp:331`).
- **받는 쪽(강인도)은 `ULSCharacterAttributeSet::Tenacity`가 소유한다.** 플레이어 스탯 DataTable에는 강인도 컬럼을 두지 않고 Attribute 기본값 `1`로 초기화한다. 노드·GameplayEffect는 이 수치에 가산할 수 있다.
- **상태 태그는 최소 강인도 보정이다.** `GetCurrentTenacity()`는 Attribute를 읽고 `State_SuperArmor`일 때 최소 `4`, `State_Invincible`일 때 최소 `6`으로 보정한다. 무적의 데미지 차단도 기존처럼 태그가 소유한다.
- **때리는 쪽(붕괴력)만 수치→티어 변환이 있다.** 캐릭터는 `Skill_Impact`(`LSGA_Override.cpp:17` `ToOverrideAbilityBreakPowerTier`), 몬스터는 `Action_Impact`(`LSMonsterCombatComponent.cpp:283`).
- **몬스터 기본 강인도는 `FLSMonsterArchetypeRow::Monster_Guard`다.** 몬스터 생성 시 같은 `Tenacity` Attribute에 초기화한다. `FLSMonsterActionRow::Action_Guard`는 액션 중 일시 강인도로서 여전히 후속 범위다.

→ 따라서 `Char_Poise +0.5`는 `Tenacity`에 적용한다. 플레이어 기본값은 DataTable이 아니라 Attribute 기본값 `1`이 단일 출처다.

---

## 전체 구조 — 5계층

```text
① 정적 정의   노드 테이블 5개 (Kind별 분리 — §① 정적 정의)
                  ↓
② 세이브      ULSSaveGame::NodeProgressByCharacter { ActivatedNodeIDs }
                  ↓
③ 순수 해석   LSSkillNodeResolve::Resolve() → FLSCharacterNodeTotals   ← 플랫 스냅샷
                  ↓
④ 적용        (a) 스탯   → 어트리뷰트 무한 GE
              (b) 스킬강화 → row 비율 (리졸버 훅)
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
| **스킬 강화** | 육각형 | **row 수치에 비율 적용** | **없음 — 신규** |
| 스킬 진화 | 마름모 | Skill_ID 스왑 | `EnhancementVariants` 그대로 |

메인/서브 스탯의 차이는 수치 크기와 비용뿐이라 코드 경로가 같다. 기획의 *"메인 스탯 수치는 서브스탯 3~4개 분"*은 DataTable 값으로 표현하며 이 문서에 수치를 적지 않는다.

---

## 핵심 결정: 스킬 강화는 row 스왑이 아니라 row 값 가로채기

### 스킬 진화는 기존 스왑으로 충분하다

기획의 *"동일한 스킬은 1개만 활성화 가능하며, 중복 활성화가 불가능"*이 `EnhancementVariants`의 배타 스왑 계약과 정확히 일치한다. 진화 DataAsset이 자기 `Skill_ID`를 가지므로 스왑 즉시 다른 row가 해석된다. **신규 메커니즘이 0이다.**

### 스킬 강화는 스왑으로 표현할 수 없다

링 경로를 따라 여러 강화 노드를 지나가므로 효과가 **누적**된다. 스왑 방식은 조합마다 DataTable row + DataAsset이 필요해 조합 폭발(2ⁿ)이 난다. 따라서 row 수치를 가로채 비율을 적용해야 한다.

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

**(가)를 `const*` 반환에서 값 반환 리졸버로 바꾸고 비율을 적용하면, 서버 판정·클라 표시·클라 예측이 한 곳에서 일관되게 강화된다.** (나)(다)는 이 리졸버로 리다이렉트한다.

UI 위젯(`ULSSkillLoadoutEntryWidget`, `ULSSkillLoadoutWidget`)은 이름·설명·타입만 읽고 수치를 읽지 않으므로 일관성에 무해하다. 단 툴팁에 상향된 수치를 보여주려면 네 번째 훅이 필요하다.

---

## 함정 (반드시 지킬 계약)

### 1. `> 0.0f` 게이트 — 0값 함정

**모든 소비 지점이 row 값이 0이면 DataAsset·Ability 폴백으로 넘어간다.** 예: `ResolveSkillCooldownDuration`은 `Row->Skill_Cooldown > 0.0f`일 때만 row를 쓰고, `ALSShortCircuitField`도 `Skill_HitCount > 0`일 때만 쓴다.

**연산 방식에 따라 고장 나는 모양이 다르다. 둘 다 고장이다.**

| 연산 | row 값이 0일 때 | 결과 |
|---|---|---|
| **비율** (기획 선택) | `0 × (1+V) = 0` → 게이트가 닫힌 채 남고 폴백이 계속 이긴다 | **강화가 조용히 무효** |
| 가산 | `0 + V ≠ 0` → 게이트가 열리고 **row가 폴백을 대체한다** | 스킬 수치가 폴백값에서 델타값으로 **급변** |

비율이 덜 위험하다(수치가 튀지 않는다). 하지만 **함정이 사라지는 것은 아니다** — 강화가 아무 일도 하지 않는데 UI에는 활성으로 보이는 상태가 된다.

→ **실제 안전장치는 연산 선택이 아니라 경고 로그다.** 리졸버에서 "델타는 있는데 row 값이 0"인 상황을 만나면 `UE_LOG(LogLS, Warning)`으로 반드시 남긴다. 조용히 무효가 되는 것이 이 시스템에서 가장 찾기 어려운 버그다. 그리고 강화 대상 필드는 테이블에 0이 아닌 값을 넣는 것을 기획자 계약으로 둔다.

> 현재 데이터는 이 함정에 걸리지 않는다. 강화 12건의 대상 필드 값이 `DT_ActiveSkill`에서 전부 0이 아니다(§노드 데이터 실측). 함정은 앞으로 강화 노드를 추가할 때를 위한 것이다.

### 2. `CooldownReduction` 중첩 순서

`ULSPlayerSkillComponent::ResolveReducedSkillCooldownDuration`이 row 쿨타임 **뒤에** 어트리뷰트를 곱한다.

**노드는 쿨타임을 두 경로로 줄인다.** 순서를 고정해야 한다.

```text
최종 쿨타임 = row.Skill_Cooldown
              × CooldownRatio        ← 스킬강화 노드 (Parameter_Field=Skill_Cooldown)
              × (1 - CooldownReduction)  ← Char_Cal 스탯 노드가 올린 어트리뷰트
```

즉 **row 값에 강화 비율을 먼저 곱하고, 그 뒤에 어트리뷰트 감소율을 적용한다.** 감소율이 강화된 쿨타임에도 비례 적용되는 쪽이 기대에 맞다.

이 겹침은 실제로 발생한다 — 3캐릭터 합산 `Char_Cal` 노드 14개와 쿨타임 강화 노드 4개가 있고, 세 캐릭터 모두 양쪽을 갖는다.

### 3. 시간적 의미 불일치

`ULSGA_Execution`은 발동 시 row를 값복사해 잠그고, `ALSShortCircuitField`는 **펄스마다 재조회**한다(`:339`). 필드가 떠 있는 중에 강화 상태가 바뀌면 이미 뜬 필드의 계수가 소급 변경된다. 노드 조작이 로비 전용이라 실전에서는 발생하지 않지만, 필드 스폰 시 스냅샷을 잠그는 쪽이 안전하다.

### 4. 훅으로 도달할 수 없는 값

강화 노드를 만들어도 다음 값은 바뀌지 않는다. 기획에 이 제약을 알려야 한다.

- **Execution 대시 시간** — `Skill_Time`을 의도적으로 무시하고 `ULSExecutionSkillDataAsset::FallbackDashDuration`만 쓴다.
- **대쉬 쿨타임** — row가 아니라 `DashCooldown` 어트리뷰트 경유다([SkillSystemStructure.md](SkillSystemStructure.md) 쿨타임 구조 참고).
- **ShortCircuit 투사체 비행/아크, 필드 기본 Duration/Interval** — DataAsset이 단일 출처다.

---

## ① 정적 정의

### 결정: Kind별 테이블 5개 / Row 구조체 4개

기획 시트 구조를 그대로 따른다. **MainStat과 SubStat은 컬럼이 완전히 같으므로 Row 구조체를 공유**한다 — 테이블은 5개지만 구조체는 4개다.

| DataTable | Row 구조체 | 행 수(3캐릭터) |
|---|---|---|
| `DT_SkillNodeCore` | `FLSSkillNodeCoreRow` | 3 |
| `DT_SkillNodeMainStat` | `FLSSkillNodeStatRow` | 36 |
| `DT_SkillNodeSubStat` | `FLSSkillNodeStatRow` (공유) | 72 |
| `DT_SkillNodeEnhance` | `FLSSkillNodeEnhanceRow` | 12 |
| `DT_SkillNodeEvolve` | `FLSSkillNodeEvolveRow` | 12 |

**노드 Kind는 컬럼이 아니라 테이블이 결정한다.** 따라서 `Node_Kind` 컬럼과 기획 시트의 `Node_Type` 컬럼은 만들지 않는다. Kind별 필수 컬럼 검증도 구조체가 다르므로 컴파일 단계에서 절반이 해결된다.

### 5분할의 비용 — 통합 인덱스로 갚는다

분할에는 대가가 넷 있고, 앞의 셋은 로드 시 **통합 인덱스 하나**로 해결한다.

| 비용 | 대응 |
|---|---|
| 노드 키 유일성이 테이블 간에 보장되지 않는다 | 인덱스 구축 시 중복 키 검출 → 경고 + 후행 row 무시 |
| 선행 노드 조회가 5테이블 교차 검색이 된다 | 인덱스가 선행 관계 해석의 유일한 조회처 |
| 그래프 순회(사이클·도달성)가 테이블을 넘나든다 | 인덱스만 보고 순회 |
| 공통 컬럼 9개가 4개 구조체에 반복된다 | **감수한다** — 아래 |

```text
FLSSkillNodeRef        인덱스 엔트리
├── Kind               어느 테이블에서 왔는지
├── Ring / Slot
├── Prereq[2]
├── Cost               Chip_Grade / Required_Count / Required_Coin
└── Payload            Kind별 원본 row 포인터

TMap<FName, FLSSkillNodeRef>   키 = 노드 키(RowName)
```

> ⚠️ **공통 컬럼을 Row 구조체 상속으로 공통화하지 않는다.** `Data/`의 Row 구조체 21개가 **예외 없이** `FTableRowBase`를 직접 상속하며, Row가 Row를 상속하는 선례가 0건이다. 중첩 USTRUCT 멤버로 묶는 방법도 쓰지 않는다 — CSV 셀이 `(A=..,B=..)` 형식이 되어 기획자 편집성이 크게 나빠진다. 9컬럼 반복이 5분할의 실제 비용이다.

### 공통 컬럼 (4개 구조체 전부)

```text
[RowName]         노드 키 — FName. 예: CORE-101 / N101-S01 / N101-M05 / N101-K01 / N101-E01
Character_ID      캐릭터 식별자 (ULSSkillPoolDataAsset::CharacterID와 같은 키)
Node_Name         UI 표시명 — FText 필수 (기획 정의는 String이지만 FString 금지)
Ring              소속 링 (0=코어). 해금 판정에는 아직 쓰지 않는다 (§먼저 확인할 것 1)
Slot              배치 라벨 — FName. **정수 각도 인덱스가 아니다** (CORE / R1-01 / R2-M03 / R3-E01)
Prerequisite_1    선행 노드 키
Prerequisite_2    두 번째 선행 노드 키 — 둘 중 하나만 활성이면 충족 (ANY)
Chip_Grade        요구 칩 등급 (ResolveItemGradeFromRowName이 반환하는 토큰)
Required_Count    요구 칩 개수
Required_Coin     요구 코인 (= ULSSaveGame::Gold)
```

### Kind별 고유 컬럼

```text
Stat (Main/Sub)   Stat_Field    변경 대상 스탯 (FLSCharacterStatRow 필드명)
                  Operation     Add / Multiply
                  Value         변화량
                  Unit          정수 / % / %p — UI 표시 전용

Enhance           Skill_ID          강화 대상 기본 스킬 ID (int32)
                  Parameter_Field   Skill_Multiplier / Range_X / Skill_Cooldown
                  Operation         Multiply (실측 12건 전부)
                  Value             비율
                  Unit              % — UI 표시 전용

Evolve            Base_Skill_ID       진화 전 기본 스킬 ID (int32)
                  Evolution_Skill_ID  교체될 진화 스킬 ID (int32)
                  Change_Type         연계 / 제어 / 버프 / 형태변경 … — UI 분류
                  Change_Summary      진화 동작 요약 — FText

Core              고유 컬럼 없음. 비용도 전부 0이다
```

`Pos_X`/`Pos_Y`는 만들지 않는다. 화면 좌표는 위젯이 계산하며, **배치 규칙은 §⑤ UI가 소유한다**(§노드 배치 좌표). `Slot`은 각도 정보를 주지 못하므로 로그·디버그 표시용 라벨로만 쓴다.

### 런타임에 임포트하지 않는 컬럼

기획 시트에는 있지만 C++ Row에 넣지 않는다. 넣으면 이중 출처가 되거나 죽은 데이터가 된다.

| 시트 컬럼 | 제외 이유 |
|---|---|
| `Node_Type` | 테이블이 Kind를 결정한다 |
| `Item_Type` | 항상 `Chip`(코어는 `None`) — 상수 |
| `Chip_Name_Pattern` | `Chip_{Grade}_*`로 `Chip_Grade`에서 도출된다 |
| `Auto_Activate` / `Initial_State` | 코어 3행 전부 `TRUE`/`Active` — 상수. 코어 자동 활성은 코드 계약이다 |
| `Status` (Draft/Review/Approved) | 기획 밸런싱 워크플로 컬럼. 런타임 의미 없음 |
| `Before` / `After` | 검산용. `Before`는 `DT_ActiveSkill`이 소유하고, 어긋나면 그쪽이 사실이다 |
| `Skill_Name` / `Base_Skill_Name` / `Evolution_Skill_Name` | `DT_ActiveSkill::Skill_Name`이 단일 출처 |
| `Exclusive_Group` | `Base_Skill_ID`에서 100% 도출된다 (실측 12건 불일치 0) — §스킬 진화 |
| `Description` | 기획 메모. UI 표시가 필요해지면 `FText`로 별도 추가 |
| **`Node_Connections` 테이블 전체** | 아래 |

### `Node_Connections`는 임포트하지 않는다

192행이 `Prerequisite_1/2`에서 도출한 엣지 집합과 **완전히 일치**한다(양방향 차집합 0). 시트 설명 자체가 "검증용"이라고 밝히고 있다.

**둘 다 임포트하면 선행 관계에 소유처가 둘 생긴다.** `Prerequisite_1/2`가 단일 출처이고, 이 시트는 기획 검토용 스프레드시트로만 유지한다. `From_Type`/`To_Type`/`Sector`/`Connection_Type`은 사람이 패턴을 눈으로 검토하기 위한 컬럼이며 판정에 쓰이지 않는다.

### 새 테이블로 분리한다 (스킬 테이블에 끼우지 않는다)

`DT_ActiveSkill`은 "스킬 수치"의 단일 출처이고 노드 그래프는 "진행 구조"다. 스킬 row가 없는 스탯 노드, 링·슬롯·비용을 스킬 테이블에 끼우면 컬럼이 계속 붙는다. 기존 `FLSCharacterSkillRow::Parent_Skill_ID`는 **스킬 계보 표기로만 남기고 그래프 판정에는 쓰지 않는다**(진화 대상은 `Evolution_Skill_ID`가 명시한다).

**테이블 참조는 `ULSGameDataSettings`에 추가한다.** `ULSGameDataSubsystem::LoadTables`(`LSGameDataSubsystem.cpp:387`)는 `ULSGameDataSettings`만 읽는다(`:389`). `ULSDropSettings`는 `ULSDropSubsystem`·`ULSSaveSubsystem`·`ULSCraftingWidget`이 `GetDefault<>()`로 직접 읽는 별개 계통이므로, 거기에 넣으면 서브시스템이 처음으로 두 설정 클래스를 교차하고 기존 "한 설정 클래스 → 한 서브시스템" 패턴이 깨진다.

**로드와 미설정 경고를 쌍으로 등록한다.** `LogMissingTables()`(`LSGameDataSubsystem.cpp:412`)에 노드 테이블 경고를 함께 넣는다. **테이블이 5개이므로 5쌍 전부** 넣어야 한다. `ComboAttackTable`이 로드는 되는데 경고 목록에서 빠져 있는 선례가 있다 — 같은 누락을 반복하지 않기 위한 기록이다.

조회 API는 `ULSGameDataSubsystem`에 붙인다. **개별 테이블 조회를 밖으로 열지 않고 통합 인덱스만 노출한다** — 호출부가 5테이블 구조를 알 필요가 없고, 알게 되면 Kind가 늘어날 때마다 호출부가 늘어난다.

```text
FindSkillNode(NodeKey)                -> const FLSSkillNodeRef*
GetSkillNodesForCharacter(CharacterID) -> TArray<FLSSkillNodeRef>
```

인덱스는 `LoadTables` 직후 1회 구축한다. Subsystem은 조회만 하고 GE를 적용하지 않는다(기존 책임 분리 유지).

### 링 해금 테이블 — 만들지 않는다 (추후)

`DT_SkillNodeRing`과 `Ring_Unlock_*` 컬럼은 **미정**이다. §먼저 확인할 것 1 참고.

---

## 노드 간 연결 구조

### 노드 키는 `FName`(RowName 자체)다 — 확정

기획이 `CORE-101` / `N101-S01` / `N101-E01` 형식을 확정했으므로 **`FName`으로 간다.** 다른 후보였던 `int32`는 쓸 수 없다.

> ⚠️ **`int32` + RowName 보정 패턴을 쓰면 전 노드 ID가 조용히 0이 된다.** `FLSCharacterSkillRow`·`FLSCharacterPassiveSkillRow`·`FLSComboAttackRow`가 쓰는 `Normalize*IDFromRowName` 헬퍼는 **RowName에 비숫자 문자가 하나라도 있으면 로그 없이 return**한다. `N101-S01`은 전부 비숫자를 포함하므로 100% 해당한다. 이 패턴을 노드에 복제하지 않는다(복제하면 4벌이 된다).
>
> 배경: UE CSV 임포터는 **헤더 이름과 무관하게 0번 컬럼을 RowName으로 소비**한다. 그래서 `int32` ID를 쓰려면 보정이 필요했던 것이고, `FName`을 쓰면 보정 자체가 불필요하다.

부수 이득이 셋 있다 — CSV에서 선행 관계가 눈으로 보이고, 조회가 `FindRow` fast path만으로 끝나고, 새 ID를 부여하다 겹치는 실수가 없다. 대가는 `Skill_ID` 등 다른 테이블 키가 `int32`라 어휘가 섞이는 것뿐이다.

### 선행 조건은 ANY다 — 컬럼 2개 고정

```text
CanActivateNode(N) =
      RingUnlocked(N.Ring)                        // 현재 항상 true (추후)
   && (선행이 둘 다 비어 있음
       || Prerequisite_1 이 Activated 에 있음
       || Prerequisite_2 이 Activated 에 있음)       ← ANY
   && 배타 충돌 없음                                 // 스킬 진화만 (§노드 타입별 구현 구조)
   && 비용 충족                                     // 칩 등급·개수 + 코인
```

기획의 *"하위 노드가 활성화 되어야 상위노드 활성화 가능"* + *"하위 노드가 2개인 상위 노드라면 1개만 활성화 되어도 활성화 가능"*이 정확히 ANY다. ALL이 아니다.

**선행은 배열이 아니라 `Prerequisite_1` / `Prerequisite_2` 두 컬럼 고정이다.** 실측 데이터의 선행 개수 최대치가 2이고, 기획 시트 구조가 그렇다. 배열을 쓰지 않으므로 `FString`에 배열 리터럴을 담아 죽은 데이터가 된 선례(`FLSCharacterSkillRow::Skill_Effects`)의 위험도 없다.

> 선행이 3개 이상 필요해지면 `Prerequisite_3`을 추가하는 것이 아니라 그때 `TArray<FName>`으로 전환한다. 판정 로직이 "활성인 선행이 하나라도 있는가"이므로 전환 비용은 컬럼 읽기 부분에 국한된다.

방사형이므로 구조는 트리가 아니라 **DAG**이다 — 서브스탯 하나가 인접한 메인스탯 2개를 선행으로 갖는 1차 링 분기가 실제로 그렇다. 단일 부모로는 표현할 수 없다.

**ANY의 결과로 최단 경로가 짧다.** 실측 위상에서 코어부터 진화 노드 하나까지는 `메인 → 서브 → 메인 → 강화 → 서브 → 진화` **6개**만 활성화하면 닿는다(캐릭터당 44개 중). 머지 노드가 전부 선행 2개인데 ANY라서 매번 한쪽만 타면 되기 때문이다. 트리를 넓게 파야 진화에 닿는 구조가 아니라 **한 섹터만 좁게 파고드는 러시가 성립한다.** 의도된 것이면 그대로 두고, 아니면 특정 노드만 ALL로 바꾸는 것이 아니라 비용으로 조절해야 한다(ALL을 섞으면 위 코어 연결성 귀납 증명은 유지되지만 판정 분기가 노드마다 갈린다) → 기획 확인 항목.

**코어는 선행이 비어 있는 유일한 노드**이며 시스템 활성 시 자동 활성이다(기획). 즉 "선행 비어 있음"은 코어를 위한 예외가 아니라 코어의 정의다. 실측에서도 선행 없는 노드는 코어 3개뿐이다.

### 코어 연결성 불변식은 자동으로 유지된다

기획의 *"모든 활성화되어있는 노드는 코어까지 연결되어있어야한다"*는 **활성화 경로만 존재하는 한 귀납적으로 성립한다.**

- 코어는 항상 활성이고 코어까지의 경로를 자명하게 갖는다.
- 노드 N을 활성화하는 시점에 N의 선행 둘 중 최소 하나(P)가 이미 활성이다(ANY 조건). P는 귀납 가정상 코어까지 경로를 가지므로, N도 P를 거쳐 경로를 갖는다.

→ **따라서 `ValidateCoreConnectivity`는 평시 활성화 판정에 쓰지 않는다.** 이 함수의 역할은 **세이브 로드 시 1회 무결성 검사**다. 방어 대상은 세 가지다.

1. 세이브 손상
2. 테이블 변경으로 활성 노드가 사라졌거나 선행 관계가 바뀐 경우
3. 리스펙(해제)이 도입된 경우

매 프레임·매 노드 판정에 그래프 순회를 돌리지 않는다. 성능 오해를 막기 위해 이 성격을 명시한다.

**리스펙이 들어오면 불변식이 깨질 수 있다.** 중간 노드를 해제하면 그 뒤 노드들이 고아가 된다. 연쇄 해제인지 해제 거부인지 정책이 필요하다 → 기획 확인 항목 참고.

### 데이터 무결성 검증 — 선례 0건, 전부 신규

프로젝트에 사이클 검출·도달성 검사 코드가 **하나도 없다**(`cycle`/`circular`/`acyclic`/`DFS` 전수 확인). 그래프 검증은 재사용할 것이 없으므로 전부 새로 쓴다.

검사 항목. **전부 통합 인덱스만 보고 돈다** — 개별 테이블을 순회하지 않는다.

| 검사 | 내용 |
|---|---|
| 키 중복 | 서로 다른 테이블이 같은 노드 키를 쓴다 (5분할 고유 위험) |
| 미존재 선행 | 선행이 인덱스에 없는 노드를 가리킨다 |
| 사이클 | 선행 관계를 따라가면 자기 자신에 도달한다 |
| 섬 | 코어에서 선행 역방향으로 도달할 수 없는 노드가 있다 |
| Ring 역행 | **선행 노드의 `Ring`이 대상 노드의 `Ring`보다 크다** |
| 캐릭터 교차 | 선행 노드의 `Character_ID`가 대상과 다르다 |

> ⚠️ **Ring 역행 검사는 동일 링을 허용해야 한다.** 실측에 같은 링 안의 연결이 120건 있고 전부 정상이다 — 1차 링의 메인↔서브 분기와 2차 링의 서브→메인 머지가 동일 링 안에서 일어난다. `>=`로 잡으면 정상 데이터의 62%가 경고로 뜬다.

> ⚠️ **사이클 검출은 반드시 유향으로 돈다.** 1차 링은 `S04`가 `M04`·`M01`을 선행으로 갖는 랩어라운드 때문에 **무향으로 보면 8각 닫힌 고리**다. 연결선을 그릴 때처럼 방향을 버리고 순회하면 정상 데이터 3건(캐릭터당 1건)이 사이클로 잡힌다. 유향으로는 순환이 없다(실측 0건). 섬 검출은 코어에서 선행 **역방향**으로 내려가면 되고, 여기서도 무향 순회를 쓰지 않는다.

여섯 검사를 현재 데이터에 실제로 돌려 전부 통과함을 확인했다(§노드 데이터 실측). 즉 이 검사들은 지금 깨진 것을 잡기 위한 것이 아니라 **앞으로 기획이 트리를 편집할 때의 안전망**이다.

방식은 `ULSDropSubsystem::ValidateGroupReferences`(`LSDropSubsystem.cpp:97`, 선언·호출 양쪽 `#if WITH_EDITOR` 가드) 패턴을 따른다 — **경고 로그만 남기고 동작은 바꾸지 않는다.** 프로젝트에 `check()`/`ensure()`/`UE_LOG(Fatal)` 사용례가 0건이므로 여기서도 쓰지 않는다.

런타임(패키지) 쪽은 그래프 순회 검증을 돌리지 않고, 개별 노드 로드 시 skip-and-warn으로 방어한다(§노드 타입별 구현 구조).

### 연결선은 선행 관계가 겸한다

`Prerequisite_1/2`가 곧 연결선 정의다 — 기획서의 "파란 선" 하나가 선행 관계 하나다. 실측에서 이 도출 결과가 기획의 `Node_Connections` 시트 192행과 완전히 일치하므로, **위젯이 그릴 선의 목록도 여기서 나온다.** 별도 연결선 데이터를 만들지 않는다.

장식용 연결선(선행 의미가 없는 선)은 지금 없다. 필요해지면 그때 컬럼을 추가한다.

---

## 노드 타입별 구현 구조

### Kind별 해석 결과

테이블이 Kind를 결정하므로 각 Row 구조체는 자기 Kind의 컬럼만 갖는다. 이게 5분할의 가장 큰 이득이다 — "이 Kind에서 의미 없는 컬럼"이 구조체에 존재하지 않는다.

| Kind | 고유 컬럼 | 해석 결과 |
|---|---|---|
| MainStat / SubStat | `Stat_Field`, `Operation`, `Value` | `StatModifiers`에 누적 |
| SkillEnhance | `Skill_ID`, `Parameter_Field`, `Operation`, `Value` | `SkillRatios[Skill_ID]`에 누적 |
| SkillEvolve | `Base_Skill_ID`, `Evolution_Skill_ID` | `Evolutions[Base_Skill_ID]` 설정 |
| Core | 없음 | 해석 결과 없음. 활성 상태만 갖는다 |

**MainStat과 SubStat은 해석·적용 경로가 완전히 같다.** 차이는 수치 크기·비용·UI 표현(큰 원 / 작은 원)뿐이라 Row 구조체를 공유하고, 해석 분기도 하나로 합친다. 테이블을 둘로 나누는 이유는 UI가 도형을 구분하고 기획 편집 단위가 시트로 갈리기 때문이다.

> 기획서 본문의 **"핵심 노드"**(빌드에 영향이 큰 중요 노드)에 해당하는 컬럼이 **데이터 시트에 없다.** 지금 `bKeyNode`를 만들지 않는다. UI 강조가 필요해지면 그때 공통 컬럼에 `bool` 하나를 추가한다 — 해석에는 영향이 없다.

### Kind별 로드 검증 (skip-and-warn)

`ULSCraftingWidget::LoadRecipes`(`LSCraftingWidget.cpp:139`) 패턴을 따른다 — **잘못된 row는 전체 로드를 실패시키지 않고 그 row만 버리고 경고한다.**

공통:

- `Character_ID`가 `ULSSkillPoolDataAsset`에 없는 값 → skip + warn
- `Chip_Grade`가 `GetKnownGrades()`에 없는 토큰 → skip + warn
- `Required_Count`/`Required_Coin`이 음수 → skip + warn

Kind별:

- Stat: `Stat_Field`가 `FLSCharacterStatRow` 필드명이 아니거나 어트리뷰트로 매핑 불가 → skip + warn
- Stat: `Value`가 0이거나 음수 → skip + warn (어트리뷰트에 클램프가 없어 그대로 통과하므로 여기서 막는다)
- Enhance: `Parameter_Field`가 `None`이거나 `Value`가 0 → skip + warn
- Enhance / Evolve: `Skill_ID`(또는 `Base_Skill_ID`)가 `DT_ActiveSkill`에 없음 → skip + warn
- Evolve: `Evolution_Skill_ID`가 `DT_ActiveSkill`에 없음 → skip + warn
- Evolve: `Evolution_Skill_ID`의 `Parent_Skill_ID`가 `Base_Skill_ID`와 다름 → **warn만** (실측 12건 전부 일치하지만, 어긋나면 노드 테이블과 스킬 테이블 중 어느 쪽이 사실인지 사람이 판단해야 한다)

### 스탯 노드 (MainStat / SubStat)

**해석:** `StatModifiers`에 `{Stat_Field, 합산 Value}`를 누적한다. 같은 `Stat_Field`는 합산한다.

#### 스탯 → 어트리뷰트 매핑 (실측 11종)

| `Stat_Field` | 어트리뷰트 | `ULSGE_ChipStats` 등록 | 노드 수 |
|---|---|---|---|
| `Char_Attack` | `Attack` | ✓ | 20 |
| `Char_Health` | `MaxHealth` (`ULSCombatAttributeSet`) | ✓ | 17 |
| `Char_Cal` (스킬가속) | `CooldownReduction` | ✓ | 14 |
| `Char_Crit` | `CritChance` | ✓ | 10 |
| `Char_Atkspeed` | `AttackSpeed` | ✓ | 9 |
| `Char_CritDmg` | `CritDamage` | ✓ | 8 |
| `Char_ArmorPen` | `ArmorPenetration` | ✓ | 7 |
| `Char_Defence` | `Defence` | ✓ | 7 |
| `Char_Speed` | `MoveSpeed` | ✓ | 7 |
| `Char_Recovery` | `Recovery` | ✓ | 4 |
| `Char_Poise` | `Tenacity` | ✗ | 5 |

`Char_Recovery`는 기획 시트가 `Char_HP_Recovery`로 적었으나 코드 필드명은 `Char_Recovery`다. `Char_Poise`는 기존 `ULSGE_ChipStats`에는 없으므로 노드 전용 `ULSGE_SkillNodeStats`에 새 모디파이어를 등록한다.

#### 결정: ChipStat 패턴을 복제한다

`ULSGE_ChipStats` 생성자가 등록하는 모디파이어 10개에 `Tenacity`를 더하면 노드가 필요한 **11개**와 일치한다. 사전 정의 무한 GE + SetByCaller 관용구를 그대로 복제하고, 노드 전용 GE에만 강인도 모디파이어를 추가한다.

```text
ULSGE_SkillNodeStats        사전 정의 무한 GE (LSGE_ChipStats 형태)
ULSSkillNodeStatComponent   SetByCaller 채우고 적용 (ULSChipStatComponent 형태)
```

대안이었던 `ResolveAttribute` 추출·공유는 채택하지 않는다. `ULSStatusEffectComponent::ResolveAttribute`(`LSStatusEffectComponent.cpp:45`)가 처리하는 것이 Attack / AttackSpeed / MoveSpeed / Defence **4개뿐**이라 어차피 확장이 붙고, 공용 코드를 건드리는 리팩터까지 따라온다. 검증된 관용구를 복제하는 쪽이 명백히 싸다.

> 대가는 프로젝트의 병렬 어트리뷰트 매핑이 **5번째**가 되는 것이다 — 기존 4개는 `ResolveAttribute` / `ULSGE_ChipStats` / `ULSGE_EquipmentStats` / `ULSGA_CombatAccelerationPassive::AccumulateCombatAccelerationModifier`(같은 `Char_*` 문자열을 독립 재구현). 스탯을 추가할 때 GE 생성자와 컴포넌트 양쪽을 고쳐야 한다. 이 부채는 인지하고 감수한다.

#### `Operation`은 둘 다 Additive 모디파이어로 구현한다

실측에서 `Multiply`를 쓰는 스탯은 `Char_Atkspeed`와 `Char_Speed` **둘뿐**이고, 나머지 9개는 전부 `Add`다.

그리고 **그 두 스탯의 기저값이 정확히 `1.0`이다**(`FLSCharacterStatRow::Char_Atkspeed = 1.0f`, `Char_Speed = 1.0f`). 따라서 `Multiply 0.06`을 Additive 모디파이어 `+0.06`으로 넣으면 결과가 `1.06`으로 같다. 별도 곱연산 경로가 필요 없다.

다중 스택 시에만 어긋난다 — 가산 6개 누적은 `1 + ΣV`, 곱연산은 `Π(1 + V)`로 1% 미만 차이가 난다. **가산을 택한다.** 칩·장비 스탯이 이미 `Additive`이고, 같은 어트리뷰트에 두 수식이 섞이면 UI 예상치와 실제가 어긋난다.

> `ELSStatusEffectMathType`(`Flat`/`Percent`)을 재사용하지 않는다. StatusEffect의 `Percent`는 곱연산(`Multiplicitive`, 1+f)이고 칩·장비의 `Percent`는 가산(`Additive`, ÷100)이라 같은 이름이 두 수식을 가리킨다. 노드는 기획 어휘(`Add`/`Multiply`)를 자기 enum으로 갖고, 적용은 위 규칙으로 고정한다.

`Unit`(정수 / % / %p)은 **UI 표시 전용**이다. 계산에 쓰지 않는다 — `Value`가 이미 최종 수치다(기획 규칙: 백분율은 소수 입력).

#### 남은 제약 4개

| 제약 | 내용 |
|---|---|
| MaxHealth 함정 | `Char_Health` 노드가 17개다. 현재 체력 보존·클램프 로직이 필요하다. 선례가 둘 있다 — `ULSChipStatComponent`(`.cpp:110`~`:125`), `ULSEquipmentStatComponent`(`.cpp:99`~`:111`). 없으면 노드 적용이 회복 수단이 되거나 현재 체력이 최대치를 넘는다 |
| 클램프 없음 | 스태미나 외 어트리뷰트에 클램프가 전혀 없다. 음수·과대값이 그대로 통과하므로 로드 검증에서 막아야 한다 |
| 대부분 비복제 | `Attack`/`Defence`/`MoveSpeed` 등은 `DOREPLIFETIME`에 없다(스태미나·체력만 복제). 노드 UI에 "현재 공격력 +15"를 띄우려면 어트리뷰트 복제를 추가하거나 세이브에서 직접 계산해야 한다 |
| 정수 표기와 소수값 | `Unit`이 `정수`인데 `Value`가 소수인 row가 16건이다. 런타임은 `float`이라 통과하지만 UI 표시 규칙(내림·반올림)을 정해야 한다 → 기획 확인 항목 |

**적용 순서**: `ALSPlayerCharacter::BeginPlay`의 `InitializeBaseAttributes`(`LSPlayerCharacter.cpp:131`) → `RefreshChipStats`(`:136`) → `RefreshEquipmentStats`(`:142`) 체인에 끼운다. 노드는 영구 스탯이므로 **칩보다 앞**이 논리적이다. (컴포넌트 자체 `BeginPlay`에서는 못 한다 — ASC `InitAbilityActorInfo` 완료 후여야 한다.)

### 스킬 강화 (SkillEnhance)

**`Parameter_Field`는 `FLSCharacterSkillRow`의 실제 필드명을 그대로 쓴다.** 기획이 이미 그렇게 적었고, 별도 어휘를 두면 매핑 계층이 하나 더 생긴다.

```text
None / Skill_Multiplier / Range_X / Skill_Cooldown
```

`None`을 첫 값으로 두는 것은 CSV 셀이 빌 수 있는 컬럼 enum의 프로젝트 관례다. `Skill_HitCount`는 **지금 넣지 않는다** — 데이터에 노드가 없고, 넣으면 ShortCircuit 외에서 조용히 무효가 된다(§먼저 확인할 것 3).

`Range_X`만 대상이다. `Range_Y`(각도·폭)와 `Range_Z`(ShortCircuit 투사체 아크 높이)는 건드리지 않는다 — 기획이 반경·길이만 늘리기로 했고, Y를 함께 늘리면 원뿔형 스킬의 각도가 같이 벌어져 체감이 달라진다.

**`Operation`은 실측 12건 전부 `Multiply`다.** 쿨타임 감소는 `Value`가 음수(`-0.2` = 20% 감소)로 표현된다.

비율을 택한 것이 가산보다 낫다 — 0값 필드에서 수치가 튀지 않는다. **다만 0값 함정 자체는 남는다**(강화가 조용히 무효가 된다). 경고 로그가 실제 안전장치다 → §함정 1.

**누적 규칙은 아직 필요하지 않다.** 실측에서 같은 `(Character_ID, Skill_ID, Parameter_Field)` 조합이 **중복 0건**이다 — 캐릭터당 강화 노드 4개가 서로 다른 (스킬, 필드)를 건드린다. 중복이 생기면 그때 규칙을 정한다: 비율이므로 합산(`1 + ΣV`)과 연쇄 곱(`Π(1+V)`)이 갈리고, 쿨타임 감소가 여러 개 겹치면 연쇄 곱이 아니면 음수 쿨타임이 나올 수 있다.

### 스킬 진화 (SkillEvolve)

**`Base_Skill_ID`가 곧 배타 그룹이다.** 같은 `Base_Skill_ID`를 가진 진화 노드 중 1개만 활성 가능하다(기획: *"동일한 스킬은 1개만 활성화 가능하며, 중복 활성화가 불가능"*). 실측도 6그룹 × 2노드 구조다.

**따라서 기획 시트의 `Exclusive_Group` 컬럼을 만들지 않는다.** 값이 `EVO-{Character_ID}-{Base_Skill_ID}` 형식이고 실측 12건이 **100% 이 규칙으로 도출**된다(불일치 0). 게다가 `DT_ActiveSkill::Parent_Skill_ID`로도 같은 그룹이 도출된다. 컬럼을 남기면 같은 사실에 소유처가 셋이 된다.

기획서의 **"분기"** 정의(*"둘 이상의 노드 중 선택하는 지점 — 능력치, 스킬 특화 선택 유도"*)가 스탯에도 하드 배타를 요구하는지는 여전히 애매하다. **현재 해석: 하드 배타는 스킬 진화뿐이고, 나머지 분기는 재화 제약에 의한 사실상의 선택이다.** 데이터도 이 해석과 일치한다 — 배타 그룹 컬럼이 진화 시트에만 있다. 스탯에도 하드 배타가 필요하면 스탯 Row에 배타 그룹 컬럼을 추가해야 한다 → 기획 확인 항목 참고.

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
3. 지정된 칩 슬롯의 등급이 Chip_Grade 조건을 만족하는지 확인 → Required_Count 개만큼 복사본에서 제거
4. Gold >= Required_Coin 확인 → 복사본에서 차감
5. 전부 성공했을 때만 커밋 + ActivatedNodeIDs 추가 + Save() + 델리게이트 발행
```

칩 제거는 슬롯 인덱스 지정이므로 `RemoveItemsFromSlotArray`를 쓰지 않는다(칩을 스킵한다 — §먼저 확인할 것 2).

---

## ③ 순수 해석 — `LSSkillNodeResolve`

`LSChipStats`와 같은 위치·같은 성격(정적 함수 모음, UObject 아님)으로 둔다.

```text
FLSSkillRowRatio            스킬 강화 누적 결과 — 비율이다(가산 델타가 아니다)
├── MultiplierRatio         Skill_Multiplier 에 곱할 (1 + ΣV)
├── RangeXRatio             Range_X 에 곱할 값
└── CooldownRatio           Skill_Cooldown 에 곱할 값 (감소는 V<0)

FLSCharacterNodeTotals      ← 복제 대상 플랫 스냅샷
├── StatModifiers           메인/서브 스탯 → GE. 키 = Stat_Field, 값 = 합산 Value
├── SkillRatios             키 = 기본 Skill_ID
└── Evolutions              기본 Skill_ID → 진화 Skill_ID (배타이므로 1:1)

Resolve(Activated, Index) -> FLSCharacterNodeTotals
CanActivateNode(NodeKey, Activated, Index, OwnedChips, Gold, OutBlockedReason) -> bool
RingUnlocked(Ring) -> bool                    // 자리표시자, 현재 항상 true (추후)
ValidateCoreConnectivity(Activated, Index) -> bool   // 세이브 로드 시 1회
```

`Index`는 §① 정적 정의의 통합 인덱스다. **해석 계층은 테이블이 5개라는 사실을 모른다** — 5분할을 인덱스 뒤에 가두는 것이 이 구조의 핵심이다.

`HitCountRatio`는 두지 않는다 — 데이터에 노드가 없다(§스킬 강화).

**`CanActivateNode`는 UI와 서버 검증이 같은 함수를 쓴다.** 기획의 노드 상태 3종(잠김 / 활성 가능 / 활성)과 *"필요 조건 표기 예) 선행 노드를 활성화 해야합니다"*가 `OutBlockedReason`(`FText`)으로 나온다. 칩 프로토콜의 `IsProtocolUnlockVisible`이 UI 가시성과 판정을 공유하는 것과 같은 구조다. 판정 규칙은 §노드 간 연결 구조가 소유한다.

**`RingUnlocked`는 지금 항상 `true`를 반환하는 자리표시자다.** 링 해금 조건이 미정이므로(§먼저 확인할 것 1) 판정 지점만 만들어 두고 내용은 나중에 채운다. 이렇게 해야 조건이 정해질 때 `CanActivateNode`의 구조를 건드리지 않는다.

**진화와 강화가 함께 걸릴 때 순서:** 진화로 최종 Skill_ID를 확정한 뒤, **기본 Skill_ID를 키로 하는 비율**을 적용한다. 즉 진화해도 강화가 유지된다. (기획 확인 항목 — 진화 시 강화를 초기화하려면 비율 키를 최종 ID로 바꾼다.)

**`ValidateCoreConnectivity`는 평시 판정이 아니라 세이브 로드 시 1회 무결성 검사다.** 활성화 경로만 있으면 불변식이 귀납적으로 자동 유지되기 때문이다 — 근거와 방어 대상은 §노드 간 연결 구조가 소유한다.

---

## ④ 적용

### (a) 스탯 노드 → 어트리뷰트 무한 GE

`ULSGE_SkillNodeStats`(사전 정의 무한 GE) + `ULSSkillNodeStatComponent`(SetByCaller 채움). `ULSGE_ChipStats` / `ULSChipStatComponent` 패턴의 복제다. 매핑 표·연산 규칙·남은 제약은 §노드 타입별 구현 구조가 소유한다. → [ChipSystem.md](ChipSystem.md)

### (b) 스킬 강화 → 리졸버 비율 적용

`ULSPlayerSkillComponent::ResolveActiveSkillRow`를 값 반환 리졸버로 바꾸고, 테이블 row를 복사한 뒤 `SkillRatios`를 **곱해서** 반환한다. (나)(다) 두 우회 지점을 이 리졸버로 리다이렉트한다. §함정의 4개 계약을 모두 지킨다.

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
| 노드 좌표 배치 | 동적 `UCanvasPanel` + `AddChildToCanvas` / `SetPosition` (선례: `ULSProtocolDebugWidget`). 좌표는 데이터에 없고 위젯이 계산한다 → §노드 배치 좌표 |
| 우측 상세 패널 | `ULSSkillLoadoutWidget::SelectedSlotEntry` 패턴(선택 항목을 전용 엔트리 위젯에 표시) |
| 칩 인벤토리 목록 | `ULSChipStationWidget::RefreshChipSlots`(인벤토리+창고에서 칩만 수집·정렬) + 등급 필터 추가 |
| 첫 프레임 레이아웃 튐 방지 | 루트를 `ULSLayoutRevealWidget` 상속 |

미니맵 헬퍼들은 전부 `private`이고 "미니맵 원 안"이라는 좌표계를 전제하므로, 직접 호출이 아니라 **패턴 복사**가 현실적이다.

### 노드 배치 좌표

> ⚠️ **`Slot`에서 각도를 계산할 수 없다.** 이전 판의 이 문서는 "`Ring` + `Slot`에서 방사형 좌표를 계산한다"고 썼는데, 실측값을 보면 `Slot`은 정수가 아니라 `CORE` / `R1-01` / `R2-M03` / `R3-E01` 같은 **라벨 문자열**이고 번호가 **종류별로 따로 1부터 다시 시작한다.** 사전순 정렬하면 2차 링이 `서브 12개 → 강화 4개 → 메인 8개` 순으로 뭉쳐서, 실제 배치(메인·서브 교대)와 전혀 다른 그림이 나온다.

**반지름은 `Ring`이 준다. 각도는 데이터에 없다.** 각도를 얻는 길이 둘이다.

| 방법 | 내용 | 판단 |
|---|---|---|
| **기획에 정수 각도 요청** | `Slot` 값을 링 내 0-기준 각도 인덱스(정수)로 바꿔 달라고 한다. 각도 = `SlotIndex / 링 노드 수 × 360°` | **권장.** 컬럼 추가 없이 값 표기만 바뀌고, 배치 통제권이 기획에 남는다 |
| 그래프에서 유도 | 코어의 자식 순서로 섹터를 나누고, 섹터 안에서 선행 깊이·`Slot` 순번으로 각도를 배분 | 아래 대칭성이 **깨지는 순간 배치가 무너진다.** 폴백으로만 |

유도가 위험한 이유는 현재 데이터가 우연히 완전 대칭이라는 데 있다. 세 캐릭터 모두 동일하게:

```text
코어 자식 4개 = 섹터 4개 (90°씩)
  섹터당 노드 11개 — 1차 1 / 2차 6 / 3차 3 + 경계 공유 서브 1
링 노드 수     1차 8(45°) / 2차 24(15°) / 3차 12(30°)
```

섹터가 5개가 되거나 캐릭터별로 분기 수가 달라지면 "코어 자식 수로 360°를 나눈다"는 규칙이 바로 깨진다. 각도는 기획이 데이터로 주는 편이 안전하다.

**섹터는 완전히 분리되지 않는다.** 섹터 간 자손 중첩은 1차 링 서브스탯 4개뿐이며(예: `S01`이 `M01`·`M02` 양쪽 섹터에 속한다), 이 4개가 1차 링을 **닫힌 8각 고리**로 만든다 — `S04`는 `M04`와 `M01`을 선행으로 갖는 랩어라운드다. 각도 배분 시 이 4개는 두 섹터의 경계(45° 지점)에 놓아야 한다.

**같은 링 안에 선행 깊이가 3단 있다.** 2차 링은 `서브 → 메인 → 강화`로 한 링 안에서 두 번 머지한다. 24개를 모두 같은 반지름에 놓으면 이 연결선이 원주를 따라 흘러 노드 원과 겹친다. → **링 내 선행 깊이만큼 반지름을 미세 가산**한다. 깊이는 그래프에서 나오므로 데이터가 필요 없다.

| 링 | 링 내 흐름 |
|---|---|
| 1차 | 메인 4 → (인접 두 메인을 머지하는) 서브 4 |
| 2차 | 서브 12 → (두 서브 머지) 메인 8 → (두 메인 머지) 강화 4 |
| 3차 | 서브 8 → (두 서브 머지) 진화 4 |

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

## 결정 요약

기획 데이터테이블이 나오면서 이전에 보류였던 항목이 전부 확정됐다. 각 결정의 근거는 해당 절이 소유하고, 여기서는 목록만 둔다.

| 결정 | 내용 | 근거 |
|---|---|---|
| 테이블 분할 | **Kind별 5테이블 / Row 구조체 4개** + 통합 인덱스 | §① 정적 정의 |
| 노드 키 | **`FName`**(RowName 자체) | §노드 간 연결 구조 |
| 선행 조건 | `Prerequisite_1` / `Prerequisite_2` 2컬럼, **ANY** | §노드 간 연결 구조 |
| 연결선 데이터 | `Node_Connections` 시트는 **임포트하지 않는다** | §① 정적 정의 |
| 스탯 GAS 적용 | **ChipStat 패턴 복제** (`ULSGE_SkillNodeStats` + 컴포넌트) | §스탯 노드 |
| 스탯 연산 | `Add` / `Multiply` 둘 다 **Additive 모디파이어**로 구현 | §스탯 노드 |
| 강화 연산 | **비율**(곱). 쿨타임 감소는 음수 `Value` | §스킬 강화 |
| 강화 대상 필드 | `Skill_Multiplier` / `Range_X` / `Skill_Cooldown` 3종. `Skill_HitCount` 제외 | §스킬 강화 |
| 배타 그룹 | `Base_Skill_ID`가 겸한다. 컬럼 만들지 않음 | §스킬 진화 |
| 링 해금 | **추후 개발 예정.** `RingUnlocked()` 자리표시자 | §먼저 확인할 것 1 |
| 노드 좌표 | `Pos_X`/`Pos_Y` 없음. 반지름은 `Ring`, **각도는 기획에 정수 요청** | §노드 배치 좌표 |

미확정으로 남은 것은 전부 **기획 결정**이며 §기획 확인 항목에 있다. `Char_Poise`는 `Tenacity` Attribute로 적용 경로가 확정됐다.

---

## 코드 관례

새 Row·네임스페이스를 만들 때 따를 것. AGENTS.md가 이미 규정한 일반 규칙은 반복하지 않고 **이 테이블에 특유한 부분만** 적는다.

- **Row 구조체 4개를 `Source/LostSignal/Data/LSSkillNodeRow.h` 한 헤더에 담는다.** `.cpp` 없음(`Data/` 21개 Row가 예외 없이 그렇다). 한 헤더에 Row 여럿을 두는 선례는 `LSConsumableRow.h`(2개)다. include 순서 `CoreMinimal.h` → `Engine/DataTable.h` → `.generated.h`
- ⚠️ **Row가 Row를 상속하지 않는다.** `Data/`의 21개가 전부 `FTableRowBase` 직접 상속이며 선례가 0건이다. 공통 컬럼 9개는 4개 구조체에 반복해서 적는다(§① 정적 정의)
- 모든 UPROPERTY는 예외 없이 `EditAnywhere, BlueprintReadOnly, Category="LS/SkillNode[/서브]"` + **스칼라 기본값 초기화자**. `Data/*Row.h`에 `EditDefaultsOnly`/`BlueprintReadWrite` 사용례가 0건이다
- ⚠️ **Category 구분자**: UE는 `|`만 계층으로 처리하고 `/`는 카테고리 이름의 리터럴 문자로 본다. 즉 `"LS/SkillNode/Cost"`는 평평한 이름 하나가 된다. 그래도 `Data/*Row.h` 이웃 다수가 `/`를 쓰므로 **이웃 일관성을 따른다**(Diff 최소화). 계층이 실제로 만들어지지 않는다는 사실만 알고 쓴다
- enum(`ELSSkillNodeKind`, `ELSSkillNodeStatOperation`, `ELSSkillNodeParameterField`)은 **같은 헤더 최상단**, `UENUM(BlueprintType)` + `: uint8` + **`None`을 첫 값으로**. CSV 셀이 빌 수 있는 컬럼 enum은 `None`으로 시작하는 것이 프로젝트 관례다
  - `ELSSkillNodeKind`는 **컬럼이 아니다.** 통합 인덱스가 "이 노드가 어느 테이블에서 왔는지"를 담는 데만 쓴다(§① 정적 정의)
- `LSSkillNodeResolve`는 `Data/` 아래 **`namespace`**(프로젝트에 `class { static }` 방식이 0건), 함수마다 `LOSTSIGNAL_API`. `.cpp`에서는 `namespace`를 다시 열어 정의한다(한정자 방식이 아니다). 파일 로컬 헬퍼는 명명된 네임스페이스 앞의 익명 `namespace`에 두고 `SkillNode` 도메인 접두사를 붙인다
  - ⚠️ `LSDropSubsystem.cpp`에 접두사 없는 `ExtractRowNamePrefix`가 있다. 유사한 RowName 파싱 헬퍼를 만들 때 같은 이름을 쓰면 유니티 빌드에서 중복 정의로 터진다
- `FLSCharacterNodeTotals`만 `USTRUCT`(복제 대상). `FLSSkillRowRatio`와 `FLSSkillNodeRef`는 plain struct로 충분하다(`FLSChipProtocolTotals`가 같은 선례)
- 테스트는 `Tests/LSSkillNodeTests.cpp`, `IMPLEMENT_SIMPLE_AUTOMATION_TEST` + 경로 `"LostSignal.SkillNode.<동작>"` + `EditorContext | EngineFilter`. **build.cs 수정 불필요** — `Tests/`의 `.cpp`는 일반 모듈 소스로 컴파일된다. 테스트 대상 헤더 include는 `#if WITH_DEV_AUTOMATION_TESTS` **밖**에 둔다(TU가 비지 않게)
- **로그는 한국어**, 접두사 `[SkillNode]`. 최근 작성된 제작·세이브 코드가 영어로 드리프트한 상태이므로 명시해 둔다

---

## 구현 순서

계층 순으로 간다. 각 단계가 앞 단계의 계약만 알면 되도록 한다.

```text
Phase 1  데이터·해석 (게임플레이 영향 0)
  1. LSSkillNodeRow.h — Row 구조체 4개 + enum, CSV 5개 임포트
     ULSGameDataSettings에 테이블 참조 5개 (+ LogMissingTables 경고 5쌍)
  2. 통합 인덱스 구축 + ULSGameDataSubsystem 조회 API 2개
     Kind별 로드 검증(skip-and-warn) — 키 중복 포함
  3. 그래프 무결성 검증 (WITH_EDITOR — 6종. Ring 역행은 동일 링 허용)
  4. LSSkillNodeResolve + 단위 테스트

Phase 2  세이브·소비
  5. ULSSaveGame / ULSSaveSubsystem 노드 진행 저장 (코어 3개는 기본 활성)
  6. 칩 + 골드 원자적 소비 (칩은 슬롯 인덱스 지정)

Phase 3  적용
  7. 스킬 진화: FindSkillByID 재귀 탐색 + ApplyEquippedSkillLoadout 치환
  8. 스킬 강화: ResolveActiveSkillRow 값 반환 전환 + 비율 곱, (나)(다) 리다이렉트
  9. 스탯 노드: ULSGE_SkillNodeStats + ULSSkillNodeStatComponent (스탯 11종)
 10. FLSCharacterNodeTotals 복제 배선 (MO 대비)

Phase 4  UI
 11. ELSLobbyPanel 값 추가 + 패널 등록(3곳 쌍)
 12. 노드 그래프 위젯 (방사형 좌표, 연결선, 상태 3종 시각화)
 13. 노드 클릭 확대 + 우측 상세 패널 + 재료 슬롯 칩 드롭
 14. 진화 노드 마우스오버 스킬 정보/영상 (영상 재생 방식은 별도 확인)

추후  링 해금 조건 (RingUnlocked 자리표시자를 채운다)
```

Phase 1~2는 게임플레이에 영향을 주지 않으므로 먼저 넣고 테스트로 굳힐 수 있다. **구조 결정이 전부 확정됐으므로 Phase 1을 바로 시작할 수 있다.**

Phase 3-9는 `Char_Poise`를 포함한 스탯 11종을 구현한다.

---

## 검증 계획

**③ 해석 계층 — 자동화 테스트** (`Tests/LSProtocolUnlockTests.cpp` 형식)

연결 판정:
- 선행 ANY 시맨틱: 선행 2개 중 1개만 활성 → **통과**해야 한다(ALL로 잘못 구현하면 여기서 걸린다)
- 선행이 하나도 활성이 아니면 거부. 선행이 둘 다 빈 노드(코어)는 선행 검사 없이 통과
- 칩 등급 부족 거부 / 골드 부족 거부 / 이미 활성인 노드 재활성 거부
- 같은 `Base_Skill_ID`의 진화 노드 2개 동시 활성 거부

노드별 해석:
- 강화 비율 누적: 같은 `(Skill_ID, Parameter_Field)` 노드 여러 개 → 정한 규칙대로 결합
- 스탯 누적: 같은 `Stat_Field` 여러 개 → 합산
- `Multiply` 스탯이 Additive 모디파이어로 기저값 `1.0` + `Value`가 되는지
- Kind별 검증: 필수 컬럼이 빈 row가 skip되고 나머지 row는 정상 로드되는지(전체 실패가 아니어야 한다)
- `Char_Poise` row가 `Tenacity`에 누적되고 나머지 10종과 함께 정상 적용되는지

그래프 무결성:
- 사이클이 있는 데이터 → 검출
- 코어에서 도달 불가한 섬 → 검출
- 미존재 노드를 선행으로 참조 → 검출
- Ring 역행 → 검출. **동일 링 연결은 검출되지 않아야 한다**(정상 데이터 120건)
- 서로 다른 테이블의 노드 키 중복 → 검출
- 코어 연결성: 정상 활성 집합은 통과, 중간 노드를 강제로 빼면 위반 검출

**현재 데이터를 기준선으로 쓴다.** 실측 135노드 / 192엣지가 6종 검사를 전부 통과함을 확인했다(§노드 데이터 실측). 검증 코드를 짠 뒤 이 데이터를 넣어 **경고가 0건**이면 검증 자체가 옳다는 근거가 된다. 경고가 뜨면 검증 코드를 먼저 의심한다.

**스킬 강화 일관성 — 가장 중요한 수동 검증.** 훅이 (가) 한 곳으로 모였는지를 실제로 확인하는 테스트다.

- 범위 강화 노드 활성 후 **프리뷰 메시 크기와 실제 판정 범위가 일치**하는가
- 쿨타임 강화 후 **UI 숫자와 실제 GE 지속시간이 일치**하는가
- ShortCircuit 필드 반경·타수, Bypass 홀로그램 풀 반경도 바뀌는가((나)(다) 리다이렉트 검증)

**0값 함정 재현.** `DT_ActiveSkill`에서 값이 0인 필드를 대상으로 하는 강화 노드를 일부러 만들어, `LogLS` 경고가 뜨는 것을 눈으로 확인한다. 비율이므로 강화는 무효가 되는 것이 정상이며, **무효가 되는 것을 알 수 있는지**가 핵심이다.

**원자성 — 자동화 테스트.** 골드는 충분하지만 칩이 없는 상태로 활성화 시도 → 골드가 차감되지 않아야 한다. 반대 경우도 확인한다. 복사본 시뮬레이션 방식의 소비를 테스트하는 선례가 `Tests/LSCraftingTests.cpp`에 있으므로 그 형식을 따른다.

**세이브 영속.** 노드 활성 → 게임 종료 → 재시작 후 유지. `ULSSaveSubsystem`의 JSON 디버그 export에 노드 진행을 추가해 눈으로 확인한다.

**UI 드롭은 패키지 빌드에서 확인한다.** `UIDragDropPackagedBuild.md`의 문제는 에디터에서 재현되지 않는다.

---

## 기획 확인 항목

1. **`Char_Poise`(강인도) 노드 5개 — 해소됨.** `ULSCharacterAttributeSet::Tenacity`에 가산한다. 플레이어는 별도 DataTable 컬럼 없이 기본값 `1`, 몬스터는 `Monster_Guard`로 초기화한다. 슈퍼아머·무적 태그는 각각 유효 강인도를 최소 `4`·`6`으로 보정한다.
2. **`Char_HP_Recovery` 표기** — 코드 필드명이 `Char_Recovery`다. 테이블을 그쪽에 맞추면 된다(단순 수정).
3. **`Unit`이 `정수`인데 값이 소수인 row 16건** — 런타임은 `float`이라 통과한다. UI에 어떻게 표시할지(내림 / 반올림 / 소수 그대로)를 정해야 한다.
4. **"분기"가 스탯에도 하드 배타인가** — 기획서의 *"분기 = 둘 이상의 노드 중 선택하는 지점, 능력치·스킬 특화 선택 유도"*를 현재는 "재화 제약에 의한 사실상의 선택"으로 해석했다. 데이터도 이 해석과 일치한다(배타 그룹 컬럼이 진화 시트에만 있다). 스탯 분기도 하드 배타여야 하면 스탯 Row에 배타 그룹 컬럼을 추가해야 한다.
5. **"노드 교체"의 의미** — 리스펙(해제)이 있는가? 있으면 환불 규칙과, 해제로 생긴 고아 노드 처리 정책(연쇄 해제 vs 해제 거부)이 필요하다. 기획서 비용 섹션에 환불 언급이 없다. **칩은 소모되면 인스턴스 스탯째로 사라지므로 환불이 원본 복구가 아니다** — 등급만 돌려줄지도 정해야 한다.
6. **진화 후 강화 유지 여부** — 기본 스킬에 걸린 강화가 진화 후에도 유지되는가(권장), 초기화되는가? 비율 키를 기본 ID로 둘지 최종 ID로 둘지가 갈린다.
   - 데이터가 유지 쪽을 지지한다 — 진화 row의 `Range_X`·`Skill_Cooldown`이 기본 row와 같은 값인 경우가 많다(예: `3030`/`3031`/`3032` 전부 `Range_X=400`, 쿨타임 12). 초기화하면 진화가 강화를 잃는 손해처럼 보인다.
7. **강화 노드가 같은 (스킬, 필드)에 겹칠 계획이 있는가** — 현재 중복 0건이라 누적 규칙을 정하지 않았다. 겹치게 만들 계획이면 합산(`1+ΣV`)과 연쇄 곱(`Π(1+V)`) 중 무엇인지 정해야 한다. 특히 쿨타임 감소가 겹치면 합산으로는 음수가 나올 수 있다.
8. **"핵심 노드" 강조가 필요한가** — 기획서 본문에는 있는데 데이터 시트에 컬럼이 없다. 필요하면 공통 컬럼에 `bool`을 추가한다(해석 영향 없음).
9. **`Slot` 값을 정수 각도 인덱스로 바꿔줄 수 있는가** — 현재 `R2-M03` 같은 라벨이라 각도를 못 뽑는다(§노드 배치 좌표). 컬럼 추가 없이 값 표기만 링 내 0-기준 정수로 바꾸면 위젯 배치가 확정된다. 라벨을 유지하고 싶으면 각도 컬럼을 따로 받는다.
10. **최단 6노드로 진화에 닿는 것이 의도인가** — ANY 선행 때문에 한 섹터만 좁게 파고들면 캐릭터당 44개 중 6개로 진화 하나에 도달한다(§노드 간 연결 구조). 넓게 파도록 유도하려면 판정을 ALL로 바꾸는 것이 아니라 진화 노드 비용으로 조절해야 한다.

> **해소된 항목.** **타수 강화 범위**는 데이터에 타수 노드가 없어 무효가 됐다(§먼저 확인할 것 3). **노드 구조 이미지 미확인**은 해소됐다 — 데이터테이블이 토폴로지를 확정했고(4섹터 반복 구조, 192엣지, 무결성 검증 통과), 기획이 제시한 배치 그림도 이 위상과 일치한다. 다만 **각도 좌표는 여전히 데이터에 없다**(위 9번). **칩 소모 모델**은 `Chip_Name_Pattern`이 `Chip_{Grade}_*` 와일드카드이므로 **등급만 맞으면 기능 무관**으로 확정됐다.
>
> **링 해금 조건**은 확인 항목이 아니라 **추후 개발 예정**으로 확정했다. §먼저 확인할 것 1 참고.

---

## 금지 / 주의 사항

- 전투 런타임에서 노드 테이블을 직접 조회하지 않는다. 스냅샷만 소비한다.
- 선행 조건을 **ALL로 구현하지 않는다.** ANY다(선행 둘 중 하나만 충족하면 활성 가능).
- `ValidateCoreConnectivity`를 평시 활성화 판정에 쓰지 않는다. 세이브 로드 시 1회다.
- **Ring 역행 검사에서 동일 링 연결을 잡지 않는다.** 정상 데이터가 120건이다.
- **사이클·도달성 검사를 무향으로 돌리지 않는다.** 1차 링이 무향으로는 닫힌 고리다(`S04` 랩어라운드).
- **`Slot`에서 각도를 계산하지 않는다.** 정수가 아니라 종류별로 번호가 다시 시작하는 라벨이다.
- **개별 노드 테이블을 서브시스템 밖으로 노출하지 않는다.** 통합 인덱스만 노출한다 — 5분할이 호출부로 새면 Kind가 늘 때마다 호출부가 늘어난다.
- **`Node_Connections` 시트를 DataTable로 임포트하지 않는다.** 선행 관계의 소유처가 둘이 된다.
- **노드 키에 `int32` + RowName 보정 패턴을 쓰지 않는다.** `N101-S01`은 비숫자를 포함하므로 ID가 조용히 0이 된다.
- **Row 구조체를 상속으로 공통화하지 않는다.** `Data/` 21개 전부 `FTableRowBase` 직접 상속이다.
- **`Before`/`After`/`Status`/`Exclusive_Group`/`Chip_Name_Pattern` 등 도출·검산 컬럼을 C++ Row에 넣지 않는다.**
- 스킬 강화를 **가산**으로 적용하지 않는다. 비율(곱)이다 — 0값 필드에서 수치가 튄다.
- 0값 함정 경고 로그를 빼지 않는다. 비율이어도 강화가 조용히 무효가 되는 것은 그대로다.
- 칩 소비에 `RemoveItemsFromSlotArray`를 쓰지 않는다(칩을 스킵한다).
- 칩과 골드를 비원자적으로 차감하지 않는다.
- `ApplySkillEnhancementByIndex`를 진화 적용 경로로 쓰지 않는다(복제되지 않는다).
- 진화 DataAsset을 `SelectableSkills`에 등록해 로드아웃 후보로 노출하지 않는다.
- 노드 그래프 배경·연결선 레이어를 히트테스트 가능하게 두지 않는다(칩 드롭을 먹는다).
- 수치(스탯 증가폭·비용·칩 등급별 요구량)를 이 문서에 적지 않는다. DataTable이 단일 출처다.
