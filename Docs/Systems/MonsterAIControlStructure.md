# Monster AI Control Structure

## 목적

이 문서는 몬스터 AI 코드를 수정하거나 확장할 때 따르는 관리 구조와 코드 규칙을 정리한다.

몬스터 상태 변화는 StateTree가 담당한다. C++은 StateTree가 판단할 수 있는 데이터를 만들거나, StateTree Task가 요청한 행동을 실행하는 역할로 제한한다.

## 핵심 원칙

```text
상태 판단 데이터 생성
-> Evaluator가 StateTree 데이터로 복사
-> Condition이 전이 가능 여부만 판단
-> StateTree Transition 발생
-> Task가 행동 실행
-> 실제 수치/상태 변경은 GAS 또는 CombatComponent로 위임
```

코드에서 직접 특정 StateTree 상태로 전환하지 않는다. C++은 `bHasVisualTarget`, `DistanceToTarget`, `bIsDead`, `bIsKnockback` 같은 판단 데이터를 갱신하고, 전이는 StateTree가 처리한다.

## 책임 분리

```text
ALSEnemyCharacter
- 몬스터 Actor 조립 지점
- AIControllerClass, AutoPossessAI, AI 컴포넌트 생성
- 몬스터 DataTable 행을 컴포넌트에 적용
- 서버에서 기본 몬스터 Ability 부여
- 몽타주/DeathMontage 같은 캐릭터별 에셋 참조 소유

ALSAIController
- StateTree 실행 호스트
- UStateTreeAIComponent 보유
- 서버 권한에서만 DefaultStateTree 시작/중지
- StateTree 상태 판단 로직을 직접 들고 있지 않는다

ULSMonsterSenseComponent
- 시야/청각/관심 위치 등 AI 판단 입력값 생성
- 플레이어 탐색, 시야각, 거리, Visibility Trace 처리
- StateTree가 직접 월드 탐색하지 않도록 결과만 제공

ULSMonsterCombatComponent
- StateTree와 GAS 사이의 실행 브리지
- AbilityTag 기반 Ability 요청/취소/활성 여부 조회
- AnimNotify 타이밍의 근접 판정 실행
- StateTree 전이를 직접 제어하지 않는다

FLSSTEvaluator_MonsterSense
- Sense / Combat / GAS 상태를 StateTree InstanceData로 복사
- Transition Condition과 Task가 읽는 단일 데이터 공급자
- Condition이 직접 시스템을 조회하지 않게 만드는 경계

StateTree Condition
- Evaluator 값만 읽고 true/false 반환
- 부작용 없는 순수 판정으로 유지

StateTree Task
- 상태 진입/유지 중 필요한 행동 실행
- Focus 설정, Ability 요청, Ability 취소, InterestLocation 초기화, Death 처리 같은 명령 수행
- 전이 조건 판단을 Task에 넣지 않는다

GameplayAbility / GameplayEffect
- 공격 실행, 데미지, 버프, 상태이상, 쿨타임 처리
- 수치 변경은 GameplayEffect를 통해 처리
```

## 몬스터 DataTable 규칙

`FLSMonsterArchetypeRow`는 기획 CSV의 영문 헤더와 1:1로 맞춘다. 첫 번째 `Name` 컬럼은 DataTable row key로 사용하고, UPROPERTY로 중복 선언하지 않는다.

```text
Monster_Name_KR
Monster_Resource_Path
Monster_Combat_Type
Monster_HP
Monster_ATK
Monster_DEF
Monster_Guard
Sight_Radius
Hearing_Radius
Patrol_Speed
Chase_Speed
Action_Group
```

현재 코드 연결 기준:

```text
Sight_Radius
-> ULSMonsterSenseComponent::BaseSightRadius

Hearing_Radius
-> ULSMonsterSenseComponent::HearingRadius

Chase_Speed
-> StateTree에 노출되는 AlertMoveSpeedMultiplier 계열 값

Monster_HP
-> ALSEnemyCharacter가 서버에서 ULSCombatAttributeSet MaxHealth / CurrentHealth 초기화

Monster_ATK
-> ALSEnemyCharacter가 서버에서 ULSCharacterAttributeSet Attack 초기화

Monster_DEF
-> ALSEnemyCharacter가 서버에서 ULSCharacterAttributeSet Defence 초기화

Monster_Guard
-> Row에는 보관하되, Tenacity/Guard 적용 정책이 정해지기 전까지 직접 적용하지 않는다

Action_Group
-> Row에는 보관하되, AbilityTag 매핑 정책이 정해지기 전까지 직접 적용하지 않는다
```

## Transition 데이터 규칙

Transition Condition은 Evaluator의 InstanceData만 읽는다.

