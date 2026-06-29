# 전투 구현 흐름

## 먼저 확인할 것

전투 관련 작업을 시작하기 전에 아래 순서로 확인한다.

1. 이 문서의 `빠른 흐름도`
2. 입력 작업이면 `플레이어 입력 흐름`
3. 기본 공격 작업이면 `기본 공격 흐름`
4. 액티브 스킬 작업이면 `액티브 스킬 흐름`
5. 데미지, 버프, 상태이상, 쿨타임 작업이면 [SkillSystemStructure.md](SkillSystemStructure.md)
6. 몬스터 AI 전투 상태 작업이면 [MonsterAIControlStructure.md](MonsterAIControlStructure.md)

이 문서는 전투 입력부터 서버 판정, 데이터 조회, GAS 적용까지의 큰 흐름만 소유한다. 스킬별 세부 규칙, 쿨타임, 강화, DataAsset 세부 구조는 [SkillSystemStructure.md](SkillSystemStructure.md)를 단일 출처로 둔다.

## 빠른 흐름도

```text
플레이어 입력
-> ALSPlayerCharacter
-> ULSPlayerCombatComponent 또는 ULSPlayerSkillComponent
-> 로컬에서 가능한 UI/프리뷰/예측 처리
-> Server RPC
-> 서버에서 DataTable row 재조회
-> ASC로 GameplayAbility 실행
-> GameplayAbility가 GameplayEffect 적용
-> AttributeSet 값 변경
-> UI/애니메이션/상태 태그가 결과를 반영
```

권한 기준:

```text
클라이언트
-> 입력 수집
-> 프리뷰 표시
-> 빠른 이동 로컬 예측
-> 서버 요청 전송

서버
-> 실제 발동 가능 여부 확인
-> DataTable row 재조회
-> 데미지, 상태이상, 쿨타임 확정
-> Attribute 변경
```

## 데이터 로드와 조회 흐름

현재 프로젝트에는 외부 데이터 서버 요청 흐름이 없다. 런타임 전투 데이터는 `ULSGameDataSubsystem`이 DataTable을 로드하고 캐시한 뒤, 필요한 시점에 조회하는 구조다.

```text
게임 시작
-> GameInstance 생성
-> ULSGameDataSubsystem::Initialize
-> ULSGameDataSubsystem::LoadTables
-> ULSGameDataSettings에서 DataTable 참조 읽기
-> ActiveSkill / PassiveSkill / ComboAttack / StatusEffect DataTable 로드
-> row 이름과 ID 보정
```

스킬 실행 중 데이터 조회:

```text
ULSSkillDataAsset.Skill_ID
-> ULSGameDataSubsystem::FindActiveSkillRowByID
-> FLSCharacterSkillRow 획득
-> FLSSkillActivationContext에 row snapshot 저장
-> GameplayAbility에서 context row 사용
```

기본 공격 콤보 데이터 조회:

```text
ComboCharacterID + ComboIndex + ComboTag
-> ULSGameDataSubsystem::FindComboAttackRowByIndex
-> FLSComboAttackRow 획득
-> 몽타주 섹션, 콤보 시간, 데미지 계수, 패시브 트리거에 사용
```

패시브 데이터 조회:

```text
ULSPassiveSkillDataAsset.PassiveSkill_ID
-> ULSGameDataSubsystem::FindPassiveSkillRowByID
-> FLSCharacterPassiveSkillRow 획득
-> Trigger_Event / Trigger_Target_ID 조건 확인
-> 패시브 GameplayAbility 또는 GameplayEffect 실행
```

멀티플레이 기준으로 클라이언트와 서버는 같은 DataTable을 로컬에 로드한다. 클라이언트 조회는 프리뷰와 UI용이고, 실제 판정은 서버가 다시 조회한 row를 기준으로 한다.

나중에 실제 외부 데이터 서버를 붙이면 전투 입력 순간마다 요청하지 않는다. 로그인, 로비, 매치 시작 시점에 데이터를 받아 `ULSGameDataSubsystem` 또는 별도 캐시 계층에 반영하고, 전투 중에는 캐시된 데이터만 조회하는 구조로 간다.

```text
외부 데이터 서버 사용 시 권장 흐름

로그인 또는 매치 시작
-> 데이터 버전 확인
-> 스킬/스탯/상태이상 데이터 수신
-> 로컬 캐시 갱신
-> 전투 중에는 캐시 조회
```

## 플레이어 입력 흐름

