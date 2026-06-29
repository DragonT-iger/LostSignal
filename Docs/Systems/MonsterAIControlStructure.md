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
Monster_Name_KR        (FText)
Monster_Resource_Path  (FSoftObjectPath)
Monster_Rank           (enum ELSMonsterRank: Normal/Boss)
Monster_Combat_Type    (enum ELSMonsterCombatType: Melee/Ranged)
Monster_HP             (float)
Monster_ATK            (float)
Monster_DEF            (float)
Monster_Guard          (int32)
Monster_ArmorPen_Resist(float)
Monster_Crit_Resist    (float)
Sight_Radius           (float)
Hearing_Radius         (float)
Patrol_Speed           (float)
Chase_Speed            (float)
Action_Group           (TArray<FName>: DT_MonsterAction row 참조)
```

`Monster_AmorPen`(공격용 방어 관통)은 기획 스키마 시트에는 있으나 실제 데이터 시트 헤더/CSV에는 없어 Row에 추가하지 않았다(기획 스키마-데이터 불일치, 기획자 확인 대상).

별도 테이블 `DT_MonsterAction`은 `FLSMonsterActionRow`(`Source/LostSignal/Data/LSMonsterActionRow.h`)를 RowStruct로 쓴다. CSV(`Content/LostSignal/Sandbox/DT/DT_MonsterAction.csv`) 영문 헤더와 1:1. 첫 컬럼 `Name`은 row key. enum `ELSActionTarget`(Self/Area), `ELSHitboxShape`(Circle/Cone/Box)를 포함한다.

현재 코드 연결 기준 (AttributeSet 역할: `ULSCombatAttributeSet`=체력 전담, `ULSCharacterAttributeSet`=공격/방어 등 능력치 전담):

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
-> 몬스터 기본 근접 공격 데미지는 FixedDamage=0, AttackCoefficient=1로 적용해서 Monster_ATK를 공격력 입력으로 사용

Monster_DEF
-> ALSEnemyCharacter가 서버에서 ULSCharacterAttributeSet Defence 초기화

Monster_ArmorPen_Resist
-> 서버에서 ULSCharacterAttributeSet ArmorPenetrationResistance 초기화
-> 어트리뷰트 보유까지만. 실제 데미지 계산식(캐릭터 관통 - 저항, 0 클램프) 반영은 데미지 파이프라인 후속

Monster_Crit_Resist
-> 서버에서 ULSCharacterAttributeSet CritChanceResistance 초기화
-> 어트리뷰트 보유까지만. 실제 치명타 계산 반영은 후속

Monster_Rank
-> Row에는 보관하되(ELSMonsterRank: Normal/Boss), 등급별 분기(스폰/보스 처리) 정책이 정해지기 전까지 직접 적용하지 않는다

Monster_Guard
-> Row에는 보관하되(int32), Tenacity/Guard 적용 정책이 정해지기 전까지 직접 적용하지 않는다

Action_Group
-> Row에는 보관하되(TArray<FName>, DT_MonsterAction row 참조), 어빌리티 소비 정책이 정해지기 전까지 직접 적용하지 않는다
```

`ULSMonsterCombatComponent`는 몬스터 기본 공격 데미지에 컴포넌트 공격력 fallback을 사용하지 않는다. `FLSMonsterArchetypeRow`가 적용되지 않으면 공격 Ability 요청 또는 실제 히트 적용을 차단하고 `UE_LOG(LogLS, Warning, ...)`를 남긴다.

## Transition 데이터 규칙

Transition Condition은 Evaluator의 InstanceData만 읽는다.

```text
SenseComponent
-> CurrentTarget
-> InterestLocation
-> HomeLocation
-> bHasVisualTarget   (현재 타겟을 이번 틱 실제로 봄: FOV+LOS)
-> bHasTarget         (타겟 보유: 시야 상실 후 기억 시간 포함)
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
-> bHasVisualTarget   (현재 가시)
-> bHasTarget         (기억 포함 보유)
-> bHasInterestLocation
-> bIsBeyondLeashDistance   (앵커=최초 인식 위치 기준, P0)
-> bIsAttacking
-> bIsDead
-> bIsKnockback
```

