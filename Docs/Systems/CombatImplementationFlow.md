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
IA_Attack Started
-> ALSPlayerCharacter::OnAttack
-> 스킬 프리뷰 중이면 ConfirmAnyActiveSkillPreview
-> 아니면 SetBasicAttackHeld(true) 후 ULSPlayerCombatComponent::RequestBasicAttack

IA_Attack Completed / Canceled
-> ALSPlayerCharacter::OnAttackReleased
-> ULSPlayerCombatComponent::SetBasicAttackHeld(false)
```

`SetBasicAttackHeld`는 같은 값이면 no-op(dedup)라 홀드 상태 RPC(`ServerSetBasicAttackHeld`)는 press/release 각 1회만 나간다. Canceled도 바인딩해 매핑 컨텍스트 제거 등으로 Completed가 누락돼도 홀드가 눌린 채 남지 않는다. 해제(`OnAttackReleased`)는 입력 블록 상태와 무관하게 항상 전달한다.

스킬 프리뷰 중 좌클릭은 스킬 확정 입력으로 소비한다. 확정이 실패하더라도 같은 입력으로 기본 공격을 실행하지 않으며, 홀드 상태도 올리지 않는다.

달리기 중 기본 공격 입력이 들어오면 걷기로 전환한다(`OnAttack`에서 `OnRunEnd()` 재사용). 공격이 끝나도 자동으로 달리기로 복귀하지 않으며, 달리기 키를 다시 눌러야 한다. 스킬 프리뷰 확정으로 소비된 입력은 달리기를 해제하지 않는다.

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

콤보 진행 (연타/홀드):

```text
섹션 몽타주 종료
-> ULSGA_PlayerBasicAttack::HandleAttackMontageEnded
-> 다음 섹션이 있으면 OpenPostComboInputWindow (입력 윈도우 오픈)
-> [연타] 윈도우 중 공격 입력 -> QueueComboInput -> 다음 틱 ConsumePostComboInput -> 다음 섹션 재생
-> [홀드] 윈도우가 열리자마자 QueueComboInput(true) 자동 버퍼링 -> 연타 최속과 동일 타이밍으로 다음 섹션 재생
```

홀드 규칙:

- 홀드 중에는 마지막 콤보 섹션 후에도 1타(섹션 0)로 래핑해 무한 반복한다. 연타(탭)는 기존대로 마지막 섹션 후 종료.
- 홀드 유래 버퍼는 소비 직전에 홀드를 재확인한다. 해제됐으면 버퍼를 취소하고 입력 윈도우를 복원해 이후 탭 입력을 기존 방식으로 받는다.
- 대시/스킬/스턴/사망으로 Ability가 캔슬되면 홀드 루프도 함께 끝난다. 홀드를 유지해도 자동 재개는 없다 (재입력 필요).

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
-> GatherBasicAttackTargets: 브로드페이즈 SphereOverlap + Row Range shape 정밀 필터
-> 데미지 GameplayEffect 적용
-> ULSPlayerSkillComponent::HandleBasicAttackHit
```

판정 범위는 `FLSComboAttackRow`의 `Range_Shape` / `Range_X` / `Range_Y`가 단일 출처다 (Circle: 반경=X / Cone: 반경=X, 각도=Y / Box: 길이=X, 폭=Y — 스킬·프리뷰와 동일 규약). 원점은 캐릭터 액터 위치이고, 정밀 판정은 스킬(Override)·몬스터와 같은 공용 경로 `ULSHitboxLibrary::IsTargetInsideSkillRange`를 쓴다. 판정은 2D(XY)라 `Range_Z`는 사용하지 않는다. Row가 없거나 `Range_Shape`가 None이거나 `Range_X`가 0 이하면 기존 폴백(전방 `BasicAttackForwardOffset` 지점의 `BasicAttackRadius` 구체, shape 필터 없음)으로 동작한다.