```text
SenseComponent
-> CurrentTarget
-> InterestLocation
-> HomeLocation
-> bHasVisualTarget
-> bHasInterestLocation

GAS Tag
-> LS.State.Dead
-> LS.State.Knockback
-> LS.State.Stunned
-> LS.Combat.Attacking

Evaluator InstanceData
-> CurrentTarget
-> InterestLocation
-> HomeLocation
-> DistanceToTarget
-> LeashDistance
-> DistanceFromHome
-> AlertDuration
-> AlertMoveSpeedMultiplier
-> bHasVisualTarget
-> bHasInterestLocation
-> bIsBeyondLeashDistance
-> bIsDead
-> bIsKnockback
```

새 전이 조건에 필요한 값이 있으면 Condition에서 직접 찾지 말고 Evaluator에 먼저 추가한다.

잘못된 방식:

```text
Condition
-> GetWorld()
-> PlayerController 순회
-> ASC 직접 조회
-> true/false 반환
```

권장 방식:

```text
Sense / GAS / Combat
-> 값 계산
-> Evaluator InstanceData에 복사
-> Condition은 복사된 값만 읽음
```

## 기본 상태 모델

코드 작업 시 기본 상태 모델은 아래 구조를 기준으로 본다.

```text
Idle
- 기본 대기
- 관심 위치나 시야 타겟이 생기면 이동 상태로 전이

Investigate
- InterestLocation으로 이동
- 도착 후 StateTree Wait를 거쳐 LS Clear Interest 실행
- 시야 타겟을 다시 얻으면 Chase로 전이

Chase
- 현재 시야 타겟 추적
- 공격 거리 안이면 Attack으로 전이
- 시야가 끊기면 마지막 InterestLocation으로 Investigate 전이
- Leash를 벗어나면 ReturnHome 전이

ReturnHome
- 전투 중 시야에서 플레이어를 놓치고 Leash를 벗어났을 때 진입
- HomeLocation 또는 순찰 복귀 지점으로 이동
- 이동 속도를 AlertMoveSpeedMultiplier 기준으로 증가
- 시야 반경을 MaxSightRadius로 고정
- 복귀 중 플레이어를 다시 보면 Combat으로 전이

Attack
- GAS Ability 요청
- Ability가 끝나거나 대상이 거리 밖으로 나가면 Chase로 전이

Knockback
- LS.State.Knockback 동안 이동 정지 및 공격 취소
- 태그가 해제되면 일반 판단 흐름으로 복귀

Dead
- LS.State.Dead 기반 터미널 상태
- 죽음 처리 후 AI Brain 정지

ReturnHome / Patrol 복귀
- HomeLocation으로 복귀
- 도착하면 Idle로 전이
```

강제 상태 우선순위는 유지한다.

```text
1. Dead
2. Knockback
3. ReturnHome
4. 일반 전투/탐색/순찰 상태
```

`Dead`와 `Knockback`은 GAS 태그를 기준으로 전이한다. 별도 bool을 새로 만들어 중복 상태를 관리하지 않는다.

## 공격 제어 규칙

StateTree는 공격을 직접 구현하지 않고 AbilityTag로 요청한다.

```text
StateTree Attack 상태
-> LS Request Ability By Tag
-> ULSMonsterCombatComponent::RequestAbilityByTag
-> ASC TryActivateAbilitiesByTag
-> GameplayAbility 실행
-> 몽타주 재생
-> AnimNotify
-> ULSMonsterCombatComponent::PerformMeleeHit
-> GameplayEffect로 데미지 적용
```

공격 로직을 추가할 때 지킬 규칙:

- StateTree는 어떤 공격을 할지 `FGameplayTag`로 요청한다.
- 실제 공격 실행은 `GameplayAbility`에 둔다.
- 실제 타격 타이밍은 몽타주 Notify가 호출한다.
- 데미지는 `ULSCharacterCombatComponent::ApplyDamageEffectToTarget` 같은 GAS 경로를 사용한다.
- Attribute를 직접 수정하지 않는다.
- 몬스터별 공격 차이는 C++ if문이 아니라 AbilityTag, AbilityClass, Montage Map, DataTable 값으로 분리한다.

## 감지 제어 규칙

감지 판단은 `ULSMonsterSenseComponent`에 모은다.