`bHasVisualTarget`(현재 가시)과 `bHasTarget`(기억 포함 보유)은 의미가 다르다. Chase 유지는 `bHasTarget`, 즉시 추격/Attack 판정은 `bHasVisualTarget`을 쓴다. `bIsBeyondLeashDistance`는 Home이 아니라 최초 인식 위치(앵커) 기준이다.

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
Idle / Patrol
- 타겟이 없을 때 LS Patrol(FLSSTTask_Patrol)로 HomeLocation 주변 배회
- 직선 5m 이동 → 정지 대기(둘러보기) → 다른 방향 직선 이동 반복(거리/대기시간 변수)
- PatrolRadius 이내로만 이동(경계 초과 시 Home 방향으로 편향), 네비 레이캐스트로 충돌체 직전 ObstacleStopMargin만큼 앞에서 정지
- 이동 속도는 기본 MaxWalkSpeed × PatrolSpeedMultiplier(상태 이탈 시 복원)
- 관심 위치나 시야 타겟이 생기면 이동 상태로 전이(순찰 태스크는 이탈을 막지 않음)

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
- 공격 Ability가 시작되면 몽타주/Ability가 끝날 때까지 상태 이탈을 막음
- Ability가 끝난 뒤 대상 거리와 시야 조건을 다시 판단

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

몬스터 공격은 평타 없이 **DT_MonsterAction(FLSMonsterActionRow) 데이터 주도**로만 동작한다. StateTree는 어떤 액션을 할지 직접 고르지 않고 "공격하라"만 요청하며, 거리/쿨다운에 따른 액션 선택은 `ULSMonsterCombatComponent`가 한다. 발동·타격·범위표시 타이밍은 montage AnimNotify가 구동한다.

```text
StateTree Attack 상태
-> LS Request Monster Action (FLSSTTask_RequestMonsterAction)
-> ULSMonsterCombatComponent::RequestAction(Target)
   -> SelectActionForDistance: 거리 적합 + 쿨다운 준비 후보 중 쿨다운이 가장 긴(=강한) 액션 우선(동률 시 Action_Group 순서)
   -> 활성 액션 컨텍스트 세팅 + 액션별 쿨다운 시작(컴포넌트 TMap 타이머)
   -> RequestAbilityByTag(Ability_MonsterAction)
-> ULSGA_MonsterAction: 활성 액션 row의 Action_Ani 몽타주 재생
-> (윈드업) AnimNotifyState ULSANS_MonsterActionTelegraph
   -> Begin: BeginActionTelegraph(TotalDuration) / Tick: UpdateActionTelegraphFill / End: EndActionTelegraph
   -> ULSSkillPreviewComponent(스킬 인디케이터 재사용)로 Hitbox 모양/크기 표시
   -> fill은 NotifyState 윈도우(TotalDuration) 동안 0→1로 차오름(가득 참=타격 시점). ULSSkillPreviewComponent::SetAreaFillAmount
-> (도약 프레임, 선택) AnimNotify ULSAN_MonsterActionDash
   -> ULSMonsterCombatComponent::PerformActionDash
   -> Dash_Distance/Duration으로 타겟 방향 평면 전진(FRootMotionSource_ConstantForce, 타겟까지 거리로 클램프)
-> (타격 프레임) AnimNotify ULSAN_MonsterActionHit
   -> ULSMonsterCombatComponent::PerformActionHit
   -> SphereOverlap + ULSHitboxLibrary::IsTargetInsideHitbox(Circle/Cone/Box)
   -> ULSCharacterCombatComponent::ApplyDamageEffectToTarget (Coeff=Action_Multiplier, BreakPower=Action_Impact)
```

공격 상태 이탈 규칙:

```text
Attack -> Combat 또는 Chase
조건:
LS Is Attacking(bInvert=true)

Attack -> Dead / Knockback
조건:
기존 강제 상태 전이 유지
```

`LS Request Monster Action`은 액션 어빌리티가 활성화된 뒤에는 기본적으로 거리 이탈로 취소하지 않는다(공격 모션 캔슬 금지). 공격 거리 이탈은 어빌리티 종료 후 StateTree 전이 조건으로 다시 판단한다.

공격 중 facing 고정: `ULSGA_MonsterAction`이 활성인 동안 `bUseControllerDesiredRotation`을 false로 꺼 **공격 시작 시점의 방향으로 body 회전을 고정**한다(어빌리티 종료 시 복원). 따라서 공격 도중에는 플레이어를 따라 돌지 않고, 공격과 공격 사이에만 다시 타겟을 향한다.

