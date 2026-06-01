# 스킬 시스템 구조

## 먼저 확인할 것

이 문서는 필요한 부분만 골라 읽는 것을 전제로 한다. 스킬 구조를 잡기 전에 항상 아래 순서로 확인한다.

1. `금지/주의 사항`
2. `빠른 흐름도`
3. 현재 작업에 해당하는 상황별 섹션

상황별 확인 기준:

- 새 액티브 스킬을 만들 때: `핵심 클래스`, `공통 입력 흐름`, `DataTable과 DataAsset 우선순위`, `데미지 구조`, `스킬 추가 절차`, `에디터 설정 체크리스트`
- 새 패시브 스킬을 만들 때: `패시브 스킬`, `GameplayEffect`, `상태 태그`, `스킬 추가 절차`, `에디터 설정 체크리스트`
- 스킬에 프리뷰가 필요할 때: `액티브 스킬`, `DataTable과 DataAsset 우선순위`, `에디터 설정 체크리스트`
- 스킬에 쿨타임을 붙일 때: `쿨타임 구조`, `상태 태그`, `에디터 설정 체크리스트`
- 스킬이 데미지를 줄 때: `데미지 구조`, `GameplayEffect`, `DataTable과 DataAsset 우선순위`
- 스킬이 버프/디버프를 줄 때: `GameplayEffect`, `상태 태그`, `DataTable과 DataAsset 우선순위`
- 스킬이 이동 스킬일 때만: `로컬 예측 구조`를 확인한다.
- 스킬이 강화 버전을 가질 때만: `강화 스킬 구조`를 확인한다.
- 새 GameplayTag가 필요할 때만: `상태 태그`를 확인하고 `LSGameplayTags`에 추가한다.
- UI 표시나 아이콘만 바꿀 때: `ULSSkillDataAsset`, `에디터 설정 체크리스트`의 위젯 항목만 확인한다.

## 목적

이 문서는 LostSignal에서 플레이어 스킬을 구현할 때 참고하는 공통 구조 문서다.

개별 스킬의 특수 규칙, 연출, 전용 판정 방식은 이 문서에 넣지 않는다. 이 문서는 새 스킬을 추가할 때 반복해서 필요한 공통 참조만 정리한다.

현재 방향은 다음과 같다.

- 런타임 스킬 실행은 `GameplayAbility`를 기준으로 한다.
- 수치 변경은 `GameplayEffect`를 통해 처리한다.
- 스킬별 설정은 `ULSSkillDataAsset` 또는 스킬 전용 DataAsset에 둔다.
- 기획 수치는 가능하면 `FLSCharacterSkillRow` DataTable에서 읽는다.
- `ULSSkill` 기반 기존 클래스는 레거시 마이그레이션 대상으로만 본다.

## 전체 구조

```text
ALSPlayerCharacter
├── Enhanced Input
├── ULSPlayerCombatComponent
│   ├── 기본 공격 요청
│   ├── 대시 요청
│   └── 대시 로컬 예측
└── ULSPlayerSkillComponent
    ├── SkillSlots
    ├── PassiveSkills
    ├── 스킬 프리뷰
    ├── 쿨타임 확인
    ├── GameplayAbility 활성화 요청
    └── 빠른 이동 스킬 로컬 예측

ULSSkillDataAsset
├── AbilityClass
├── CooldownTag
├── CooldownEffectClass
├── SkillRow
├── PreviewSpec
├── DamageEffectClass
├── FixedDamage / AttackCoefficient / bCanCrit / BreakPower
├── UI 정보
└── EnhancementVariants

GameplayAbility
└── 실제 스킬 실행 로직

GameplayEffect
└── 데미지 / 쿨타임 / 버프 / 상태이상 적용
```

## 핵심 클래스

## ULSPlayerSkillComponent

플레이어 스킬 슬롯과 입력 흐름의 중심이다.