`ALSPlayerCharacter`는 Enhanced Input을 받아 전투 컴포넌트로 위임한다. 캐릭터 클래스는 입력 라우팅과 조준 방향 계산을 담당하고, 실제 전투 로직은 컴포넌트와 GAS로 넘긴다.

```text
Enhanced Input
-> ALSPlayerCharacter::SetupPlayerInputComponent
-> 입력 액션 바인딩
-> OnAttack / OnDash / OnSkill1~4 / OnUltimate
```

공격 입력:

```text
ALSPlayerCharacter::OnAttack
-> 스킬 프리뷰 중이면 ConfirmAnyActiveSkillPreview
-> 아니면 ULSPlayerCombatComponent::RequestBasicAttack
```

스킬 프리뷰 중 좌클릭은 스킬 확정 입력으로 소비한다. 확정이 실패하더라도 같은 입력으로 기본 공격을 실행하지 않는다.

기본 공격 콤보 진행 중에 스킬을 확정하면 기본 공격이 즉시 캔슬되고 스킬이 발동한다. (스킬 시전 중 다른 스킬은 차단) 차단/캔슬 태그 계약은 [SkillSystemStructure.md](SkillSystemStructure.md)의 `기본 공격 캔슬과 스킬 차단 태그`가 단일 출처다.

스킬 입력:

```text
ALSPlayerCharacter::OnSkill1~4 / OnUltimate
-> ALSPlayerCharacter::BeginSkillPreview
-> ULSPlayerSkillComponent::BeginSkillPreview
```

마우스 조준:

```text
마우스 화면 좌표
-> DeprojectMousePositionToWorld
-> 캐릭터 기준 평면과 교차점 계산
-> 조준 위치와 회전값 생성
-> 프리뷰 위치 또는 서버 요청 인자로 사용
```

## 기본 공격 흐름

기본 공격은 `ULSPlayerCombatComponent`가 중심이다.

```text
ALSPlayerCharacter::OnAttack
-> ULSPlayerCombatComponent::RequestBasicAttack
-> 로컬 가능 여부 확인
-> 클라이언트면 ServerRequestBasicAttack
-> 서버에서 RequestBasicAttack 재실행
-> BasicAttackAbilityClass를 ASC로 활성화
```

콤보 row 조회:

```text
현재 콤보 인덱스
-> 필요하면 PendingBasicAttackComboIndexOverride 확인
-> ULSPlayerCombatComponent::ResolveComboAttackRow
-> ULSGameDataSubsystem::FindComboAttackRowByIndex
-> FLSComboAttackRow 사용
```

타격 판정:

```text
Anim Notify 또는 Ability 타이밍
-> ULSPlayerCombatComponent::PerformMeleeHit
-> 전방 공격 위치 계산
-> Sphere overlap/trace
-> 유효 타겟 필터링
-> 데미지 GameplayEffect 적용
-> ULSPlayerSkillComponent::HandleBasicAttackHit
```

패시브 트리거:

```text
기본 공격 명중
-> ULSPlayerSkillComponent::HandleBasicAttackHit
-> PassiveSkills 순회
-> 패시브 row 조건 확인
-> 조건이 맞으면 패시브 Ability/Event 실행
```

## 액티브 스킬 흐름

액티브 스킬은 `ULSPlayerSkillComponent`가 입력, 프리뷰, 쿨타임 확인, 서버 요청, 로컬 예측을 담당한다. 실제 효과는 `GameplayAbility`가 담당한다.

프리뷰 시작:

```text
스킬 입력
-> ULSPlayerSkillComponent::BeginSkillPreview
-> SkillSlots에서 ULSSkillDataAsset 확인
-> 쿨타임 태그 확인
-> Skill_ID로 FLSCharacterSkillRow 조회
-> PreviewSpec 생성
-> DataTable Range 값이 있으면 PreviewSpec에 반영
-> 프리뷰 표시
```

프리뷰 확정:

```text
공격 입력 또는 확정 입력
-> ULSPlayerSkillComponent::ConfirmAnyActiveSkillPreview
-> 마우스 목표 지점 확인
-> Cast_Range 기준으로 목표 지점 Clamp
-> 빠른 이동 스킬이면 로컬 예측 시작
-> ServerRequestActivateSkill
```

서버 발동:

```text
ULSPlayerSkillComponent::ServerRequestActivateSkill
-> ActivateSkillOnServer
-> SkillData 확인
-> 서버에서 FLSCharacterSkillRow 재조회
-> 쿨타임 확인
-> FLSSkillActivationContext 생성
-> Context.SkillRow에 row snapshot 저장
-> ASC로 AbilityClass 활성화
```