텔레그래프 표시 여부는 `ULSMonsterCombatComponent::ShouldShowActionTelegraph()`가 게이팅한다(현재는 항상 true, 추후 전투 프로토콜 레벨 게이팅 확장점).

공격 로직을 추가할 때 지킬 규칙:

- 어떤 액션을 할지는 StateTree가 아니라 CombatComponent가 거리/쿨다운으로 고른다(StateTree는 "공격" 요청만).
- 실제 공격 실행은 `ULSGA_MonsterAction`(단일 데이터 주도 어빌리티)에 둔다.
- 타격·텔레그래프 타이밍은 montage Notify(`ULSAN_MonsterActionHit`)/NotifyState(`ULSANS_MonsterActionTelegraph`)가 호출한다.
- 데미지는 `ULSCharacterCombatComponent::ApplyDamageEffectToTarget` GAS 경로를 사용한다.
- 히트박스(Circle/Cone/Box) 판정은 공용 `ULSHitboxLibrary`를 쓴다(플레이어 스킬과 공유).
- Attribute를 직접 수정하지 않는다.
- 몬스터별 공격 차이는 C++ if문이 아니라 `DT_MonsterAction` 행(계수·사거리·쿨다운·히트박스·Action_Ani)으로 분리한다.

## 감지 제어 규칙

감지 판단은 `ULSMonsterSenseComponent`에 모은다.

```text
Tick (UpdateSensing = 우선순위 중재 1패스)
-> 서버 권한 확인
-> 죽음 상태면 관심 정보 초기화 및 Tick 중지
-> P0(해제): IsBeyondLeashDistance(앵커=최초 인식 위치 기준)는 데이터로만 노출.
   실제 해제는 StateTree가 ReturnHome 진입 시 ClearInterest로 처리(C++ 자체 해제 안 함)
-> P2(시야): FindBestVisibleTarget(최근접) + CanSeeActor
   - 신규 획득(없다가 생김)이면 SetTarget이 앵커 캡처
   - P2 최근접 전환은 앵커 유지
   - 공격 중(LS.Combat.Attacking)에는 타겟 식별자 전환을 보류(모션 캔슬 금지)
-> 현재 타겟이 보이면 InterestLocation 갱신, 기억 타이머 리셋
-> P3(관측 불가): 시야 상실 시 기억 시간(LostSightMemorySeconds, 기본 5s)동안 타겟 유지
   (InterestLocation 동결), 초과 시 ReleaseTarget. 공격 중에는 타이머 정지
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
- 시야/청각/속도와 몬스터 공격력은 DataTable 행에서 받는다. Leash, 시야각, 공격 판정 범위처럼 아직 row 컬럼이 없는 정책값은 별도 정책이 정해질 때까지 컴포넌트 설정으로 관리한다.
- 타겟 기억 시간(P3, 시야 상실 후 유지)은 SenseComponent가 `LostSightMemorySeconds` 타이머로 관리한다. Investigate의 마지막 위치 조사(InterestLocation Move/Wait/ClearInterest)와는 별개 개념이다.
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
- HUD 사운드 인디케이터 알림에서는 플레이어 캐릭터/플레이어 컨트롤 Pawn이 낸 소음을 제외

HUD 사운드 인디케이터의 실제 표시는 `DT_Protocol`의 생존 프로토콜 `Monster_Sound` 해금 row가 보이는 경우에만 허용한다. 해당 row나 데이터 서브시스템을 찾지 못하면 생존 프로토콜 현재 레벨 5 이상을 fallback 기준으로 사용한다.

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
Combat(Chase) 유지
조건:
bHasTarget   (현재 가시 또는 기억 시간 내)

Combat -> ReturnHome (P0 해제)
조건:
bIsBeyondLeashDistance   (앵커=최초 인식 위치에서 leash 초과)
사용 Condition:
LS Is Beyond Leash Distance
참고: ReturnHome 진입 시 LS Set Return Home Mode -> ClearInterest가 타겟/앵커를 해제한다(C++ 자체 해제 아님).

Combat -> InvestigateInterest
조건:
!bHasTarget && bHasInterestLocation   (기억 시간 경과로 타겟 드롭, 마지막 위치 확인)

ReturnHome -> Combat
조건:
bHasVisualTarget   (복귀 중 Leash 안에서 다시 봄)

ReturnHome -> InvestigateInterest
조건:
!bHasVisualTarget && bHasInterestLocation

ReturnHome -> Patrol
조건:
HomeLocation 또는 순찰 복귀 지점 도착
```

