# 소모품 시스템 구조

이 문서는 소모품(회복약·수류탄·붕대 등)의 데이터 구조와 효과 적용 경로의 단일 출처다.
수치는 DataTable이, 클래스/필드 구조는 C++ 헤더가 소유한다. 여기엔 의도와 경로만 적는다.

## 데이터 구조 (2-테이블 + 상태이상 테이블)

구조체는 모두 [`LSConsumableRow.h`](../../Source/LostSignal/Data/LSConsumableRow.h)가 소유한다.

- **`DT_Consumable`**(`FLSConsumableRow`) — "무엇을 / 어떻게 쓰나". 사용방식·입력·시전·범위 + 효과 목록.
  Weapon/Armor/Chip Row와 동일하게 아이템 정의(`FLSItemRow`, `Item_Type 4~9`)와 **같은 RowName**으로 조회한다.
  `Effects`는 `FLSConsumableEffectValue{ Effect_ID(FName), Effect_Value(float) }` 배열이다 — 사전 참조 + 아이템별 수치("얼마나")만 담고, 복합 효과(응급키트 = 회복 + 출혈 제거)를 위해 배열이다.
- **`DT_ConsumableEffect`**(`FLSConsumableEffectRow`) — 적용효과 사전, "어떻게 작동하나". RowName이 효과 ID(예: `Heal`).
  여러 소모품이 같은 효과 정의를 재사용하고, 수치는 각 소모품이 전달한다.
- **`DT_StatusEffect`**(`FLSStatusEffectRow`) — 지속형·상태 효과의 규칙(지속시간·스택·stat modifier). 사전의 상태 효과가 `Status_ID`로 참조한다. [SkillSystemStructure.md](SkillSystemStructure.md) 상태이상 절이 단일 출처.
- 아이템 공통 정의(이름/설명/가격/최대 수량/타입)는 [`FLSItemRow`](../../Source/LostSignal/Data/LSItemRow.h)가 소유한다. `FLSItemRow`는 프로그래머 관리 영역이 아니라 건드리지 않고, 같은 RowName으로 조회만 한다.

## 효과는 성질로 분리한다

사전 Row(`FLSConsumableEffectRow`)의 컬럼은 엑셀 적용효과 사전과 1:1로 맞춘다(`Consumable_Effect_Type`/`_Operation`/`_Target`/`_Attribute`/`_Value_Type`/`_Apply_Type`/`_Interval`/`_Duration`/`Consumable_Status_Effect_Name`). 단 `Consumable_Status_Effect_Name`만 엑셀의 FName 대신 **int32**(DT_StatusEffect RowName-as-int)로 둔다. 디스패치는 이 컬럼들을 읽어 별도 해석기 없이 기존 GAS 경로로 라우팅한다.

| Type / Operation | 적용 경로 |
|------|-----------|
| `Attribute` + `Add`/`Subtract` (Once) | 즉발 수치 가감 → SetByCaller **Instant GameplayEffect** |
| `Status` + `Apply` | 상태 부여 → `ULSStatusEffectComponent::ApplyStatusEffectByID` |
| `Status` + `Remove` | 상태 제거 → `RemoveStatusEffectByID` |

- **즉발 수치**는 아이템별로 수치가 달라야 하므로 GE의 값을 고정하지 않고 `SetByCaller`로 주입한다.
  - Health: [`ULSGE_HealthChange`](../../Source/LostSignal/GAS/Effects/LSGE_HealthChange.h) + `LS.Data.Health.Amount`
  - Stamina: `ULSGE_StaminaChange` + `LS.Data.Stamina.Amount`
  - Health/Stamina는 서로 다른 AttributeSet(Combat/Character)에 있어 GE 클래스로 분기한다.
- **지속형·상태**는 `DT_StatusEffect`가 규칙(지속시간·스택·stat modifier)을 소유한다. 상태 구조는 [SkillSystemStructure.md](SkillSystemStructure.md)의 상태이상 절과 `FLSStatusEffectRow`가 단일 출처다.