디버그 범위 표시: 콘솔 `LS.Debug.BasicAttackRange 1`을 켜면 로컬 플레이어의 기본 공격 판정 범위를 스킬 프리뷰 컴포넌트(`ULSSkillPreviewComponent`)로 표시한다 (`0`=끔, 구현: `LSPlayerCombatDebugPreview.cpp`). 공격 중이면 재생 중인 콤보 섹션, 아니면 1타 row 기준으로 조준 방향을 따라가고, 폴백 판정이면 폴백 구체를 그대로 보여준다. 실제 스킬 프리뷰가 뜨면 양보하고 끝나면 자동 복귀한다. 표시 재질은 별도 매핑 없이 `ULSSkillPreviewComponent`가 소유한 CircleMaterial/BoxMaterial을 그대로 쓴다(미할당이면 프리뷰 컴포넌트가 Warning 로그를 남기고 표시되지 않음 — [SkillSystemStructure.md](SkillSystemStructure.md) 참고).

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
ULSGA_PlayerSkillBase::ActivateAbility
-> ULSPlayerSkillComponent::ConsumePendingAbilityContext
-> PrepareSkillExecution (SkillData/SkillRow 검증·캐싱)
-> CommitAbility + ApplySkillCooldown
-> SkillMontage 재생 (없으면 즉발)
-> LSAN_SkillEffect 노티파이(LS.Event.Skill.Hit) 수신
-> ExecuteSkillEffect: 판정/데미지/버프/상태이상 GameplayEffect 적용
-> 몽타주 종료 -> EndAbility
```

스킬 효과 발동 지점은 베이스가 소유한다. 각 스킬은 `ExecuteSkillEffect`에 효과만 구현하고, 몽타주 재생·노티파이 대기·종료 타이밍은 `ULSGA_PlayerSkillBase`가 처리한다. 자세한 구조는 [SkillSystemStructure.md](SkillSystemStructure.md)가 단일 출처.

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

## 타격 사운드·VFX 레이어 (GameplayCue + AnimNotify)

타격 사운드는 **주체(때리는 쪽/맞는 쪽)** 와 **레이어**로 나뉜다.

| 레이어 | 주체 | 발동 위치 |
|--------|------|-----------|
| 휘두름 효과음 / 공격 음성(기합) | 때리는 쪽 | 공격 몽타주의 `LSAN_PlaySound` 노티파이 (코드 무관, 아트 영역) |
| 스킬 시전음 | 때리는 쪽 | 스킬 발동 시 코드가 Cue 발동 |
| 피격 임팩트음(재질) + 피격 보이스 | 맞는 쪽 | 데미지 수신 시 코드가 Cue 발동 |
| 기본 공격 명중 VFX | 때리는 쪽 데이터 + 맞는 쪽 ASC | 서버 데미지 적용 성공 분기에서 Cue 발동 |

**공통 GCN**: 세 Cue 모두 사운드를 직접 들지 않고 `GameplayCueParameters.SourceObject`로 받은 `USoundBase`를 대상 위치에서 재생하는 **`ULSGCN_PlaySound` 한 클래스**를 공유한다. 태그별로 BP만 하나씩 둔다(`GameplayCue.Combat.Hit` / `GameplayCue.Voice` / `GameplayCue.Skill.Cast`). 사운드는 BP가 아니라 데이터(피격자/스킬)에서 오므로 종류가 늘어도 GCN/태그가 늘지 않는다. Notify BP는 `/Game/LostSignal` 하위, 스캔 경로는 `DefaultGame.ini`의 `GameplayCueNotifyPaths`.

### 기본 공격 명중 VFX (성공 판정 이후)

기본 공격 명중 VFX는 공격 몽타주 Notify가 직접 출력하지 않는다. `LSAN_PlayerMeleeHit`는 판정 타이밍만 전달하고, 서버의 실제 데미지 적용 성공 분기에서 피격 대상 ASC로 `GameplayCue.Combat.HitVFX`를 실행한다. 따라서 빗나감·무적 등으로 `ApplyDamageEffectToTarget`이 실패하면 VFX도 출력되지 않는다.

```text
LSAN_PlayerMeleeHit
-> ULSPlayerCombatComponent::PerformMeleeHit
-> ExecuteMeleeHit의 대상별 ApplyDamageEffectToTarget 성공
-> BasicAttackHitEffect를 SourceObject에 담아 피격 대상 ASC에서 GameplayCue.Combat.HitVFX 실행
-> ULSGCN_SpawnNiagara 파생 BP가 전 클라이언트에서 Niagara 1회 출력
```

- Niagara 에셋의 단일 출처는 `ULSPlayerCombatComponent::BasicAttackHitEffect`이며 캐릭터 BP 컴포넌트 기본값에서 매핑한다.
- `GameplayCue.Combat.HitVFX`용 BP는 `ULSGCN_SpawnNiagara`를 상속하고 `/Game/LostSignal` 하위에 둔다. BP에는 로직이나 Niagara 에셋을 넣지 않고 태그만 매핑한다.
- 현재 기본 공격 판정은 `SphereOverlapActors`라 `FHitResult`가 없다. VFX 위치는 명중 액터의 콜리전 바운드 중심, 방향은 공격 반대 방향을 사용한다. 정확한 표면·본 위치가 필요해지면 판정 결과 구조에 충돌 정보를 추가해 확장한다.

### 피격 사운드 (맞는 쪽, victim-side)

피격 임팩트음과 피격 보이스를 **피격자 데이터([ULSCharacterHitAudioData](../../Source/LostSignal/Characters/LSCharacterHitAudioData.h))** 가 단일 출처로 들고, 데미지 수신 시 한 곳에서 발동한다. **피격자 종류·재질별 분기는 데이터 에셋 교체만으로** 된다(태그 안 늘림). 같은 재질은 여러 적이 같은 Sound Cue를 참조해 공유.

```text
데미지로 CurrentHealth 감소(서버 권한)
-> ULSCharacterCombatComponent::HandleCurrentHealthChanged (NewValue < OldValue)
-> PlayHitAudio():
   ├─ HitImpactSound(재질 퍽) → ExecuteGameplayCue(GameplayCue.Combat.Hit, {SourceObject=사운드, Location=피격자})
   └─ HitVoices 랜덤 1개(서버 선택) + VoiceMinInterval 스로틀 → ExecuteGameplayCue(GameplayCue.Voice, ...)
