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
- UI 표시나 아이콘만 바꿀 때: `ULSSkillDataAssetBase`, `에디터 설정 체크리스트`의 위젯 항목만 확인한다.

## 목적

이 문서는 LostSignal에서 플레이어 스킬을 구현할 때 참고하는 공통 구조 문서다.

개별 스킬의 특수 규칙, 연출, 전용 판정 방식은 이 문서에 넣지 않는다. 이 문서는 새 스킬을 추가할 때 반복해서 필요한 공통 참조만 정리한다.

현재 방향은 다음과 같다.

- 런타임 스킬 실행은 `GameplayAbility`를 기준으로 한다.
- 수치 변경은 `GameplayEffect`를 통해 처리한다.
- 스킬별 설정은 액티브 `ULSSkillDataAsset`, 패시브 `ULSPassiveSkillDataAsset`, 또는 스킬 전용 파생 DataAsset에 둔다.
- 액티브 기획 수치는 `FLSCharacterSkillRow`, 패시브 기획 수치는 `FLSCharacterPassiveSkillRow` DataTable에서 읽는다.

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

ULSSkillDataAssetBase
├── AbilityClass
├── CooldownTag
├── CooldownEffectClass
└── UI 정보

ULSSkillDataAsset
├── Skill_ID
├── PreviewSpec
├── DamageEffectClass
└── EnhancementVariants

ULSPassiveSkillDataAsset
└── PassiveSkill_ID

GameplayAbility
└── 실제 스킬 실행 로직

GameplayEffect
└── 데미지 / 쿨타임 / 버프 / 상태이상 적용
```

## 핵심 클래스

## ULSPlayerSkillComponent

플레이어 스킬 슬롯과 입력 흐름의 중심이다.

- `SkillSlots`(`Skill1~4` 4칸)에 슬롯별 액티브/궁극기 `ULSSkillDataAsset`을 가진다. → `스킬 로드아웃` 참고.
- `PassiveSkills`에 패시브 `ULSPassiveSkillDataAsset`을 가진다(캐릭터 고정 기본 장착, 선택 대상 아님).
- `SkillPool`(`ULSSkillPoolDataAsset`)에 이 캐릭터가 고를 수 있는 후보 목록을 참조한다. `BeginPlay`에서 세이브 로드아웃을 이 풀로 해석해 슬롯에 적용한다.
- 프리뷰 시작, 갱신, 확정, 취소를 담당한다.
- 스킬 쿨타임 태그를 확인해서 프리뷰와 발동을 차단한다.
- 서버에 스킬 발동 요청을 보낸다.
- `FLSSkillActivationContext`를 준비해서 Ability가 읽을 수 있게 한다.
- 빠른 이동 스킬의 클라이언트 예측을 담당한다.
- 전투 이벤트를 받아 패시브 발동 조건을 검사한다.

## ULSSkillDataAssetBase

액티브/패시브 스킬 DataAsset의 공통 기준 클래스다.

```text
AbilityClass
- 실제 실행할 GameplayAbility

SkillMontage
- 확정 입력 시 재생할 스킬 몽타주
- DataTable Skill_Time(시전시간)이 있으면 몽타주 전체가 그 길이에 맞게 자동 스케일된다(playRate 0.25~3.0 클램프, 초과 시 LogLS 경고). 없으면 원본 길이로 재생.
- 실제 효과는 몽타주의 LSAN_SkillEffect 노티파이 시점에 발동한다
- 미할당이면 발동 즉시 효과가 나가는 즉발로 동작한다(애니메이션 미적용 스킬 호환)

CooldownTag
- 쿨타임 차단에 사용할 태그
- 비어 있으면 일부 기본 AbilityClass 기준으로 fallback 태그를 찾는다

FallbackCooldown
- DataTable 쿨타임이 없을 때 사용할 기본 쿨타임

CooldownEffectClass
- 쿨타임 적용용 GameplayEffect
- 기본값은 ULSGE_SkillCooldown

DisplayName / Description / Icon
- UI 표시 정보

CastSound
- 스킬 발동 순간 재생할 시전음(USoundBase). 스킬별로 다르게 둔다.
- 발동 시 ULSPlayerSkillComponent가 GameplayCueParameters에 실어 GameplayCue.Skill.Cast로 발동 → 전 클라 복제 재생.
- 미할당이면 무음. 프레임에 맞춰야 하는 휘두름/타격음은 여기가 아니라 몽타주의 LSAN_PlaySound로 둔다.
```

## ULSSkillDataAsset

액티브 스킬 전용 DataAsset이다.

```text
Skill_ID
- FLSCharacterSkillRow DataTable row 조회 키
- DataAsset은 row handle을 직접 들지 않고, 사용 시점에 ULSGameDataSubsystem에서 Skill_ID로 조회한다.

PreviewSpec
- 프리뷰 표시 기본값
- DataTable Range_Shape / Range_X / Range_Y / Range_Z가 있으면 프리뷰 범위는 DataTable 값을 우선한다.
- Range_Shape가 Cone이면 원형 프리뷰 머티리얼을 쓰고 Degrees는 Range_Y 값을 그대로 사용한다.
- 프리뷰 머티리얼은 DataAsset이 아니라 `ULSSkillPreviewComponent`의 CircleMaterial/BoxMaterial(캐릭터 BP 매핑)이 소유한다. PreviewSpec에는 머티리얼 필드가 없다(몬스터 텔레그래프 등 전용 재질이 필요한 호출자만 `BeginAreaPreview`의 MaterialOverride 인자로 지정).

DamageEffectClass
- 데미지 적용용 GameplayEffect
- 기본값은 ULSGE_PlayerBasicDamage

공통 데미지 수치
- 공통 `ULSSkillDataAssetBase`와 액티브 `ULSSkillDataAsset`에는 AttackCoefficient / bCanCrit / BreakPower 같은 데미지 수치를 두지 않는다.
- 스킬 경로에서는 FixedDamage를 사용하지 않는다. 스킬 데미지는 DataTable의 Skill_Multiplier 또는 Ability/파생 DataAsset fallback 계수 기반으로 계산한다.

DisplayName / Description / Icon
- UI 표시 정보는 ULSSkillDataAssetBase에서 상속받는다.