- `SkillSlots`에 슬롯별 `ULSSkillDataAsset`을 가진다.
- `PassiveSkills`에 패시브 스킬 DataAsset을 가진다.
- 프리뷰 시작, 갱신, 확정, 취소를 담당한다.
- 스킬 쿨타임 태그를 확인해서 프리뷰와 발동을 차단한다.
- 서버에 스킬 발동 요청을 보낸다.
- `FLSSkillActivationContext`를 준비해서 Ability가 읽을 수 있게 한다.
- 빠른 이동 스킬의 클라이언트 예측을 담당한다.
- 전투 이벤트를 받아 패시브 발동 조건을 검사한다.

## ULSSkillDataAsset

스킬 공통 데이터의 기준 클래스다.

```text
SkillClass
- 레거시 마이그레이션용
- 신규 런타임 실행 기준으로 사용하지 않는다

AbilityClass
- 실제 실행할 GameplayAbility

CooldownTag
- 쿨타임 차단에 사용할 태그
- 비어 있으면 일부 기본 AbilityClass 기준으로 fallback 태그를 찾는다

FallbackCooldown
- DataTable 쿨타임이 없을 때 사용할 기본 쿨타임

CooldownEffectClass
- 쿨타임 적용용 GameplayEffect
- 기본값은 ULSGE_SkillCooldown

SkillRow
- FLSCharacterSkillRow DataTable row
- 기획 수치의 1차 기준

PreviewSpec
- 프리뷰 표시 기본값
- DataTable Range_Shape / Range_X / Range_Y / Range_Z가 있으면 일부 값이 덮어써진다

DamageEffectClass
- 데미지 적용용 GameplayEffect
- 기본값은 ULSGE_PlayerBasicDamage

FixedDamage / AttackCoefficient / bCanCrit / BreakPower
- DataTable 값이 없거나 fallback이 필요할 때 사용하는 공통 데미지 데이터

DisplayName / Description / Icon
- UI 표시 정보

EnhancementVariants
- 강화 스킬 DataAsset 목록
```

공통 DataAsset에 모든 스킬 전용 필드를 계속 추가하지 않는다. 특정 스킬에만 필요한 값은 파생 DataAsset을 만든다.

## GameplayAbility

실제 스킬 실행 로직이다.

- 발동 가능 여부를 확인한다.
- 몽타주를 재생하거나 즉발 판정을 수행한다.
- 서버 권한에서 데미지, 상태이상, 넉백을 적용한다.
- 필요하면 쿨타임 GE를 적용한다.
- 필요하면 DataAsset과 DataTable row를 읽어서 수치를 해석한다.

스킬 입력이 들어왔다고 바로 태그가 붙는 구조가 아니다.

```text
입력
-> Ability 발동 요청
-> Ability 발동 성공
-> Ability/GE가 필요한 태그 부여
```

## GameplayEffect

수치 변경과 상태 태그 부여를 담당한다.

- 데미지 적용
- 쿨타임 태그 부여
- 버프 스택
- 상태이상 태그 부여
- Attribute Modifier 적용

체력, 공격력, 공격속도 같은 수치는 직접 바꾸지 않고 GE를 거친다.

## 공통 입력 흐름

## 액티브 스킬

```text
스킬 입력
-> ULSPlayerSkillComponent::BeginSkillPreview
-> 쿨타임 태그 확인
-> PreviewSpec 기준 프리뷰 표시
-> ConfirmActiveSkillPreview
-> 쿨타임 태그 재확인
-> FLSSkillActivationContext 생성
-> 빠른 이동 스킬이면 로컬 예측 시작
-> ServerRequestActivateSkill
-> 서버에서 AbilityClass 활성화
-> Ability가 PendingAbilityContext 소비
-> Ability 실행
-> 쿨타임 GE 적용
```

프리뷰가 없는 즉발 스킬도 같은 DataAsset과 Ability 경로를 사용한다. 차이는 PreviewSpec을 보여주느냐의 문제다.

## 패시브 스킬

패시브는 `ULSPlayerSkillComponent::PassiveSkills`에 DataAsset을 등록한다.