Ability 실행:

```text
GameplayAbility::ActivateAbility
-> ULSPlayerSkillComponent::ConsumePendingAbilityContext
-> SkillData와 SkillRow 읽기
-> 몽타주, 이동, 판정, 투사체, 장판 등 실행
-> 데미지/버프/상태이상 GameplayEffect 적용
-> ApplySkillCooldown
-> EndAbility
```

## 빠른 이동 스킬 흐름

대시, 바이패스, 익스큐션처럼 앞으로 빠르게 이동하는 스킬은 렌더링 버벅임을 줄이기 위해 클라이언트 로컬 예측을 사용한다.

```text
클라이언트 ConfirmAnyActiveSkillPreview
-> TryPredictFastMovementSkill
-> ResolvePredictedFastMovementParams
-> 적 충돌 무시 대상 수집
-> RootMotionSource 적용
-> 일정 시간 후 FinishPredictedFastMovementSkill
```

서버는 별도로 Ability에서 같은 이동을 권위 실행한다. 클라이언트 예측은 조작감을 위한 임시 이동이고, 최종 위치와 판정은 서버가 결정한다.

빠른 이동 스킬을 추가할 때 확인할 것:

```text
1. Ability에 서버 이동 로직이 있는가
2. ULSPlayerSkillComponent::ResolvePredictedFastMovementParams에서 예측 거리/시간을 해석하는가
3. 이동 중 적 충돌을 무시해야 하는가
4. 예측 종료 시 RootMotionSource와 충돌 무시가 정리되는가
```

## 데미지와 Attribute 흐름

데미지는 GameplayEffect와 ExecutionCalculation을 통해 적용한다. 체력 같은 수치는 직접 수정하지 않는다.

```text
공격 또는 스킬 명중
-> Damage GameplayEffect Spec 생성
-> SetByCaller로 계수, 고정값, 치명타 가능 여부 등 전달
-> Source ASC와 Target ASC 지정
-> Target ASC에 GameplayEffect 적용
-> ULSDamageExecutionCalculation 실행
-> ULSCombatAttributeSet.CurrentHealth 변경
```

AttributeSet 역할 분리:

```text
ULSCombatAttributeSet     생명력 풀 전담 (MaxHealth / CurrentHealth + 데미지 적용용 임시 Damage)
ULSCharacterAttributeSet  캐릭터 능력치 전담 (공격/방어/치명/관통/쿨감/이동/대시/스태미나)
```

데미지 계산은 `ULSCharacterAttributeSet`의 공격/방어 수치를 읽어 최종값을 `ULSCombatAttributeSet.CurrentHealth`에 반영한다.

ASC와 AttributeSet 관계:

```text
캐릭터 인스턴스
-> ASC 보유
-> AttributeSet 인스턴스 보유
-> ASC 초기화 시 AttributeSet을 등록
-> GameplayEffect가 ASC를 통해 AttributeSet 값을 변경
```

즉 Ability가 Attribute 값을 참조하면 해당 캐릭터 인스턴스에 등록된 AttributeSet 값을 읽는다. 서버에서 적용된 Attribute 변경은 복제를 통해 클라이언트 UI와 표시 상태에 반영된다.

## 피격 연출 흐름 (GameplayCue)

타격 사운드는 주체로 나뉜다. **공격음(휘두름)은 때리는 쪽**이 `LSAN_PlaySound`(공격 몽타주 Notify)로 내고, **피격음은 맞는 쪽**이 GameplayCue로 낸다. 피격음을 GameplayCue로 두면 데미지 GE 한 곳에 Cue를 다는 것만으로 몬스터/플레이어/스킬 등 모든 공격원이 자동 커버되고, 멀티에서도 복제 재생된다.

```text
데미지 GE(Instant)가 대상 ASC에 적용
-> GE에 달린 GameplayCue.Combat.Hit "Executed" 발동
-> GameplayCueManager가 태그로 GameplayCueNotify 매칭 (ULSGCN_Hit 파생 BP)
-> ULSGCN_Hit::OnExecute에서 피격자 위치에 피격음 재생
```