-> 서버 ExecuteGameplayCue가 NetMulticast로 전 클라가 피격자 위치에서 재생
```

- victim-side라 어떤 공격원(몬스터/플레이어/스킬)이든 자동 커버되고, 무적/막힘으로 체력이 안 깎이면 자연히 안 난다.
- 서버에서 변주를 골라 파라미터로 넘기므로 전 클라가 같은 클립을 듣고, 권한 경로 발동이라 클라 중복이 없다.
- 데미지 GE에는 Cue를 박지 않는다(과거 GE-박기 방식은 피격자 재질을 몰라 재질 분기 불가 → victim-side로 통합).
- 사망 음성은 `DeathVoices` + `HandleDeathStateChanged` 훅으로 확장(에셋 생기면 치사타에서 분기).

### 스킬 시전음 (때리는 쪽, 스킬별)

사운드 에셋을 스킬 DataAsset(`CastSound`)에 두고 발동 시 파라미터로 전달한다. DataAsset 필드는 [SkillSystemStructure.md](SkillSystemStructure.md)의 `ULSSkillDataAssetBase`가 단일 출처.

```text
스킬 발동(서버 권한, ULSPlayerSkillComponent::TryActivateGameplayAbility 성공)
-> CastSound를 GameplayCueParameters.SourceObject에 담아 ASC->ExecuteGameplayCue(GameplayCue.Skill.Cast, Params)
-> NetMulticast로 전 클라가 캐스터 위치에서 재생 (GCN은 ULSGCN_PlaySound)
```

- **공격 음성(기합)·휘두름 효과음**은 코드가 아니라 공격 몽타주의 `LSAN_PlaySound` 노티파이로 처리한다(변주는 Sound Cue 내부 랜덤, 캐릭터별 차이는 캐릭터 몽타주에 각자 배치).

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

플레이어 액티브 스킬은 `ULSGA_PlayerSkillBase`가 `SkillMontage`를 재생하고, 몽타주의 `LSAN_SkillEffect` 노티파이가 `LS.Event.Skill.Hit` 이벤트를 보내는 시점에 효과(`ExecuteSkillEffect`)를 발동한다. 몽타주가 없는 스킬은 발동 즉시 효과가 나가는 즉발로 fallback한다(애니메이션 미적용 스킬 호환). 노티파이가 누락된 몽타주는 몽타주 종료 시점에 효과를 보장하고, 윈드업 중 캔슬(스턴/사망)되면 효과가 발동하지 않는다.

### 발소리 (싱크 마커 노티파이 기반)

이동 로코모션은 AnimBP 스테이트머신의 8방향 블렌드스페이스(걷기/달리기 포함)다. 로코모션 시퀀스에 발 접지 싱크 마커(L/R)가 들어간 뒤로는 접지 프레임을 애니메이션이 직접 알므로, 발소리는 마커 위치에 삽입된 **[ULSAN_Footstep](../../Source/LostSignal/Animation/LSAN_Footstep.h) 노티파이**가 발동한다. (이전의 거리 누적 방식 `ULSFootstepComponent`는 마커 도입 전 임시 구현이라 삭제됨.)

```text
로코모션 시퀀스의 LS_Footsteps 트랙 노티파이 발화 (싱크 마커 L/R와 같은 시간)
-> ULSAN_Footstep::Notify
-> 소유자 ALSCharacterBase에서 FootstepSound·FootstepVFX 조회(미할당 항목은 생략)
-> 공중·이동 스킬(RootMotionSource) 중이면 스킵
-> 접지한 발 소켓(SocketName) 위치에서 사운드 로컬 재생 + VFX 비부착 스폰
```

- 노티파이 삽입은 수작업이 아니라 `tools/insert_footstep_notifies.py`가 수행: 블렌드 스페이스(`BS_Unarmed`) 샘플 시퀀스들의 싱크 마커를 읽어 같은 시간에 `LSAN_Footstep`을 삽입하고 마커 L/R로 `SocketName`(foot_l/foot_r)을 굽는다. 새 로코모션 시퀀스를 추가하면 마커를 찍고 스크립트를 재실행해야 발소리가 난다(마커 없는 시퀀스는 무음).
- 노티파이별 `bSpawnVFX`로 이펙트만 끌 수 있다(사운드는 유지). 걷기는 먼지가 과해서 스크립트가 시퀀스 이름 키워드(`walk`)로 자동으로 끄고 굽는다.
- 블렌드 스페이스의 Notify Trigger Mode 기본값(Highest Weighted Animation) 덕에 가중치 1등 샘플의 노티파이만 발화 → 27샘플이 블렌딩돼도 발소리 중복 없음. 샘플들이 싱크 마커로 위상 동기화되므로 1등이 바뀌어도 타이밍이 튀지 않는다.
- 사운드·VFX는 노티파이가 아니라 **캐릭터가 소유**: `ALSCharacterBase::FootstepSound`(Sound Cue로 변주)·`FootstepVFX`(먼지 등, 발에 붙지 않고 접지 지점에 남는 비부착 스폰)를 캐릭터 BP에서 매핑. 같은 애니메이션을 공유해도 캐릭터별 연출이 다르고, 에셋 교체 시 시퀀스를 다시 안 건드린다.
- 이동 스킬(대시/바이패스/처형 등 `ApplyRootMotionSource` 이동) 중에는 발소리를 억제한다: `Movement->CurrentRootMotion.HasActiveRootMotionSources()`면 스킵. 스크립트 이동이라 보행 발소리가 부적합하고, 태그/어빌리티를 안 건드려도 RootMotion 이동기 전부(향후 추가분 포함) 자동 커버. 몽타주 애님 루트모션은 그룹 API라 대상 아님. 공중(점프/낙하)도 같은 지점에서 스킵.
- 노티파이는 각 클라의 로컬 애님 재생에서 발화하므로 발소리도 로컬 재생 → MO 복제 불필요.
- 확장: 지면 재질별 사운드/이펙트가 필요하면 `ULSAN_Footstep::Notify`에서 `SocketName` 발밑 라인트레이스 → `PhysicalMaterial` → 선택으로 확장(트리거가 한 곳이라 여기만 수정, 시퀀스 재작업 없음).
- AI 청각용 `Noise_Walk`/`Noise_Run` 태그는 발소리 오디오와 별개 레이어다.

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