```text
전투 이벤트 발생
-> ULSPlayerSkillComponent가 이벤트 수신
-> PassiveSkills 순회
-> DataAsset 조건 확인
-> GameplayEvent 전송 또는 Ability 직접 활성화
-> 패시브 GameplayAbility 실행
-> 필요한 GameplayEffect 적용
```

패시브도 GAS 기반으로 처리한다. 단, 패시브는 플레이어가 직접 입력하는 Ability가 아니라 전투 이벤트를 받아 발동하는 Ability로 본다.

## 쿨타임 구조

쿨타임은 스킬별 GameplayTag로 관리한다.

기준:

- 쿨타임이 서로 다르고 독립적으로 돌아야 하면 태그를 분리한다.
- 같은 쿨타임 그룹을 공유해야 하면 같은 태그를 사용한다.
- 쿨타임 중이면 프리뷰 진입도 차단한다.
- 쿨타임 중 입력하면 `UE_LOG(LogLS, ...)`로 디버그 로그를 남긴다.

적용 흐름:

```text
Ability 발동 성공
-> ULSPlayerSkillComponent::ApplySkillCooldown
-> SkillData.GetCooldownDuration
-> SkillData.GetCooldownTag
-> ULSGE_SkillCooldown Spec 생성
-> Spec에 쿨타임 태그 동적 부여
-> ASC에 GE 적용
```

쿨타임 시간 우선순위:

```text
FLSCharacterSkillRow.Skill_Cooldown > 0
-> SkillData.FallbackCooldown
```

## DataTable과 DataAsset 우선순위

`FLSCharacterSkillRow`는 기획 수치의 기준이다.

주요 필드:

```text
Skill_ID
Parent_Skill_ID
Skill_Char
Skill_Name
Skill_Info
Skill_Type
Skill_Target
Skill_Time
Range_Shape
Range_X / Range_Y / Range_Z
Skill_HitCount
Skill_HitRate
Skill_Multiplier
Skill_Cooldown
Skill_Guard
Skill_Impact
Skill_Count
Skill_Count_Multiplier
Skill_Effect_Type / Value / Duration
Skill_Effect_Type_2 / Value_2 / Duration_2
```

권장 기준:

- 기획자가 조정할 수치: DataTable에 둔다.
- 에셋 참조, 클래스 참조, VFX/Projectile/Field Actor: DataAsset에 둔다.
- 특정 스킬에만 필요한 필드: 공통 `ULSSkillDataAsset`이 아니라 파생 DataAsset에 둔다.
- DataTable row가 없거나 값이 0인 경우: DataAsset fallback 값을 사용한다.

공통 우선순위 예시:

```text
쿨타임
-> DataTable Skill_Cooldown 우선
-> 없으면 FallbackCooldown

프리뷰 범위
-> DataTable Range_Shape / Range_X / Range_Y / Range_Z 우선
-> 없으면 PreviewSpec

데미지 계수
-> DataTable Skill_Multiplier 우선
-> 없으면 SkillData.AttackCoefficient
```

## 데미지 구조

스킬 데미지는 `DamageEffectClass`와 `ULSDamageExecutionCalculation`을 통해 처리한다.

기획식:

```text
공격환산 = (((캐릭터 공격력 + 아이템 증가 수치) * 스킬 계수) + 고정 피해) * 피증 합산
관통공식 = 적 방어력 * (1 - 방어 관통)
방어환산 = 공격환산 * (100 / (관통공식 + 100)) * (1 - 피감 합산)
최종대미지 = 방어환산 * 치명타피해 + 최소 피해 보정
```

현재 연결 기준:

```text
고정 피해
-> LS.Data.Damage.Fixed

스킬 계수
-> LS.Data.Damage.AttackCoefficient

치명타 가능 여부
-> LS.Data.Damage.CanCrit

공격력/방어력/치명 관련 수치
-> ULSCharacterAttributeSet
```

아직 아이템 증가 수치, 피증, 피감, 방어 관통 등 추가 Attribute가 부족한 항목은 Attribute가 생기면 ExecutionCalculation에 연결한다.