- 데미지 적용 = Cue 발동 시점이라, 무적/슈퍼아머로 GE가 막히면(`bDamageBlocked`) 피격음도 자연히 안 난다.
- Cue 태그는 `GameplayCue.Combat.Hit` 단일로 시작한다. 재질/부위별로 쪼갤 때는 `GameplayCue.Combat.Hit.<재질>` 하위 태그를 추가하고, 그때만 발동을 "GE-박기"에서 "코드가 대상 재질을 읽어 `ExecuteGameplayCue`"로 옮긴다. 단일 태그는 부모 폴백으로 계속 동작한다.
- 사운드 에셋은 `ULSGCN_Hit` 파생 BP에서 매핑한다(경로 하드코딩 금지). Notify BP는 `/Game/LostSignal` 하위에 두며 스캔 경로는 `DefaultGame.ini`의 `GameplayCueNotifyPaths`로 지정한다.

## 쿨타임 흐름

쿨타임은 GameplayTag와 GameplayEffect로 관리한다. 스킬 입력 시 쿨타임 중이면 프리뷰 진입도 막는다.

```text
스킬 입력
-> SkillData의 CooldownTag 확인
-> ASC에 해당 태그가 있으면 차단
-> 남은 시간 로그 출력
```

스킬 발동 성공 후:

```text
GameplayAbility 실행
-> ULSPlayerSkillComponent::ApplySkillCooldown
-> Skill_Cooldown 또는 FallbackCooldown 확인
-> CooldownEffectClass로 GE 생성
-> CooldownTag를 동적으로 부여
-> ASC에 적용
```

## 애니메이션과 Notify 흐름

전투 기능 발동 타이밍이 애니메이션 프레임과 맞아야 하면 Anim Notify 또는 Anim Notify State에서 기능을 호출한다.

```text
Ability 실행
-> 몽타주 재생
-> Anim Notify 타이밍 도달
-> 타격 판정 / 태그 윈도우 / 콤보 윈도우 실행
-> 몽타주 종료
-> Ability End
```

즉발 스킬은 Ability에서 바로 효과를 실행할 수 있다. 나중에 애니메이션이 생겨 타이밍이 중요해지면 효과 발동 지점을 Notify로 옮긴다.

## 몬스터와 플레이어 전투 연결

몬스터 AI는 StateTree가 상태를 결정하고, 공격 실행은 GAS Ability로 넘기는 구조다.

```text
StateTree Combat/Attack State
-> 공격 거리 조건 확인
-> LSSTTask_RequestAbilityByTag
-> 몬스터 ASC에서 공격 Ability 활성화
-> 몽타주 재생
-> Anim Notify에서 몬스터 타격 판정
-> 플레이어 ASC에 Damage GE 적용
```

몬스터의 상태 전이와 감지 구조는 [MonsterAIControlStructure.md](MonsterAIControlStructure.md)를 기준으로 한다.

## 구현 체크리스트

전투 기능을 추가하거나 수정할 때 확인할 것:

```text
1. 입력은 ALSPlayerCharacter에서 컴포넌트로 위임되는가
2. 클라이언트에서 할 일과 서버에서 할 일이 분리되어 있는가
3. 서버가 DataTable row를 다시 조회하는가
4. 수치 변경은 GameplayEffect를 통해 처리하는가
5. Ability 실행 중 필요한 데이터가 FLSSkillActivationContext에 들어가는가
6. 로컬 예측이 필요한 이동인지 확인했는가
7. 쿨타임 태그가 스킬별 정책에 맞게 분리되어 있는가
8. 애니메이션 타이밍이 필요하면 Notify로 분리했는가
9. 패시브 트리거가 기본 공격/스킬 명중 이벤트에서 호출되는가
10. 문서상 단일 출처와 코드가 어긋나지 않는가
```

## 금지/주의 사항

- 클라이언트 판정만으로 데미지, CC, 쿨타임을 확정하지 않는다.
- 전투 중 외부 데이터 서버에 매번 요청하는 구조로 만들지 않는다.
- Attribute 값을 직접 더하거나 빼지 않는다. 반드시 GameplayEffect를 거친다.
- DataTable 수치를 DataAsset이나 Ability에 중복 저장하지 않는다.
- 스킬별 특수 필드를 공통 DataAsset에 계속 추가하지 않는다. 필요하면 전용 파생 DataAsset을 만든다.
- 빠른 이동 스킬의 로컬 예측만 만들고 서버 이동을 빼지 않는다.
- Anim Notify에 복잡한 게임 규칙을 넣지 않는다. Notify는 타이밍 신호만 담당하고 실제 처리는 C++ 컴포넌트나 Ability가 한다.