> Attack 진입은 `bHasVisualTarget && IsTargetInRange`(현재 가시)로 판단한다. Chase 유지/InterestLocation 추격은 `bHasTarget`을 쓰므로, 기억 시간 동안 타겟이 안 보여도 마지막 목격 위치(`GetInterestLocation`)로 계속 추격한다.

권장 Task:

```text
ReturnHome
-> LS Set Return Home Mode
   - Focus Clear
   - State Enter 시점의 오래된 InterestLocation 정리
   - ReturnHome 중 Leash 밖의 위치가 InterestLocation으로 저장되는 것 억제
   - SenseComponent ForceMaxSightRadius = true
   - CharacterMovement MaxWalkSpeed *= AlertMoveSpeedMultiplier
-> MoveTo HomeLocation 또는 순찰 복귀 지점
```

`LS Set Return Home Mode`는 State Enter에서 포커스와 기존 추적 잔상을 끊고, State Exit에서 시야 반경 강제와 이동 속도를 복구한다. State Exit에서는 `InterestLocation`을 지우지 않는다. 대신 ReturnHome 중에는 Home 기준 Leash 밖의 시야 타겟, 소음, 피격 위치를 `InterestLocation`으로 다시 만들지 않는다. Leash 안의 새 감지만 Combat 또는 InvestigateInterest 전이에 사용한다.

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

## 기획 대비 현재 범위

몬스터 시스템 기획서에는 있으나 현재 코드에 아직 없는 항목을 기록한다. 코드가 사실이므로, 같은 분석을 반복하지 않도록 미구현/보류 상태만 남긴다. 구현 설계는 각 항목을 실제로 작업할 때 별도로 합의한다.

- **위험도(Threat) 시스템:** 기획에서 삭제됨. 코드 잔재(`ULSMonsterSenseComponent` `ThreatMultiplier` / `SetThreatMultiplier`)는 제거 완료. 현재 시야 반경은 평상시 `BaseSightRadius`, ReturnHome(경계) 시 `bForceMaxSightRadius`로 `MaxSightRadius` 고정.
- **스폰/배치 시스템:** 일반·고정형 보스·배회형 보스·탈출 시 스폰 모두 미구현. 메인 레이드용 스폰 매니저 없음(레벨 수동 배치 + DataTable 초기화만). 별도 작업으로 보류.
- **어그로 우선순위 엔진:** 1차로 SenseComponent 내부 완결분 구현됨 — P2(시야 최근접), P3(`LostSightMemorySeconds` 기억 후 해제), 공격 중 타겟 전환 보류, P0 이탈 판정 데이터(앵커=최초 인식 위치 기준 `IsBeyondLeashDistance`). **미구현(후속):** P0 무적(`LS.State.Invulnerable` 등) 및 복귀 200% 전용 속도, P1 특수 상호작용 발신원 배선(루팅/소모품/탈출 → 어그로 트리거)과 그 LOS/2명 동률 처리. 현재 P1 자리는 `SetCurrentTargetFromDamage`(피격자 타겟팅)가 임시로 메움.
  - **StateTree 에셋 배선(후속, 에디터 작업):** `Content/LostSignal/AI/StateTree/ST_EnemyTest`에서 (1) Chase 유지 전이를 `bHasVisualTarget`이 아니라 `bHasTarget`에 바인딩(기억 시간 P3 추격), (2) Attack 진입을 `LS Has Usable Action`(→`bHasUsableAction`) + `LS Has Visual Target`(→`bHasVisualTarget`, 현재 가시)로, (3) 공격 노드를 삭제된 `LS Request Ability By Tag` → 신규 **`LS Request Monster Action`**으로 교체해야 한다. C++은 값/태스크/Condition만 노출한다. (참고: "현재 가시" 변수명은 `bHasVisualTarget`이며 별도 `bIsTargetVisible` 변수는 없다.)