## 상태 태그

C++에서 전투 흐름에 영향을 주는 태그는 `LSGameplayTags`에 선언한다.

주요 태그 그룹:

```text
LS.State.*
LS.Ability.*
LS.Cooldown.*
LS.Combat.*
LS.Buff.*
LS.Event.*
LS.Data.*
```

규칙:

- 상태 판정에 쓰는 태그는 문자열 하드코딩하지 않는다.
- C++ 전투 흐름에 영향을 주는 태그는 `LSGameplayTags`에 추가한다.
- 단순 VFX/SFX만 필요한 태그는 GameplayCue 쪽으로 분리한다.
- 같은 쿨타임을 공유할 스킬만 같은 `LS.Cooldown.*` 태그를 쓴다.

## 로컬 예측 구조

## 대시

대시는 `ULSPlayerCombatComponent`가 로컬 예측을 담당하고, `ULSGA_Dash`가 서버 권위 실행을 담당한다.

```text
입력
-> 로컬 CanRequestDashLocally
-> PredictDashMovement
-> ServerRequestDash
-> ULSGA_Dash
```

대시는 빠른 입력 반응이 중요하므로 클라이언트에서 먼저 RootMotionSource를 적용한다. 서버는 같은 방향으로 권위 실행한다.

## 빠른 이동 스킬

전방으로 빠르게 이동하는 스킬은 `ULSPlayerSkillComponent`의 공통 예측 경로를 사용한다.

```text
TryPredictFastMovementSkill
-> ResolvePredictedFastMovementParams
-> IgnoreEnemiesForPredictedFastMovement
-> RootMotionSource 적용
-> FinishPredictedFastMovementSkill
-> 충돌 무시 해제
```

서버 Ability도 별도로 이동을 실행한다. 클라이언트 예측은 렌더링 버벅임을 줄이기 위한 것이고, 최종 결과는 서버가 결정한다.

새 전방 이동 스킬을 추가할 때:

```text
1. 전용 DataAsset에 이동 거리/시간 fallback을 둔다.
2. Ability에서 서버 이동을 구현한다.
3. ULSPlayerSkillComponent::ResolvePredictedFastMovementParams에 예측 파라미터 해석을 추가한다.
4. 이동 중 적 충돌을 무시해야 하면 기존 IgnoreEnemiesForPredictedFastMovement 경로를 사용한다.
```

## 강화 스킬 구조

강화 스킬은 별도 DataAsset으로 만든 뒤, 원본 스킬 DataAsset의 `EnhancementVariants`에 등록한다.

```text
BaseSkillData
└── EnhancementVariants
    ├── EnhancedSkillData_0
    └── EnhancedSkillData_1
```

강화 적용 흐름:

```text
ULSPlayerSkillComponent::ApplySkillEnhancementByIndex
-> 현재 슬롯 SkillData 확인
-> SkillData.GetEnhancementVariant(Index)
-> 슬롯 SkillData 교체
-> Skill UI 갱신
```

강화 버전이 DataTable에서 다른 row를 읽어야 하면 강화 DataAsset의 `SkillRow`를 별도로 지정한다.

즉, 강화 스킬은 원본 DataAsset 안에서 분기 플래그만 늘리는 방식이 아니라 “다른 DataAsset으로 교체”하는 방식이다.

## 스킬 추가 절차

새 액티브 스킬을 추가할 때 기본 순서:

```text
1. FLSCharacterSkillRow DataTable row를 추가한다.
2. 필요한 경우 ULSSkillDataAsset 파생 클래스를 만든다.
3. GameplayAbility 클래스를 만든다.
4. 필요한 GameplayEffect를 만든다.
5. 필요한 GameplayTag를 LSGameplayTags에 추가한다.
6. DataAsset을 만들고 AbilityClass, SkillRow, CooldownTag, DamageEffectClass를 연결한다.
7. 프리뷰가 필요하면 PreviewSpec 또는 DataTable Range 값을 설정한다.
8. ULSPlayerSkillComponent의 SkillSlots에 DataAsset을 등록한다.
9. UI 아이콘/이름/설명을 DataAsset에 설정한다.
10. 서버 권한 판정, 쿨타임 차단, 상태 차단 로그를 확인한다.
```