```text
Tick
-> 서버 권한 확인
-> 죽음 상태면 관심 정보 초기화 및 Tick 중지
-> FindBestVisibleTarget
-> CanSeeActor
-> 타겟이 보이면 CurrentTarget / InterestLocation 갱신
-> 타겟이 안 보이면 CurrentTarget만 초기화
-> InterestLocation은 StateTree가 LS Clear Interest를 호출할 때까지 유지

RegisterNoiseEvent
-> 소음 이벤트 반경과 몬스터 청각 반경이 모두 닿으면 InterestLocation 갱신
-> 실제 사운드 재생 여부가 아니라 게임플레이 소음 이벤트 기준

SetCurrentTargetFromDamage
-> 몬스터가 플레이어에게 데미지를 받으면 공격한 플레이어를 CurrentTarget으로 설정
-> InterestLocation도 공격자 위치로 갱신
-> StateTree 전이를 직접 실행하지 않고 Evaluator 값 갱신으로 Combat 판단을 유도
```

감지 로직을 확장할 때 지킬 규칙:

- StateTree Condition에 감지 계산을 넣지 않는다.
- 타겟 탐색은 SenseComponent에서 처리한다.
- StateTree에는 결과값만 제공한다.
- 시야/청각/속도 수치는 DataTable 행에서 받고, Leash/시야각 같은 보조값은 별도 정책이 정해질 때까지 컴포넌트 UPROPERTY 기본값으로 관리한다.
- 기억 시간은 SenseComponent가 관리하지 않는다. Investigate 상태의 Move/Wait/ClearInterest 흐름으로 관리한다.
- 죽은 몬스터는 감지 Tick을 멈추고 관심 정보를 비운다.

## 소음 이벤트 발생 구조

캐릭터 행동 소음은 실제 사운드 재생과 분리된 게임플레이 이벤트다. 소음 발생자는 몬스터를 직접 찾지 않고 `ULSNoiseSubsystem`으로 이벤트를 발행한다.

```text
ULSNoiseEmitterComponent
-> FLSNoiseEvent
-> ULSNoiseSubsystem
-> 등록된 ULSMonsterSenseComponent
-> InterestLocation 갱신
-> StateTree가 bHasInterestLocation으로 Investigate 전이
```

책임 분리:

```text
FLSNoiseProfileRow
- 행동별 소음 태그, 반경, 지속 발생 주기 보관
- 반경은 기획 기준 meter 단위로 작성

ULSNoiseEmitterComponent
- 플레이어 캐릭터에 부착
- 이동 중 Walk/Run 지속 소음 발생
- 상호작용 확정 시 Interact 1회 소음 발생
- DataTable 행을 읽어 FLSNoiseEvent로 변환

ULSNoiseSubsystem
- 월드 단위 소음 브로커
- MonsterSenseComponent 등록 목록 관리
- 소음 이벤트를 등록된 감지 컴포넌트에 전달

ULSMonsterSenseComponent
- BeginPlay/EndPlay에서 NoiseSubsystem에 등록/해제
- 소음 이벤트 반경과 Hearing_Radius를 모두 검사
- 청각 감지는 CurrentTarget을 만들지 않고 InterestLocation만 갱신
- bDrawSenseDebug가 켜지면 몬스터 청각 반경을 파란색 원으로 표시
```

판정 규칙:

```text
몬스터 위치와 소음 발생 위치 거리 <= NoiseEvent.RadiusCm + Monster Hearing_Radius
```

소음으로는 `bHasVisualTarget`을 true로 만들지 않는다. 시야로 실제 타겟을 본 경우에만 `CurrentTarget`을 설정한다.

디버그 표시는 아래 색상을 기준으로 한다.

```text
플레이어 소음 이벤트 반경: Cyan
몬스터 청각 반경: Blue
소음으로 갱신된 InterestLocation: Orange
시야 타겟 InterestLocation: Yellow
```

소음 감지가 예상대로 동작하지 않으면 아래 로그를 켜서 단계별로 확인한다.

```text
ULSNoiseEmitterComponent::bLogNoiseDebug
- 플레이어가 소음 이벤트를 실제 발행했는지 확인
- DataTable / RowName / RadiusMeters 문제 확인

ULSNoiseSubsystem::SetLogNoiseDebug(true)
- 소음 이벤트가 등록된 리스너 수만큼 전달되는지 확인

ULSMonsterSenseComponent::bLogNoiseDebug
- 몬스터가 소음을 받았는지 확인
- HearingRadius / NoiseRadius / Distance 판정으로 거절됐는지 확인
- 수락 시 InterestLocation 갱신 위치 확인
```

## ReturnHome 상태 규칙

`ReturnHome`은 마지막 관심 위치 조사와 다르다. `InvestigateInterest`는 플레이어를 놓친 마지막 위치를 확인하는 상태이고, `ReturnHome`은 추적 거리를 벗어나 전투를 포기하고 복귀하는 경계 상태다.

권장 전이:

```text
Combat -> ReturnHome
조건:
!bHasVisualTarget && bIsBeyondLeashDistance
사용 Condition:
LS Has Visual Target(bInvert=true) && LS Is Beyond Leash Distance

ReturnHome -> Combat
조건:
bHasVisualTarget

ReturnHome -> Patrol
조건:
HomeLocation 또는 순찰 복귀 지점 도착
```

권장 Task:

```text
ReturnHome
-> LS Set Return Home Mode
   - Focus Clear
   - SenseComponent ForceMaxSightRadius = true
   - CharacterMovement MaxWalkSpeed *= AlertMoveSpeedMultiplier
-> MoveTo HomeLocation 또는 순찰 복귀 지점
-> LS Clear Interest
```

`LS Set Return Home Mode`는 State Enter에서 포커스와 현재 VisualTarget을 끊고, State Exit에서 시야 반경 강제와 이동 속도를 복구한다. 따라서 이 Task는 ReturnHome 상태에 머무는 동안 유지되는 Running Task로 둔다. ReturnHome 중 플레이어가 최대 시야 안에서 다시 보이면 `bHasVisualTarget`이 다시 true가 되며 Combat로 재진입한다.

## 상태 태그 규칙

AI 전이에 영향을 주는 상태는 GAS Tag를 기준으로 한다.

```text
LS.State.Dead
- 사망 터미널 전이

LS.State.Knockback
- 넉백 일시 중단 전이

LS.State.Stunned
- Ability 요청 차단 및 Brain 일시 정지

LS.Combat.Attacking
- 공격 중복 실행 차단
```

규칙:

- C++ 전투/AI 흐름에 영향을 주는 태그는 `LSGameplayTags`에 선언한다.
- 태그 문자열을 코드에 직접 하드코딩하지 않는다.
- 상태 확정은 GameplayEffect 또는 CombatComponent의 기존 GAS 경로를 사용한다.
- StateTree는 태그를 직접 붙이지 않고 Evaluator가 복사한 상태값을 읽는다.

## 새 상태 추가 절차

새 몬스터 상태나 전이를 추가할 때는 이 순서를 지킨다.

```text
1. 상태 전이에 필요한 입력값을 정한다.
2. 입력값의 원본을 정한다.
   - 감지/위치: ULSMonsterSenseComponent
   - 공격/Ability: ULSMonsterCombatComponent
   - 죽음/넉백/스턴/공격중: GAS Tag
   - 체력/스탯: Attribute 또는 GAS 경로
3. Evaluator InstanceData에 바인딩용 값을 추가한다.
4. Condition은 Evaluator 값만 읽도록 만든다.
5. 행동 실행이 필요하면 StateTree Task를 추가하거나 기존 Task를 확장한다.
6. 수치 변경이 필요하면 GameplayEffect 경로로 처리한다.
7. 몬스터별 차이는 DataTable, AbilityTag, AbilityClass, Montage Map, StateTree Asset으로 분리한다.
```

예시:

```text
Flee 상태 추가
-> HealthRatio 필요
-> AttributeSet 또는 CombatComponent에서 값 제공
-> Evaluator에 HealthRatio 추가
-> LS Is Health Ratio Below Condition 추가
-> Flee State에서 이동 Task 실행
-> 몬스터별 FleeThreshold는 DataTable 또는 StateTree 파라미터로 관리
```

## 코드 작성 금지 사항

- Condition 안에서 `GetWorld()` 기반 탐색을 하지 않는다.
- Condition 안에서 컴포넌트 탐색이나 ASC 직접 조회를 하지 않는다.
- Task 안에 전이 판단을 누적하지 않는다.
- CombatComponent에서 StateTree 상태를 직접 바꾸지 않는다.
- AI 상태용 bool을 GAS 태그와 중복으로 관리하지 않는다.
- 몬스터 종류별 예외를 C++ if문으로 늘리지 않는다.
- 공격 수치, 감지 수치, 전이 임계값을 코드에 하드코딩하지 않는다.
- 클라이언트에서 AI 판단이나 데미지를 확정하지 않는다.
- BP 이벤트그래프에 게임 로직을 넣지 않는다.

## 코드 변경 체크

AI 코드를 수정한 뒤 다음을 확인한다.

```text
- 새 판단값이 Evaluator를 통해 StateTree로 전달되는가
- Condition이 부작용 없이 값만 판정하는가
- Task는 실행 명령만 담당하는가
- 상태 변화가 StateTree Transition으로만 일어나는가
- 데미지/상태이상/쿨타임이 GAS 경로를 사용하는가
- 서버 권한이 필요한 실행 코드에 HasAuthority() 경계가 있는가
- 몬스터별 차이가 C++ 분기가 아니라 데이터/에셋 참조로 분리되어 있는가
- 기존 Dead / Knockback / Stunned 흐름을 깨지 않는가
```