EnhancementVariants
- 강화 스킬 DataAsset 목록
```

## ULSPassiveSkillDataAsset

패시브 스킬 전용 DataAsset이다.

```text
PassiveSkill_ID
- FLSCharacterPassiveSkillRow DataTable row 조회 키
- 패시브 row 조회는 액티브 Skill_ID와 분리한다.
```

공통 DataAsset에 모든 스킬 전용 필드를 계속 추가하지 않는다. 액티브/패시브 공통은 `ULSSkillDataAssetBase`, 액티브 전용은 `ULSSkillDataAsset`, 패시브 전용은 `ULSPassiveSkillDataAsset`, 특정 스킬 전용 값은 각 파생 DataAsset에 둔다.

## GameplayAbility

실제 스킬 실행 로직이다.

- 발동 가능 여부를 확인한다.
- 공통 베이스는 `ULSGA_PlayerSkillBase`다. 컨텍스트 소비, 커밋, 쿨타임, 몽타주 재생, 노티파이 대기, 종료 처리를 베이스가 담당하고, 각 스킬은 `PrepareSkillExecution`(검증/캐싱)과 `ExecuteSkillEffect`(실제 효과)만 override한다.
- `SkillMontage`가 있으면 몽타주를 재생하고, 몽타주의 `LSAN_SkillEffect` 노티파이가 `LS.Event.Skill.Hit` GameplayEvent를 보내는 시점에 `ExecuteSkillEffect`가 실행된다. 몽타주가 없으면 발동 즉시 실행되는 즉발이다.
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

## 스킬 로드아웃

플레이어가 들고 나가는 스킬 구성이다. (이 문서가 단일 출처)

- **기본 장착(고정, 슬롯 아님):** 일반 공격(`ULSPlayerCombatComponent`), 패시브 스킬(`ULSPlayerSkillComponent::PassiveSkills`), 회피/대시(`ULSGA_Dash`)는 캐릭터마다 고정이며 선택 대상이 아니다. 현재는 캐릭터 BP 기본값으로 부여한다.
- **선택 슬롯 4칸:** `ELSPlayerSkillSlot::Skill1~4`(궁극기 전용 칸 없음). 각 칸에는 **액티브 또는 궁극기**(`ELSCharacterSkillType::Active`/`Ultimate`)만 넣는다. 궁극기도 4칸 중 아무 곳에나 배치한다.
- **선택 가능 후보(`ULSSkillPoolDataAsset`):** 캐릭터별로 고를 수 있는 `ULSSkillDataAsset` 목록. 로비 선택 UI와 런타임 `Skill_ID → DataAsset` 해석이 이 풀 하나를 공용으로 쓴다. 후보의 액티브/궁극기 판정은 `Skill_ID`로 DataTable(`Skill_Type`)을 조회한다(타입은 DataTable이 단일 출처). `ULSPlayerSkillComponent::SkillPool`(런타임)과 로비 스킬 UI(WBP)가 같은 DA 자산을 가리킨다. 풀은 `CharacterID`를 들고 있어, 세이브의 캐릭터별 로드아웃을 이 키로 조회한다(로비·런타임 공용 캐릭터 식별자).
- **기본 로드아웃 시딩:** 풀의 `DefaultEquippedSkillIDs`(최대 4, 순서=Skill1~4)가 최초 진입 시 채울 기본 스킬이다. 로비 스킬 페이지를 처음 열 때(`RefreshSkillLoadout`) 해당 캐릭터 로드아웃의 `bInitialized`가 false면 `ULSSaveSubsystem::TrySeedDefaultSkillLoadout(CharacterID, ...)`이 이 값으로 4칸을 **1회만** 시딩하고 플래그를 세운다. 이후엔 사용자가 슬롯을 다 비워도 다시 채우지 않는다(새 게임 시 세이브가 새로 만들어져 플래그가 리셋됨). 시딩값·0·중복은 앞 칸부터 채우며 건너뛴다.
- **저장:** 선택 결과는 `ULSSaveGame::SkillLoadoutsByCharacter`(키=CharacterID, 값=`FLSSkillLoadout{ SkillIDs[4], bInitialized }`)에 캐릭터별로 저장된다. 조작 API는 `ULSSaveSubsystem::GetEquippedSkillIDs(CharacterID)`/`SetEquippedSkillSlot(CharacterID, Slot, ID)`/`ClearEquippedSkillSlot(CharacterID, Slot)`이며, 변경 시 `OnSkillLoadoutChanged`를 발행한다. 저장 구조는 [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md)가 소유한다.
- **선택 UI(로비):** 로비 상단 `캐릭터` 탭이 스킬 로드아웃 패널을 연다. 로비 패널 전환 구조는 [LobbyScreenStructure.md](LobbyScreenStructure.md)가 소유한다. `ULSSkillLoadoutWidget`이 풀에서 액티브/궁극기 후보를 나열한다. 슬롯 클릭은 현재 편집 슬롯만 선택하고, 후보 클릭은 선택 슬롯에 해당 스킬을 장착한다. 같은 Skill_ID가 다른 슬롯에 이미 있으면 기존 슬롯을 비워 중복 장착을 막는다(이동 처리). 후보/슬롯 개별 표시는 `ULSSkillLoadoutEntryWidget`·슬롯 아이콘이 담당한다. 현재 선택/변경 중인 슬롯 상세는 WBP의 `SelectedSlotEntry`가 같은 Entry 클래스를 표시 전용으로 재사용해 보여준다.
- **런타임 적용:** `ULSPlayerSkillComponent::BeginPlay → ApplyEquippedSkillLoadout`이 세이브에서 `SkillPool->CharacterID`로 캐릭터 로드아웃을 조회해 `SkillPool->FindSkillByID`로 해석하고 4칸에 `SetSkillData`한다. 저장된 선택이 하나도 없으면(신규/미선택) 캐릭터 BP 기본 `SkillSlots`를 폴백 기본 로드아웃으로 유지한다. 서버 권한 또는 로컬 조종 클라에서만 적용한다(데디 서버 비소유 캐릭터는 건너뜀 — 멀티에서 서버 반영은 추후 복제 과제).

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
-> Ability(ULSGA_PlayerSkillBase)가 PendingAbilityContext 소비
-> PrepareSkillExecution 검증 후 커밋 + 쿨타임 GE 적용
-> SkillMontage 재생 (전 클라 멀티캐스트, Skill_Time 시전시간에 맞게 자동 스케일)
-> [임팩트 프레임] LSAN_SkillEffect 노티파이 -> LS.Event.Skill.Hit 이벤트
-> Ability의 WaitGameplayEvent 수신 -> ExecuteSkillEffect (데미지/CC/버프)
-> 몽타주 종료 -> EndAbility
```