새 패시브 스킬을 추가할 때 기본 순서:

```text
1. 패시브 DataAsset을 만든다.
2. 패시브 발동 조건을 DataAsset에 둔다.
3. 패시브 GameplayAbility를 만든다.
4. 필요한 GameplayEffect를 만든다.
5. ULSPlayerSkillComponent::PassiveSkills에 등록한다.
6. 발동 이벤트를 어디서 보낼지 정한다.
7. 이벤트 태그가 전투 흐름에 쓰이면 LSGameplayTags에 추가한다.
```

새 강화 스킬을 추가할 때 기본 순서:

```text
1. 강화 버전 DataAsset을 별도로 만든다.
2. 강화 버전의 SkillRow를 필요한 row로 지정한다.
3. 원본 스킬 DataAsset의 EnhancementVariants에 등록한다.
4. 강화 선택 시스템에서 ApplySkillEnhancementByIndex를 호출한다.
5. 강화 후 UI와 쿨타임 태그가 의도대로 바뀌는지 확인한다.
```

## 에디터 설정 체크리스트

스킬 DataAsset:

```text
- AbilityClass 지정
- SkillRow 지정
- CooldownTag 지정
- CooldownEffectClass 지정
- DamageEffectClass 지정
- DisplayName / Description / Icon 지정
- EnhancementVariants 필요 시 지정
```

전용 DataAsset:

```text
- 공통 DataAsset에 넣기 애매한 스킬별 필드만 둔다.
- 기획자가 조정할 수치면 DataTable row에 둘 수 있는지 먼저 본다.
- 에셋 참조와 클래스 참조는 DataAsset에 둔다.
```

Ability BP 또는 C++ 기본값:

```text
- 필요한 몽타주/GE 클래스가 할당됐는지
- Activation Blocked Tags에 Dead/Stunned 등 차단 태그가 포함됐는지
- Instancing Policy가 내부 상태 보유 방식과 맞는지
```

위젯:

```text
- SkillSlotWidget은 SkillData의 Icon/DisplayName을 표시한다.
- SkillBarWidget은 ULSPlayerSkillComponent의 슬롯 데이터를 읽는다.
- 쿨타임 표시는 ASC의 CooldownTag remaining time 기준으로 연결한다.
```

## 금지/주의 사항

- 신규 스킬을 `ULSSkill` 실행 중심으로 만들지 않는다.
- HP, 공격력, 공격속도 같은 수치를 직접 수정하지 않는다.
- 스킬별 특수 필드를 공통 `ULSSkillDataAsset`에 무한히 추가하지 않는다.
- 쿨타임 태그를 공유할지 분리할지 명확히 정하지 않고 추가하지 않는다.
- 로컬 예측 이동만 만들고 서버 Ability 이동을 빼먹지 않는다.
- 서버 판정 없이 클라이언트에서 데미지나 CC를 확정하지 않는다.
- BP 이벤트그래프에 게임 로직을 넣지 않는다.
- 태그 문자열을 코드에 직접 하드코딩하지 않는다.

## 빠른 흐름도

```text
Skill Input
  -> ULSPlayerSkillComponent
  -> SkillData 확인
  -> CooldownTag 확인
  -> Preview 표시 또는 즉시 확정
  -> FLSSkillActivationContext 생성
  -> Local prediction if fast movement
  -> ServerRequestActivateSkill
  -> ASC activates GameplayAbility
  -> Ability reads SkillData / DataTable
  -> Apply damage / buff / cooldown GameplayEffect

Passive Event
  -> ULSPlayerSkillComponent
  -> PassiveSkills
  -> Passive Ability
  -> GameplayEffect

Enhancement
  -> Base SkillData
  -> EnhancementVariants[Index]
  -> Slot SkillData 교체
  -> 이후 입력부터 강화 SkillData 기준 실행
```