즉발 수치를 왜 `DT_StatusEffect`로 통합하지 않는가: 현재 `ULSStatusEffectComponent`는 즉발(Instant) 정책·호출자 수치 override·Health/Stamina 매핑·주기 틱을 지원하지 않는다. 공용 컴포넌트를 그만큼 개조하면 멀티·스택 계약에 회귀 위험이 커서, 이미 검증된 SetByCaller GE 경로(`ApplyStaminaChange`)를 즉발 수치에 재사용한다.

## 적용 진입점

서버 권위에서 [`ULSCharacterCombatComponent::ApplyConsumableEffects`](../../Source/LostSignal/Combat/LSCharacterCombatComponent.h)가 `Item_Effects`를 순회한다.
각 원소의 `Effect_ID`로 사전 Row를 조회해 `Consumable_Effect_Type`으로 라우팅하고, **수치는 원소의 `Effect_Value`, 나머지(Operation/Target/Attribute/Duration 등)는 사전 Row**에서 읽는다.
`Consumable_Effect_Target`은 `Self`=사용자, `Enemy`=대상에 적용한다(`Friendly`/`All`은 진영/광역 정책이 정해지면 확장).
`Enemy` 대상 체력 감소(Subtract)는 근접 공격과 동일하게 대상 위에 플로팅 데미지 넘버를 띄운다(`BroadcastDamageNumberToPlayers`). 자기 회복(Self)이나 체력 외 어트리뷰트에는 띄우지 않는다.
`Periodic`(주기 Attribute)·`Percent`(비율 즉발)·즉발 Attack/Defense/MoveSpeed 조합은 현재 미지원으로 경고 후 스킵한다(후속).

## 사용(소비) 흐름

퀵슬롯 키 입력으로 소모품을 사용하는 경로는 [QuickSlotSystem.md](QuickSlotSystem.md)가 소유한다(입력→시전→차감→발동 지연→효과 적용). 시전 시간(`Item_Cast_Time`)·이동 중단(`Item_Can_Move`)·발동 지연(`Item_Trigger_Delay`)을 반영하며, 시전은 클라 구동(HUD 시전 게이지)이다. **수량 1 차감은 모든 소모품 공통으로 시전 완료 시점**에 실행하고(발동 지연 전), **효과 적용은 발동 지연 뒤**에 실행한다. 직접 사용(`Direct`)은 자기 자신(Self)에게, 투척(`Throwable`)은 조준한 착탄 지점 범위 내 적에게 `ApplyConsumableEffectsInArea`로 적용한다(Self 효과는 소유자 1회).

## 조회

DataTable 조회는 [`ULSGameDataSubsystem`](../../Source/LostSignal/Data/LSGameDataSubsystem.h)의 `FindConsumableRow`/`FindConsumableEffectRow`가 담당한다.
테이블 참조는 `ULSGameDataSettings`의 `ConsumableTable`/`ConsumableEffectTable`(프로젝트 설정 > LS Game Data Settings)에서 매핑한다.

## 미구현(후속 과제)

이번 범위는 데이터 구조와 효과 적용 경로까지다. 아래는 아직 없다.

- 투척 발사체 비주얼/궤적(범위 인디케이터·착탄 지점 광역 판정은 [QuickSlotSystem.md](QuickSlotSystem.md)가 소유·구현됨)
- HoT/DoT(주기 회복·독): `DT_StatusEffect` 주기 틱 지원이 선행돼야 한다
- CC 상태(기절 등) 부여: `ULSStatusEffectComponent`의 CC/Tag 그룹 지원이 선행돼야 한다
- `Enemy` 즉발 피해가 데미지 파이프라인(방어/치명)을 경유할지 여부 결정(현재는 어트리뷰트 직접 가감)