`SkillMontage`가 없으면 커밋 직후 `ExecuteSkillEffect`가 바로 실행되는 즉발로 동작한다. 프리뷰 유무는 PreviewSpec을 보여주느냐의 문제로, 즉발/몽타주 분기와는 별개다.

## 슬롯별 캐스트 모드 (발동 방식)

스킬 슬롯(Skill1~4)마다 발동 방식을 플레이어가 지정한다. 위 입력 흐름의 앞단(누름/뗌 입력 → 발동 커밋)만 분기하며, 서버 발동·쿨타임·GAS 경로는 세 방식이 동일하게 공유한다.

- `ELSSkillCastMode`(`LSSkillTypes.h`) 3종:
  - **PreviewConfirm**: 키를 누르면 프리뷰가 뜨고 **마우스 좌클릭**(`OnAttack`)으로 커서 위치를 확정해 발동. 키를 떼도 발동하지 않는다.
  - **QuickCastWithIndicator**(기본): 키를 **누르는 동안** 프리뷰 표시, **키를 떼는 순간**(`Completed`/`Canceled`) 커서 위치로 확정 발동.
  - **QuickCast**: 키를 **누르는 즉시** 커서 위치로 발동(프리뷰/확정 생략).
- 입력 분기는 `ALSPlayerCharacter`가 담당한다. `OnSkillN`(누름)→`HandleSkillInputPressed`, `OnSkillNReleased`(뗌)→`HandleSkillInputReleased`. 누름 시 QuickCast는 `ActivateSkillInstant`, 나머지는 `BeginSkillPreview`. 뗌 시 QuickCastWithIndicator이고 해당 슬롯 프리뷰 중이면 `ConfirmActiveSkillPreview`.
- 프리뷰 취소는 `CancelActiveSkillPreview()`가 담당하며, 전용 취소 입력(`IA_SkillCancel`→`OnSkillPreviewCancelInput`)·대시(`OnDash`)가 호출한다. 추가로 `OnSkill1`은 **누름 시 프리뷰 중이면 먼저 취소**하고 아니면 스킬1 발동 → 한 버튼(예: 우클릭)으로 발동/취소를 겸한다. ⚠️ 이때 그 버튼 IMC에는 `IA_Skill1` **하나만** 매핑해야 한다. `IA_SkillCancel`을 같은 버튼에 함께 두면 Enhanced Input이 두 액션을 순서 보장 없이 모두 실행해 프리뷰가 켜지자마자 취소된다(전용 취소 키로 쓰려면 다른 키에 매핑).
- 즉발/릴리즈 발동 목표점은 세 방식 모두 `ResolveMouseWorldPoint()`(커서 월드 좌표)를 재사용한다.
- 발동 커밋은 `ULSPlayerSkillComponent::ActivateSkillInstant`(즉발) / `ConfirmAnyActiveSkillPreview`(확정)가 공통 헬퍼 `CommitSkillActivation`으로 모인다(사거리 클램프 → 서버 직접 발동 또는 클라 예측+서버 RPC).
- 모드 저장소는 `ULSSkillCastSettingsSubsystem`(`config=GameUserSettings`, `Session/`)이며 슬롯별 스마트키 사용 여부와 스마트키 공통 프리뷰 옵션을 `GameUserSettings.ini`에 저장한다 → New Game으로도 초기화되지 않는다. 슬롯별 스마트키가 꺼져 있으면 `PreviewConfirm`, 켜져 있고 공통 프리뷰 옵션이 켜져 있으면 `QuickCastWithIndicator`, 공통 프리뷰 옵션이 꺼져 있으면 `QuickCast`로 해석한다. 설정 UI(WBP)는 슬롯별 `IsSlotSmartKeyEnabled`/`SetSlotSmartKeyEnabled`, 공통 `IsSmartKeyPreviewOnReleaseEnabled`/`SetSmartKeyPreviewOnReleaseEnabled`를 연결한다.
- 실제 적용 모드 해석은 `ULSPlayerSkillComponent::GetEffectiveCastMode`로 모인다. **디버그 오버라이드**(`bOverrideCastModeForDebug` + `DebugCastModeOverrides` 맵, 컴포넌트 디테일 패널)가 켜져 있으면 맵 값을 우선하고, 맵에 없는 슬롯은 저장소 값을 따른다. 에디터에서 저장소 UI 없이 즉석 테스트용.

## 기본 공격 캔슬과 스킬 차단 태그

액티브 스킬 Ability는 발동 차단/캔슬을 다음 태그 계약으로 처리한다. (이 문서가 단일 출처)

- 기본 공격(`LSGA_PlayerBasicAttack`)은 진행 중 `LS.Combat.Attacking`을 부여하고, AssetTag로 `LS.Ability.Player.BasicAttack`을 가진다.
- 스킬 Ability는 발동 시 `LS.Combat.Attacking`(공통 "진행 중" 의미)과 `LS.Combat.SkillCasting`(스킬 시전 표식)을 함께 부여한다.
- 스킬의 `ActivationBlockedTags`는 `LS.Combat.SkillCasting`을 막는다. → 스킬 시전 중에는 다른 스킬을 발동할 수 없다.
- 스킬의 `CancelAbilitiesWithTag`는 `LS.Ability.Player.BasicAttack`을 캔슬한다. → 기본 공격 모션 중 스킬을 확정하면 기본 공격이 즉시 취소되고(어느 콤보 단계에서든) 스킬이 발동한다. 취소된 기본 공격은 자신의 `EndAbility(bWasCancelled)` 경로에서 몽타주를 멈추고 전투 상태를 정리한다.
- 공격 계열 Ability(기본 공격·스킬·몬스터 액션)는 `LS.Combat.Attacking`을 **AssetTags에도** 가진다. `ASC->CancelAbilities`가 AssetTags 기준으로 매칭하므로, 스턴 등 외부 시스템이 이 태그 하나로 진행 중인 공격 계열 어빌리티를 일괄 취소하기 위한 분류 태그다. (사망은 터미널 상태라 태그 매칭 없이 `CancelAllAbilities`로 전부 취소한다.)