- **저항 어트리뷰트:** Row 컬럼(`Monster_ArmorPen_Resist`/`Monster_Crit_Resist`)과 어트리뷰트(`ArmorPenetrationResistance`/`CritChanceResistance`) **추가 완료**. 단 저항을 실제 데미지/치명타 계산식(관통 - 저항, 0 클램프)에 반영하는 것은 데미지 파이프라인 후속.
- **회전 속도(Turn_Rate):** 최신 DT_MonsterStat CSV에서 컬럼이 빠져 Row 필드·RotationRate 적용 모두 제거됨. 회전 속도는 생성자 기본값(540)으로 동작. 데이터 주도가 필요하면 CSV에 Turn_Rate 컬럼을 다시 넣고 재적용해야 한다.
- **몬스터 공격(데이터 주도):** 평타(`ULSGA_MonsterMelee`/`PerformMeleeHit`) **제거 완료**. 모든 공격이 `DT_MonsterAction`(FLSMonsterActionRow) 기반으로 `ULSGA_MonsterAction` + montage Notify(`ULSAN_MonsterActionHit`)/NotifyState(`ULSANS_MonsterActionTelegraph`)로 실행된다. 거리/쿨다운 선택은 `ULSMonsterCombatComponent::SelectActionForDistance`(쿨다운은 컴포넌트 TMap 월드타이머). 범위 표시는 `ULSSkillPreviewComponent` 재사용. 히트박스 판정은 공용 `ULSHitboxLibrary`(플레이어 Override와 공유).
  - **Dash 이동(도약 물기):** **구현 완료**. `ULSAN_MonsterActionDash`(도약 프레임 노티파이) → `ULSMonsterCombatComponent::PerformActionDash`가 `Dash_Distance`/`Duration`으로 타겟 방향 평면 전진(`FRootMotionSource_ConstantForce`, Priority 6, 타겟까지 거리로 클램프해 오버슈트 방지). 수직 점프 비주얼은 애니메이션이 담당. 어빌리티 종료/캔슬 시 `EndActionDash`로 루트모션 제거. 몽타주 도약 프레임에 노티파이 배치 필요.
    - **착지 정렬:** 도약 액션은 텔레그래프(`BeginActionTelegraph`)·데미지 판정(`PerformActionHit`)을 모두 **착지 예정 지점**에 맞춘다(공유 헬퍼 `ComputeActionOriginAndDirection`). 타격 프레임이 착지보다 일러도 데미지·범위표시가 착지 위치에 들어간다. 텔레그래프는 윈드업 시작 시점의 예측 착지로 1회 배치(윈드업 중 타겟 추종은 미적용).
    - **박스 프리뷰 전방 보정:** Box 히트박스는 원점(뒷변)에서 전방으로 뻗는데 프리뷰 메시는 중심 정렬이라, `BeginActionTelegraph`가 Box일 때 프리뷰를 전방 `Hitbox_X*0.5`만큼 밀어 실제 판정과 맞춘다(플레이어 `LocationOffset.X=Range_X*0.5`와 동일 규칙). Circle/Cone은 원점 중심이라 보정 없음. `ULSSkillPreviewComponent::UpdateAreaPreview`는 `LocationOffset`을 적용하지 않으므로 호출부(여기)에서 더한다.
  - **미구현(후속):** 실제 공중 포물선(JumpForce)·도약 중 호밍, `Erosion_Value`(침식)·`Action_Guard`(액션 중 포이즈) 적용, `bCanCrit`(현재 false 고정).
  - **에디터/BP 셋업:** `ULSMonsterCombatComponent`에 `MonsterActionTable`(DT_MonsterAction)·텔레그래프 머티리얼(Circle/Box), `ULSSkillPreviewComponent`에 `DefaultPreviewMesh`를 BP에서 할당해야 텔레그래프가 보인다.
- **강인도(Monster_Guard):** Row에는 존재하나(int32) 적용 정책 미정(위 "몬스터 DataTable 규칙" 참고).
- **구체 몬스터 클래스:** `ALSEnemyHyena`(ALSEnemyCharacter 상속, 생성자에서 `MonsterRowName="10001"`만 설정) 추가 완료. 그 외 몬스터는 같은 패턴의 얇은 서브클래스 + BP로 확장.