`LS.Combat.Attacking`은 몬스터 감지/소음, 전투 컴포넌트 등 다른 시스템이 "공격/스킬 진행 중" 의미로 참조하므로 스킬 간 차단/캔슬 판정에는 쓰지 않는다. 스킬 차단은 `LS.Combat.SkillCasting`, 기본공격 캔슬은 `LS.Ability.Player.BasicAttack`으로 분리하고, `LS.Combat.Attacking`(AssetTags)은 스턴 같은 외부 강제 취소 매칭에만 쓴다.

## 스킬 시전 중 입력 차단

스킬 몽타주가 재생되는 동안 플레이어의 전투 입력(이동·대시·스킬·기본공격 등)을 무시한다. (이 문서가 단일 출처)

- 단일 게이트 태그: `LS.State.InputBlocked`. 캐릭터의 `ALSPlayerCharacter::IsInputBlocked()`가 ASC에서 이 태그를 확인하고, `Move`/`OnAttack`/`OnDash`/`BeginSkillPreview`·`ActivateSkillInstant`(Skill1~4 공통, 프리뷰·즉발 두 진입 경로) 상단에서 게이트한다.
- 태그를 켜는 소스는 둘이며 같은 태그를 토글한다.
  - **기본(몽타주 전체)**: `ULSGA_PlayerSkillBase`가 스킬 몽타주를 재생할 때, 그 몽타주에 입력차단 NotifyState가 **없으면** 몽타주 시작~종료(취소 포함) 동안 태그를 부여한다.
  - **구간 지정**: 몽타주에 `ULSANS_BlockInput`(AnimNotifyState)을 배치하면, 베이스는 기본 차단을 걸지 않고 그 **NotifyState 구간에만** 태그를 토글한다. 애니메이터가 프레임 단위로 차단 창을 제어한다.
- **루트모션 무영향**: 게이트는 입력 이동(`AddMovementInput`)만 막는다. 이동 스킬의 `FRootMotionSource`(Bypass 슬라이드·Execution 대시)는 별도 경로라 그대로 이동한다.
- **모달 UI 게이트(별도 경로)**: 모달 UI(인벤토리/룻드랍/로비창고/칩스테이션)가 열려 있으면 `ALSPlayerCharacter::IsModalUIBlockingInput()`(컨트롤러 `IsAnyModalPanelOpen()` 기준)이 `OnAttack`/`OnDash`/`BeginSkillPreview`·`ActivateSkillInstant`를 추가로 게이트한다. 이동(`Move`)은 허용. GAS 태그를 쓰지 않고 클라 로컬 UI 상태를 매번 재계산하는 폴링 판정이다(닫힘 경로가 여러 곳이라 태그 누수 방지). 모달이 열릴 때 진행 중 스킬 프리뷰는 취소된다(`ShowInventoryWidgetInternal`·`OnInteract`).
- **회전 잠금(별도 경로)**: 캐릭터 회전(마우스 조준 `ULSAimComponent::UpdateFacing`·달리기 `FaceMovementDirection`)은 `ALSPlayerCharacter::Tick`에서 `IsFacingRotationLocked()`로 게이트한다. 잠금 범위는 둘로 나뉜다.
  - 스킬 시전: `LS.Combat.SkillCasting` 보유 중 전 구간 잠금.
  - 기본공격: 콤보 스윙 시작(`PlayComboSection`→`ResetBasicAttackHit`)부터 히트 판정 프레임(`LSAN_PlayerMeleeHit`→`PerformMeleeHit`, `IsBasicAttackHitConsumed()`)까지만 잠금. 히트 이후엔 다음 콤보 조준을 위해 회전을 허용하며, 다음 섹션 시작 시 다시 잠긴다.
  - `LS.State.InputBlocked`를 쓰지 않는 이유: 기본공격은 InputBlocked를 부여하지 않는다. 어빌리티가 주도하는 회전(루트모션 등)은 이 경로 밖이라 영향 없다.
- 몽타주가 없는 즉발 스킬은 이 경로를 타지 않아 차단이 걸리지 않는다(막을 애니메이션이 없음).
- 멀티(데디케이티드): 기본(몽타주 전체) 차단은 서버 권위 Ability가 `AddLooseGameplayTag(..., EGameplayTagReplicationState::TagOnly)`로 부여해 소유 클라로 복제된다. NotifyState 경로는 소유 클라에서 몽타주가 재생되며 로컬로 토글되어 별개로 동작한다. 둘 다 데디 호환이며, 기본 차단은 서버→클라 복제 RTT만큼 시작 지연이 있으나 몽타주 멀티캐스트도 같은 RTT를 타 정렬된다. 시작 지연까지 없애려면 확정 시점 소유 클라 예측 차단을 추가한다(선택).

## 패시브 스킬

패시브는 `ULSPlayerSkillComponent::PassiveSkills`에 `ULSPassiveSkillDataAsset` 파생 DataAsset을 등록한다.

```text
전투 이벤트 발생
-> ULSPlayerSkillComponent가 이벤트 수신
-> PassiveSkills 순회
-> PassiveSkill_ID로 FLSCharacterPassiveSkillRow 조회
-> Trigger_Event / Trigger_Target_ID 조건 확인
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
ULSDashSkillDataAsset.CooldownAttribute 유효 (대쉬 전용)
-> FLSCharacterSkillRow.Skill_Cooldown > 0
-> SkillData.FallbackCooldown
```

대쉬 표시 전용 DataAsset(`ULSDashSkillDataAsset`)의 `CooldownAttribute`가 유효하면 스킬 테이블/`FallbackCooldown`을 건너뛰고 소유 ASC의 해당 어트리뷰트 값을 총시간으로 쓴다(대쉬 = `DashCooldown`). 어트리뷰트에서 쿨타임을 읽는 스킬은 현재 대쉬뿐이라 이 필드는 대쉬 파생 클래스에만 둔다.

## DataTable과 DataAsset 우선순위

`FLSCharacterSkillRow`는 액티브 스킬 기획 수치의 기준이다.

주요 필드:

```text
Skill_ID
Parent_Skill_ID
Skill_Char
Skill_Name
Skill_Info
Skill_Input
Skill_Type
Skill_Target
Skill_Time
Range_Shape
Range_X / Range_Y / Range_Z
Cast_Range
Skill_HitCount
Skill_HitRate
Skill_Multiplier
CC_Type / CC_Value
Skill_Cooldown
Skill_Guard
Skill_Impact
Consume_Res_ID
Consume_Res_Value
Res_Multiplier
Move_Distance / Move_Duration
Skill_Effects
Status_ID / Effect_Target / Skill_Effect_Duration
Status_ID_2 / Effect_Target_2 / Skill_Effect_Duration_2
```

`Skill_Time`은 **시전시간(캐스팅 시간)** 전용이다. `ULSGA_PlayerSkillBase`가 기본으로 스킬 몽타주 전체를 이 길이에 맞춰 스케일한다. 과거처럼 다른 시간 용도로 전용하지 않는다 — 투사체 비행시간(ShortCircuit)·대시 시간(Execution)은 각 스킬 DataAsset(`ProjectileFlightDuration` / `FallbackDashDuration`)이 단일 출처다. 넉백/끌어당김 지속시간·감속 커브는 스킬별 값이 아니라 프로젝트 설정 `ULSCombatSettings`(Knockback Duration / Knockback Strength Curve)가 전 스킬·몬스터 액션 공용 단일 출처다(속도만 각 테이블의 `CC_Value`). Bypass 슬라이드는 이동시간이 곧 시전시간이므로 `Skill_Time`을 그대로 쓴다.

UE DataTable CSV import에서는 첫 컬럼이 RowName으로 소비된다. 액티브 스킬 테이블은 `Skill_ID`를 첫 컬럼 RowName으로 사용하되, `FLSCharacterSkillRow`가 import/change 시 RowName 숫자를 `Skill_ID` 프로퍼티에도 보정한다.

`FLSCharacterPassiveSkillRow`는 패시브 스킬 기획 수치의 기준이다. 패시브 테이블은 `Name`을 첫 컬럼 RowName으로 사용하되, import/change 시 RowName 숫자를 `PassiveSkill_ID`에도 보정한다.

`FLSComboAttackRow`는 일반 공격 콤보 기획 수치의 기준이다. Combo Attack 테이블은 `Combo_ID`를 첫 컬럼 RowName으로 사용하되, import/change 시 RowName 숫자를 `Combo_ID`에도 보정한다.

전투 가속 같은 콤보 기반 패시브는 다음 키 관계를 사용한다.

```text
기본공격 히트
-> ULSPlayerCombatComponent가 ComboCharacterID + 현재 Combo_Index + 선택적 Combo_Tag로 FLSComboAttackRow 조회
-> Combo_ID를 ULSPlayerSkillComponent에 전달
-> 패시브 row의 Trigger_Target_ID와 Combo_ID 비교
-> 패시브 row의 Status_ID / Effect_Duration 기준으로 GE 적용
```

바이패스 매크로처럼 특정 콤보 row를 선택해야 하는 스킬은 DataAsset에 `ComboTagOverride`를 설정한다. 예를 들어 Combo Attack 시트에서 3타 매크로 row가 `Combo_Tag=5001`이면, 실행 시 `Combo_Index=3`과 `Combo_Tag=5001` 조합으로 row를 찾고 최종 `Combo_ID`를 패시브 트리거 비교에 사용한다.

기본 공격 입력 서버 요청은 `ULSPlayerCombatComponent`가 담당한다. 캐릭터는 입력을 컴포넌트에 전달하고, 컴포넌트의 `RequestBasicAttack`이 클라이언트에서는 `ServerRequestBasicAttack` RPC를 보내고 서버에서는 실제 기본 공격 Ability 활성화 또는 콤보 입력 큐잉을 수행한다.

Combo Attack row의 시간 값은 기본 공격 Ability에서 사용한다. `Combo_Time`이 0보다 크면 현재 몽타주 섹션 길이를 기준으로 재생 속도를 보정하고, `Combo_Input_Window`가 0보다 크면 섹션 종료 후 다음 콤보 입력 대기 시간으로 사용한다. 값이 없으면 기존 Ability fallback 값을 사용한다.

기본 공격 데미지는 히트 시점의 Combo Attack row를 기준으로 계산한다. `Combo_Multiplier`가 0보다 크면 `캐릭터 공격력 * Combo_Multiplier` 계수로 Damage GE에 전달하고, row가 없거나 값이 없으면 기존 컴포넌트 fallback 계수를 사용한다.

권장 기준:

- 기획자가 조정할 수치: DataTable에 둔다.
- 에셋 참조, 클래스 참조, VFX/Projectile/Field Actor: DataAsset에 둔다.
- 특정 스킬에만 필요한 필드: 공통 `ULSSkillDataAssetBase`가 아니라 액티브/패시브 또는 스킬별 파생 DataAsset에 둔다.
- DataTable row가 없거나 값이 0인 경우: Ability 또는 파생 DataAsset fallback 값을 사용한다.

공통 우선순위 예시:

```text
시전시간(몽타주 길이)
-> DataTable Skill_Time 우선 (몽타주 전체 자동 스케일)
-> 없으면 몽타주 원본 길이로 재생

쿨타임
-> ULSDashSkillDataAsset.CooldownAttribute 유효 시 그 어트리뷰트 값 (대쉬 = DashCooldown)
-> DataTable Skill_Cooldown 우선
-> 없으면 FallbackCooldown

프리뷰 범위
-> DataTable Range_Shape / Range_X / Range_Y / Range_Z 우선
-> 없으면 PreviewSpec

데미지 계수
-> DataTable Skill_Multiplier 우선
-> 없으면 Ability 또는 파생 DataAsset fallback 계수

고정 피해
-> 스킬 경로에서는 사용하지 않음
```

## 전역 데이터 조회와 상태이상 적용 구조

데이터테이블 구조체가 정리되면 스킬, 패시브, 상태이상 row 조회는 전역 데이터 조회 객체가 담당한다.

권장 구조:

- `ULSGameDataSettings`: 액티브 스킬, 패시브 스킬, Combo Attack, 상태이상 등 전역 DataTable 참조를 들고 있는 `UDeveloperSettings`
- `ULSGameDataSubsystem`: DataTable 로드, 캐싱, row 조회, 누락 row 검증만 담당하는 `UGameInstanceSubsystem`
- `ULSStatusEffectComponent`: 상태이상 적용, 제거, 스택, 지속시간, UI/FX 훅을 담당하는 대상 Actor 컴포넌트
- `GameplayAbility`: 스킬 발동, 명중 판정, 적용 대상 결정, 적용 타이밍만 담당
- `ULSSkillDataAssetBase`: AbilityClass, 쿨타임 공통값, UI 정보만 담당한다.
- `ULSSkillDataAsset`: 액티브 Skill_ID, 프리뷰, 액티브 GE/에셋 참조, 강화 목록만 담당한다.
- `ULSPassiveSkillDataAsset`: 패시브 PassiveSkill_ID와 패시브 공통 확장점만 담당한다.

`ULSGameDataSubsystem`은 GameplayEffect를 직접 적용하지 않는다. Subsystem이 GE까지 적용하면 데이터 조회, 게임 규칙, 대상 ASC 처리, 서버 권한 처리가 한 곳에 섞여 테스트와 멀티 전환이 어려워진다.

상태이상 적용 흐름 (구현됨):

```text
GameplayAbility/콤보 서버 권한에서 명중/대상 확정
-> ULSCharacterCombatComponent::ApplyStatusEffectFromRow(StatusID, Effect_Target, Duration, HitTarget)
   (Effect_Target: Self=자신, Target=피격 대상, Ally=미지원)
-> 대상 Actor의 ULSStatusEffectComponent::ApplyStatusEffectByID
-> ULSGameDataSubsystem::FindStatusEffectRowByID로 FLSStatusEffectRow 조회
-> Stat_Modifiers의 Target_Stat -> 어트리뷰트 매핑 (Char_*/Mon_* -> ULSCharacterAttributeSet)
-> Stack_Rule / Max_Stack에 따라 동적 UGameplayEffect 생성 (Flat=Additive, Percent=Multiplicitive)
-> 대상 ASC에 적용, 어트리뷰트 복제로 클라 반영
```

구현 메모:
- 진입점은 `ULSCharacterCombatComponent::ApplyStatusEffectFromRow` (스킬/콤보 공용). 액티브 스킬(Override/Overclock/Execution)과 기본 공격 콤보 명중 시 row의 `Status_ID`/`Status_ID_2`를 적용한다.
- `Stat_Modifiers` 배열을 단일 출처로 쓰고 평면 `Target_Stat`/`_2`는 배열이 빈 경우만 fallback (CSV 중복 컬럼 이중 집계 방지).
- **전투가속 패시브는 이 컴포넌트로 통합하지 않는다.** 전투가속은 전용 `ULSGE_CombatAcceleration`(네이티브 GE 스태킹 AggregateBySource·StackLimit=5 + `UTargetTagsGameplayEffectComponent`로 `LS.Buff.CombatAcceleration` 부여)을 쓰고, Overclock/Execution이 그 부여 태그와 `GetCurrentStackCount`로 스택을 소비한다. 동적 GE는 호출마다 다른 객체라 네이티브 스태킹을 합칠 수 없으므로 이 계약을 재현할 수 없다. 범용 컴포넌트는 그런 특수 계약이 없는 신규 상태이상(Refresh/단일 스택 디버프 등)에 쓴다.
- 미지원: `CC`/`Tag` 그룹(기절·무적 등 — Status_ID→GameplayTag 매핑/데이터 필요), 자원 stat(예: `2002`), 일반 패시브 `Skill_Effects[]` 배열 배선, ShortCircuit 장판/Bypass 경로. 버프 아이콘 UI(`LSCombatBuffListWidget`)는 현재 `LS.Buff.*` 부여 태그를 감시하므로, 범용 컴포넌트가 적용하는 신규 버프를 표시하려면 부여 태그 연동이 별도로 필요하다.

책임 분리:

- `ULSGameDataSubsystem`: `FindActiveSkillRow`, `FindPassiveSkillRow`, `FindComboAttackRow`, `FindStatusEffectRow` 같은 순수 조회 API만 제공한다.
- `ULSPlayerSkillComponent`: 클라이언트 입력을 `ServerRequestActivateSkill`로 서버에 보내고, 서버의 `ActivateSkillOnServer`에서 `ULSGameDataSubsystem`에 `Skill_ID`로 row를 직접 요청해 스킬 row 존재 여부, 사거리, 쿨타임을 검증한다.
- `GameplayAbility`: `FLSSkillActivationContext`에 포함된 서버 조회 row snapshot을 사용한다. 지연 실행 Actor처럼 context 밖에서 동작하는 객체는 실행 시점에 `ULSGameDataSubsystem`을 직접 조회한다.
- `ULSStatusEffectComponent`: 상태이상 적용 가능 여부, 중복 스택 처리, 지속시간 override, 제거, UI/FX 이벤트를 관리한다.
- 클라이언트는 프리뷰와 UI 표시용으로 DataTable을 읽을 수 있지만, 데미지와 상태이상 적용 확정은 서버 권한에서만 수행한다.
- 캐릭터마다 상태이상 DataTable을 직접 들고 있지 않는다. 모든 캐릭터가 같은 전역 테이블을 조회하되, 실제 적용 상태는 각 대상의 `ULSStatusEffectComponent`가 보유한다.

스킬 발동 시 현재 조회 흐름:

```text
클라이언트 ConfirmAnyActiveSkillPreview
-> ServerRequestActivateSkill(Slot, TargetLocation, AimYaw)
-> 서버 ActivateSkillOnServer
-> SkillData.Skill_ID 확인
-> ULSGameDataSubsystem.FindActiveSkillRowByID(Skill_ID)
-> row 없으면 서버에서 발동 거부
-> 사거리 Clamp / 쿨타임 검증
-> FLSSkillActivationContext에 row snapshot 저장
-> GameplayAbility 활성화
```

초기 구현 순서:

1. 액티브 스킬 row 구조는 `FLSCharacterSkillRow`를 액티브 전용으로 사용한다.
2. 패시브 스킬 row는 `FLSCharacterPassiveSkillRow`로 분리한다.
3. 상태이상 row는 `FLSStatusEffectRow`로 분리한다.
4. `ULSGameDataSettings`와 `ULSGameDataSubsystem`으로 액티브, 패시브, 상태이상 테이블 조회를 연결한다.
5. `ULSStatusEffectComponent`를 추가하고 `Self` / `Target` 상태이상 적용만 우선 구현한다. (완료 — Buff/Debuff stat modifier 적용. `Ally`·`CC`/`Tag`·자원 stat은 미지원.)
6. 이후 Resource 테이블 조회를 같은 Subsystem 구조로 확장한다.

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

### 스킬 바 표시 (표시 전용 슬롯)

대시는 발동/무적/예측/쿨타임 부여 경로를 스킬 시스템과 독립적으로 유지하면서, 스킬 바에는 **표시 전용 슬롯**으로 노출된다. `ELSPlayerSkillSlot::Dash`는 Skill1~4를 소비하지 않는 전용 슬롯이며, `ULSSkillBarWidget::DashSlot`(`BindWidget`)이 다른 슬롯과 동일하게 `ULSSkillSlotWidget`으로 아이콘/단축키/쿨타임만 그린다.

- 슬롯에 배정하는 DataAsset은 대쉬 전용 `ULSDashSkillDataAsset`(`ULSSkillDataAsset` 파생, 캐릭터별 `DA_Dash_*`)이며 **표시 전용**이다: `AbilityClass`/`CooldownEffectClass`를 비워 발동·쿨타임 부여에 관여하지 않는다. `Skill_ID`로 캐릭터별 대시 스킬 행을 참조해 이름/설명을 얻고, `CooldownTag`는 `LS.Cooldown.Dash`로 둔다. 쿨타임 총시간 출처는 스킬 테이블이 아니라 캐릭터 어트리뷰트다 → `CooldownAttribute`에 `DashCooldown`을 지정한다.
- 대쉬 쿨타임의 단일 출처는 `DashCooldown` 어트리뷰트(초 단위)다. 서버 GE(`ULSGE_DashCooldown`)는 이 어트리뷰트를 지속시간(=쿨타임)으로 캡처하고, 로컬 예측(`ULSPlayerCombatComponent::GetDashCooldown`)도 같은 어트리뷰트를 읽는다. 스킬 바 표시도 여기에 맞춘다.
- 쿨타임 남은시간은 `GetSkillCooldownRemaining`이 `LS.Cooldown.Dash` 태그로 실제 활성 GE(`ULSGE_DashCooldown`)를 조회해 그대로 표시한다. 총시간은 `ResolveSkillCooldownDuration`이 `ULSDashSkillDataAsset::CooldownAttribute`(대쉬는 `DashCooldown`)가 지정돼 있으면 그 어트리뷰트 값을, 없으면 스킬 행 `Skill_Cooldown` → `FallbackCooldown` 순으로 읽어 진행바 분모가 실제 쿨타임과 일치한다.
- 노출(바 표시·쿨타임 숫자·게이지)은 다른 스킬 슬롯과 동일하게 전투 프로토콜 잠금(`Skill_Slot`/`Skill_Cooldown`/`Skill_Cooldown_Gauge`)을 따른다. → [CombatProtocolUI.md](CombatProtocolUI.md)

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

이동 스킬은 즉발/연출형(Override 등)과 몽타주 처리 규칙이 다르다. 다음을 지킨다.

- **종료 권위는 몽타주 끝이 아니라 서버 타이머(이동 Duration)** 다. `ULSGA_PlayerSkillBase::ShouldMontageDriveEnd`를 `false`로 override하면 베이스는 몽타주를 재생만 하고 종료 델리게이트를 바인딩하지 않는다. 데디케이티드 서버에서 서버 메시 애니가 tick되지 않아도 능력이 결정적으로 종료된다.
- **몽타주는 비주얼 전용이고 루트모션 트랙을 넣지 않는다.** 이동은 `FRootMotionSource`(클라 예측 + 서버 권위)만 담당한다. 몽타주가 루트모션을 가지면 소유 클라에서 예측 루트모션과 이중 적용되어 튄다.
- **몽타주 길이는 이동 Duration에 자동으로 맞춘다.** `GetSkillMontagePlayRate`에서 `ComputeMontagePlayRateForDuration(몽타주, None, 이동Duration)`를 반환하면 베이스가 그 playRate로 재생한다. 이동 Duration의 출처는 Bypass는 DataTable `Skill_Time`(슬라이드가 곧 시전시간), Execution 대시는 DataAsset `FallbackDashDuration`이다. (`ResolveComboPlayRate`와 같은 원리, [CombatImplementationFlow.md](../Systems/CombatImplementationFlow.md))

새 전방 이동 스킬을 추가할 때:

```text
1. 전용 DataAsset에 이동 거리/시간 fallback을 둔다.
2. ULSGA_PlayerSkillBase 상속, PrepareSkillExecution(검증·캐싱)/OnSkillStarted(루트모션·타이머)로 서버 이동을 구현한다.
3. ShouldMontageDriveEnd=false, GetSkillMontagePlayRate에서 이동 Duration 기준 자동 스케일.
4. ULSPlayerSkillComponent::ResolvePredictedFastMovementParams에 예측 파라미터 해석을 추가한다.
5. 이동 중 적 충돌을 무시해야 하면 기존 IgnoreEnemiesForPredictedFastMovement 경로를 사용한다.
6. 몽타주는 루트모션 없는 비주얼로 author, DataAsset SkillMontage에 할당(미할당이면 타이머로만 동작).
```

다구간(섹션 분할) 이동 스킬 — 이동 구간과 타격 구간이 한 클립에 있는 스킬(Execution)은 시간 성질이 달라 단일 playRate 스케일이 불가능하다. 다음 패턴을 쓴다(`ULSGA_Execution`이 구현 예).

- 아트가 몽타주에서 전환 프레임에 섹션 마커를 찍어 이동/타격 섹션으로 분할한다(클립 재분할 불필요). 섹션 이름은 Ability의 `DashSectionName`/`SlashSectionName` 기본값과 맞춘다. 이동 섹션이 몽타주의 첫 섹션이어야 한다.
- **재생은 하나의 연속 재생으로 흘린다(끊김 방지)**. 베이스가 몽타주를 처음부터(`NAME_None`) 재생하고, `OnSkillMontagePlaying` override에서 `MulticastSetLSMontageNextSection(이동섹션, 타격섹션)`으로 이동→타격 링크를 명시적으로 세팅한다. 몽타주는 재생을 끊지 않고 자연스럽게 타격 구간으로 흐른다.
- 이동 섹션만 자동 스케일: `GetSkillMontagePlayRate`에서 `ComputeMontagePlayRateForDuration(몽타주, 이동섹션, 이동Duration)`. 이 playRate로 재생을 시작한다.
- 이동 타이머(서버 권위) 만료 시 `MulticastSetLSMontagePlayRate(몽타주, 1.0)`로 **재생을 재시작하지 않고 playRate만 복구**한다(Montage_Play 재호출은 블렌드-인 팝을 유발하므로 쓰지 않는다). 이어서 타격 섹션 길이 타이머가 종료를 주관한다.
- 타격은 타격 섹션의 `LSAN_SkillEffect` 노티파이 시점. 종료 직전 `TriggerSkillEffectOnce()`로 노티파이 누락/몽타주 미할당 시에도 타격 1회를 보장한다(중복은 베이스 가드가 차단).
- 몽타주/섹션 미존재 시 이동 타이머 만료 즉시 타격 후 종료(섹션 미분할 에셋·미할당 호환).

## 강화 스킬 구조

이 섹션은 **런타임 교체 계약**만 담당한다. 어떤 강화를 언제 어떤 조건으로 획득·적용하는지(노드 그래프·해금·비용·저장·UI)는 [CharacterNodeSystem.md](CharacterNodeSystem.md)가 소유한다.

> **용어 주의:** 여기서 말하는 "강화 스킬"은 캐릭터 강화 노드 기획의 **스킬 진화**(스킬 작동 구조 변경, 동일 스킬 1개만 활성)에 해당한다. 기획의 **스킬 강화**(계수·범위·쿨타임·타수를 수치로 상향)는 DataAsset 교체가 아니라 DataTable row 값에 델타를 적용하는 **별개 경로**이며, 그 구조도 [CharacterNodeSystem.md](CharacterNodeSystem.md)가 소유한다.

강화 스킬은 별도 DataAsset으로 만든 뒤, 원본 스킬 DataAsset의 `EnhancementVariants`에 등록한다.

```text
BaseSkillData
└── EnhancementVariants
    ├── EnhancedSkillData_0
    └── EnhancedSkillData_1
```

교체 메커니즘:

```text
슬롯 SkillData 교체
-> 이후 입력부터 교체된 SkillData 기준으로 실행
-> Skill UI 갱신
```

강화 버전이 DataTable에서 다른 row를 읽어야 하면 강화 DataAsset의 `Skill_ID`를 다른 row로 지정한다.

즉, 강화 스킬은 원본 DataAsset 안에서 분기 플래그만 늘리는 방식이 아니라 "다른 DataAsset으로 교체"하는 방식이다.

교체를 실제로 수행하는 경로는 `ULSPlayerSkillComponent::ApplyEquippedSkillLoadout`이다. 세이브 로드아웃의 기본 `Skill_ID`를 해석 결과로 치환한 뒤 `SkillPool->FindSkillByID`로 DataAsset을 얻는다. 이 함수는 서버 권한/로컬 조종 가드를 이미 갖고 있다.

⚠️ `ApplySkillEnhancementByIndex`는 **교체 적용 경로로 쓰지 않는다.** RPC가 아니고 `SkillSlots`가 Replicated도 아니라 로컬 호출만 반영되며 멀티에서 깨진다. 현재 호출자가 없다.

## 스킬 추가 절차

새 액티브 스킬을 추가할 때 기본 순서:

```text
1. FLSCharacterSkillRow DataTable row를 추가한다.
2. 필요한 경우 ULSSkillDataAsset 파생 클래스를 만든다.
3. GameplayAbility 클래스를 만든다.
4. 필요한 GameplayEffect를 만든다.
5. 필요한 GameplayTag를 LSGameplayTags에 추가한다.
6. DataAsset을 만들고 AbilityClass, Skill_ID, CooldownTag, DamageEffectClass를 연결한다.
7. 프리뷰가 필요하면 PreviewSpec 또는 DataTable Range 값을 설정한다.
8. 선택 가능한 스킬이면 캐릭터 `ULSSkillPoolDataAsset`(`SelectableSkills`)에 DataAsset을 등록한다. (로비에서 4칸 중 하나로 고르게 된다. `스킬 로드아웃` 참고.)
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
2. 강화 버전의 Skill_ID를 필요한 row로 지정한다.
3. 원본 스킬 DataAsset의 EnhancementVariants에 등록한다.
   (강화 DataAsset은 SkillPool의 SelectableSkills에 등록하지 않는다 — 로드아웃 후보로 새면 안 된다.)
4. 획득 조건과 교체 트리거는 CharacterNodeSystem.md를 따른다.
5. 강화 후 UI와 쿨타임 태그가 의도대로 바뀌는지 확인한다.
```

## 에디터 설정 체크리스트

스킬 DataAsset:

```text
- AbilityClass 지정
- 액티브 Skill_ID 지정
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
- Activation Blocked Tags에 Dead/Stunned 등 차단 태그가 포함됐는지 (스킬끼리 차단은 SkillCasting, 기본공격 캔슬은 CancelAbilitiesWithTag — `기본 공격 캔슬과 스킬 차단 태그` 참고)
- Instancing Policy가 내부 상태 보유 방식과 맞는지
```

위젯:

```text
- SkillSlotWidget은 SkillData의 Icon/DisplayName을 표시한다.
- SkillBarWidget은 ULSPlayerSkillComponent의 슬롯 데이터를 읽는다.
- SkillBarWidget의 `bTextOverride`가 켜져 있으면 슬롯별 오버라이드 텍스트를 단축키 텍스트 대신 표시한다.
- 쿨타임 표시는 ASC의 CooldownTag remaining time 기준으로 연결한다.
```

## 금지/주의 사항

- HP, 공격력, 공격속도 같은 수치를 직접 수정하지 않는다.
- 스킬별 특수 필드를 공통 `ULSSkillDataAssetBase`나 액티브 `ULSSkillDataAsset`에 무한히 추가하지 않는다.
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
  -> ASC activates GameplayAbility (ULSGA_PlayerSkillBase)
  -> Commit + cooldown, then play SkillMontage
  -> LSAN_SkillEffect notify (LS.Event.Skill.Hit) or 즉발 if no montage
  -> ExecuteSkillEffect reads SkillData / DataTable
  -> Apply damage / buff GameplayEffect
  -> Montage end -> EndAbility

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
