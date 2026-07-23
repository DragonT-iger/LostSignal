# 전투 프로토콜 UI

## 목적

전투 프로토콜 UI는 전투 중 플레이어에게 공개되는 전투 정보를 프로토콜 단계별로 관리한다.

데이터 테이블과 연계되는 변수의 이름은 반드시 데이터 테이블 기준으로 변수명을 설정한다.

데이터 테이블에 존재하지 않는 변수명을 데이터 테이블에 추가하지 않는다.

전투 판정, 데미지 계산, 체력 변경은 [CombatImplementationFlow.md](CombatImplementationFlow.md)와 [SkillSystemStructure.md](SkillSystemStructure.md)를 단일 출처로 둔다. 이 문서는 전투 결과를 어떤 UI로 보여줄지와 프로토콜 단계별 해금 기준만 다룬다.

## 단계 구성

전투 프로토콜은 5단계로 구성한다.

| 단계 | UI 범위 | 구현 방향 |
|------|---------|-----------|
| 1 | 데미지 수치 표시, 스킬 슬롯 UI 표시 | 데미지 수치는 `ULSDamageNumberWidget`, 스킬 슬롯은 기존 `ULSSkillBarWidget`/`ULSSkillSlotWidget` 사용 |
| 2 | 스킬 범위 표시, 스킬 쿨타임 숫자 UI 표시 | 스킬 범위는 기존 `ULSSkillPreviewComponent` 제어 흐름 사용, 쿨타임 숫자는 스킬 슬롯 UI 확장 |
| 3 | 캐스팅, 버프 지속 시간, 스킬 쿨타임 게이지바 UI 표시 | 캐스팅 게이지와 버프 표시 UI 추가 필요, 쿨타임 게이지는 스킬 슬롯 UI 확장 |
| 4 | 적 공격 타이밍 및 공격 범위 UI 표시 | 적 공격 범위/타이밍 표시 구조 추가 필요 |
| 5 | 적 체력바 UI 표시 | `ULSEnemyHealthBarComponent`가 월드 스페이스 `UWidgetComponent` 체력바를 표시 |

4단계의 `적 하단 원형 표시`는 기획에서 제외되었으므로 구현하지 않는다.

이미 해금된 정보가 일정 단계까지 사라지지 않는 규칙은 `DT_ProtocolUnlock`의 `Protocol_Protected_Level`로 관리한다. 전투 프로토콜에서는 Lv.3, Lv.5 기준의 정보 유지 규칙을 데이터로 표현하고, 코드에는 특정 단계 수치를 하드코딩하지 않는다.

## 1단계: 데미지 수치 표시

1단계에서 해금되는 기능은 적 또는 플레이어가 피해를 받을 때 해당 월드 위치 주변에 데미지 숫자를 표시하고, 플레이어 스킬 슬롯 UI를 표시하는 것이다.

분류는 월드 위치 기반 UI다. 다만 실제 렌더링은 각 클라이언트의 `WBP_PlayerHUD`에서 월드 좌표를 화면 좌표로 투영해 표시한다. 짧게 많이 생성되는 UI라서 대상 액터마다 `WidgetComponent`를 붙이지 않고, HUD가 `ULSDamageNumberWidget` 풀을 관리한다.

스킬 슬롯 UI는 기존 `ULSPlayerHUDWidget`이 `ULSSkillBarWidget`을 초기화하는 흐름을 사용한다. 1단계에서는 새 스킬 슬롯 클래스를 만들지 않는다.

## 해금 기준

데미지 수치 표시는 `ELSProtocolType::Battle` 프로토콜의 1단계 기능이다.

`DT_ProtocolUnlock`에는 다음 의미의 row를 둔다.

```text
Protocol_Enable_Type = Battle
Protocol_Enable_Name = Damage_Number
Protocol_Required_Level = 1
```

스킬 슬롯 UI도 전투 프로토콜 1단계부터 노출되어야 한다. `DT_ProtocolUnlock`에는 다음 의미의 row를 추가한다.

```text
Protocol_Enable_Type = Battle
Protocol_Enable_Name = Skill_Slot
Protocol_Required_Level = 1
```

정확한 row 이름은 프로토콜 데이터 테이블 규칙을 따른다. UI 코드는 `ULSGameDataSubsystem::FindProtocolUnlockRowByEnableName`으로 `Damage_Number`, `Skill_Slot` 해금명을 각각 조회한다.

테스트 중에는 `LSTestBattleProtocol 1`로 1단계 표시 여부를 확인한다.

## 2단계: 스킬 범위와 쿨타임 숫자

2단계에서 해금되는 기능은 스킬 사용 전 범위 표시와 스킬 슬롯의 쿨타임 남은 시간 숫자 표시다.

스킬 범위는 기존 `ULSPlayerSkillComponent`의 스킬 타겟팅 흐름과 `ULSSkillPreviewComponent`를 사용한다. 전투 프로토콜 2단계 미만에서는 스킬 타겟팅과 확정 입력은 유지하되, 월드 범위 메시만 표시하지 않는다. 즉 프로토콜은 조작 가능 여부가 아니라 정보 표시량만 제어한다.

쿨타임 숫자는 `ULSSkillSlotWidget`의 `CooldownText`만 제어한다. 3단계 기능인 쿨타임 게이지는 `CooldownMaskImage`로 분리하고, 2단계에서는 표시하지 않는다.

`DT_ProtocolUnlock`에는 다음 의미의 row를 추가한다.

```text
Protocol_Enable_Type = Battle
Protocol_Enable_Name = Skill_Range
Protocol_Required_Level = 2
```

디버그 예외:

- `ULSPlayerSkillComponent::bAlwaysShowSkillPreviewDebug`가 켜져 있으면 전투 프로토콜 레벨과 관계없이 스킬 범위 프리뷰 메시를 표시한다.
- 이 플래그는 로컬 디버깅 전용이다. 쿨타임, 스킬 데이터 유효성, 서버 발동 판정은 우회하지 않는다.

```text
Protocol_Enable_Type = Battle
Protocol_Enable_Name = Skill_Cooldown
Protocol_Required_Level = 2
```

3단계 쿨타임 게이지바는 다음 해금명을 사용한다.

```text
Protocol_Enable_Type = Battle
Protocol_Enable_Name = Skill_Cooldown_Gauge
Protocol_Required_Level = 3
```

## 3단계: 캐스팅, 버프 지속 시간, 쿨타임 게이지바

3단계에서 해금되는 기능은 스킬 캐스팅 게이지바, 버프 지속 시간 표시, 스킬 쿨타임 게이지바 표시다.

스킬 쿨타임 게이지는 `ULSSkillSlotWidget`의 `CooldownMaskImage`(UImage)가 담당한다. 선형 진행바가 아니라 아이콘과 같은 크기로 겹쳐 아이콘 위를 덮는 방사형(시계방향 파이 와이프) 마스크이며, 브러시 머티리얼의 `Progress` 스칼라 파라미터(0~1)를 `GetDynamicMaterial()`로 얻은 MID에 매 틱 넣어 구동한다. 기본은 남은시간/총시간(부채꼴이 줄어듦), `bCooldownFillByElapsed`로 진행도 방향(차오름) 전환. 2단계의 `Skill_Cooldown`은 숫자만 표시하고, 3단계의 `Skill_Cooldown_Gauge`가 열려야 게이지가 표시된다.

버프 지속 시간은 `ULSCombatBuffListWidget`이 플레이어의 AbilitySystemComponent에서 지속 중인 `LS.Buff.*` 계열 GameplayEffect를 조회해 표시한다. 현재 표시 대상은 `LS.Buff.CombatAcceleration`, `LS.Buff.AttackSpeed`다. 버프 이름/설명/아이콘 같은 표시 정보는 위젯 로컬 텍스처가 아니라 활성 GameplayEffect의 SourceObject에 들어 있는 `ULSSkillDataAssetBase` DataAsset에서 먼저 읽고, SourceObject가 없는 예외만 태그별 fallback DataAsset을 쓴다. 개별 버프 표시는 `ULSCombatBuffIconWidget`이 담당한다. 남은시간 게이지는 스킬 쿨타임과 동일하게 `DurationMaskImage`(UImage) + MID의 `Progress` 파라미터로 구동하는 방사형(시계방향 파이 와이프) 마스크다.

스킬 캐스팅 게이지바는 `ULSSkillCastGaugeWidget`이 담당한다. 아직 실제 캐스팅 시스템이 없으므로 HUD의 `ShowSkillCastGauge`/`HideSkillCastGauge` API와 디버그 명령 `LSTestSkillCastGauge <Duration>`으로 표시 경로를 먼저 열어 둔다.

`DT_ProtocolUnlock`에는 다음 의미의 row를 추가한다.

```text
Protocol_Enable_Type = Battle
Protocol_Enable_Name = Skill_Casting_Gauge
Protocol_Required_Level = 3
```

```text
Protocol_Enable_Type = Battle
Protocol_Enable_Name = Buff_Duration
Protocol_Required_Level = 3
```

## 5단계: 적 체력바

5단계에서 해금되는 기능은 살아 있는 적의 머리 위에 현재/최대 체력바를 표시하는 것이다.

적 체력바는 `WBP_PlayerHUD` 뷰포트 위젯이 아니라 `ALSEnemyCharacter`에 붙은 `ULSEnemyHealthBarComponent`가 월드 스페이스 `UWidgetComponent`로 표시한다. 컴포넌트는 로컬 플레이어 기준으로 전투 프로토콜 5단계 표시 여부를 확인하고, 적의 AbilitySystemComponent에서 `ULSCombatAttributeSet::CurrentHealth`와 `MaxHealth` 변경 delegate를 구독해 채움 비율을 갱신한다. 화면 텍스트는 표시하지 않는다.

체력바 위젯은 `ULSEnemyHealthBarWidget`이 담당하며, 필수 바인딩은 `HealthProgressBar` 하나다. 모든 몬스터가 공유하는 위젯 클래스는 `ULSMonsterPresentationSettings::EnemyHealthBarWidgetClass`에서 한 번만 설정한다. `ULSEnemyHealthBarComponent`는 몬스터별 배치값인 `WidgetOffset`, `DrawSize`, `Pivot`, `WidgetSpace`만 에디터에 노출한다. `WidgetSpace`가 `World`이고 `bFaceCamera`가 켜져 있으면 Tick에서 로컬 카메라를 향하도록 위젯 컴포넌트만 회전시킨다.

`DT_ProtocolUnlock`에는 다음 의미의 row를 둔다.

```text
Protocol_Enable_Type = Battle
Protocol_Enable_Name = Enemy_Health_Bar
Protocol_Required_Level = 5
```

현재 샌드박스 데이터에는 같은 의미의 기존 이름 `Enemy_HP`가 남아 있을 수 있다. 코드에서는 `Enemy_Health_Bar`를 먼저 찾고, 없으면 `Enemy_HP`를 호환 이름으로 조회한다. 새 데이터는 `Enemy_Health_Bar`를 기준으로 작성한다.

## 표시 흐름

```text
서버 데미지 판정
-> ULSCharacterCombatComponent::ApplyDamageEffectToTarget
-> GameplayEffect 적용 전/후 CurrentHealth 비교
-> 실제 적용 데미지 산출
-> 모든 ALSPlayerControllerBase에 데미지 표시 이벤트 전달
-> 각 로컬 ULSPlayerHUDWidget이 전투 프로토콜 1단계 해금 여부 확인
-> ULSDamageNumberWidget 풀에서 위젯 획득
-> 월드 위치를 화면 좌표로 투영해 숫자 표시
```

모든 플레이어에게 보여야 하므로 공격자에게만 Client RPC를 보내지 않는다. 서버가 현재 월드의 PlayerController를 순회해 각 클라이언트에 표시 이벤트를 보낸다.

데미지 숫자는 게임 판정이 아니라 표시 효과다. 따라서 전달 RPC는 누락 가능성을 허용하는 `Unreliable`을 기본으로 한다.

## 코드 책임

| 코드 | 책임 |
|------|------|
| `ULSCharacterCombatComponent` | 서버 데미지 확정 후 실제 데미지와 표시 위치를 만든다. |
| `ALSPlayerControllerBase` | 서버에서 모든 클라이언트로 데미지 표시 이벤트를 전달한다. |
| `ULSPlayerHUDWidget` | 전투 프로토콜 1단계 해금 여부를 확인하고 데미지 숫자 위젯 풀을 관리한다. |
| `ULSDamageNumberWidget` | 숫자 하나의 텍스트, 투영 위치, 상승/페이드 수명을 담당한다. |
| `FLSDamageNumberPayload` | 데미지량, 월드 위치, 치명타 여부 같은 표시 입력값을 담는다. |
| `ULSSkillBarWidget` | `Skill_Slot` 해금 여부에 따라 스킬 슬롯 UI 표시 여부를 결정한다. |
| `ULSSkillSlotWidget` | `Skill_Cooldown`/`Skill_Cooldown_Gauge` 해금 여부에 따라 쿨타임 숫자(`CooldownText`)와 방사형 게이지(`CooldownMaskImage`+MID)를 분리 표시한다. |
| `ULSPlayerSkillComponent` | `Skill_Range` 해금 여부에 따라 스킬 범위 메시 표시 여부를 결정한다. |
| `ULSChipStationWidget` | 디버그 패널이 떠 있고 오버라이드가 설정됐을 때만 그 값을, 아니면 장착 칩 프로토콜 합산값을 프리뷰로 전달한다. |
| `ULSCombatBuffListWidget` | `Buff_Duration` 해금 여부에 따라 플레이어 버프 지속 시간 목록을 표시한다. |
| `ULSCombatBuffIconWidget` | 버프 하나의 아이콘 이미지, 스택 텍스트, 남은 시간 방사형 게이지(`DurationMaskImage`+MID)를 표시한다. |
| `ULSSkillCastGaugeWidget` | `Skill_Casting_Gauge` 해금 여부에 따라 캐스팅 진행률을 표시한다. |
| `ULSEnemyHealthBarComponent` | 전투 프로토콜 5단계 해금 여부에 따라 적 머리 위 월드 스페이스 위젯 체력바를 표시한다. |
| `ULSEnemyHealthBarWidget` | 텍스트 없이 `HealthProgressBar`의 체력 비율만 표시한다. |

## 칩스테이션 프리뷰

`WBP_ChipStation`에는 전투 프로토콜 프리뷰용 `SkillBar` 자식 위젯을 둔다. 이 위젯의 부모 클래스는 `ULSSkillBarWidget`이다.

칩스테이션은 기본적으로 장착 칩의 전투 프로토콜 합산값(현재=신호 활성 칩, 이전=전체 칩)을 `SetPreviewBattleProtocol`을 통해 `ULSSkillBarWidget::SetPreviewBattleProtocolLevels`에 전달한다. 프로토콜 디버그 패널이 떠 있고 오버라이드가 설정됐을 때만 그 디버그 값을 따른다. 신호율 슬라이더는 활성 칩 집합을 바꿔 현재 레벨에 반영된다. 이 프리뷰 값은 칩스테이션 내부 표시 전용이며 실제 플레이 HUD의 전투 프로토콜 레벨을 변경하지 않는다. 디버그 패널을 토글하거나 값을 바꾸면 `ALSPlayerControllerBase::RefreshProtocolTestTargets`가 열린 칩스테이션을 다시 그려 로비에서도 즉시 반영된다.

스킬 범위 표시는 새 `UUserWidget`을 만들지 않고 기존 `ULSSkillPreviewComponent`와 `FLSSkillAreaPreviewSpec` 흐름을 사용한다. 적(몬스터) 공격 범위 표시(텔레그래프)도 같은 `ULSSkillPreviewComponent`/`FLSSkillAreaPreviewSpec`를 재사용한다 — 몬스터에 부착된 프리뷰 컴포넌트를 `ULSMonsterCombatComponent`가 액션 row(`DT_MonsterAction`)의 히트박스로 구동한다([MonsterAIControlStructure.md](MonsterAIControlStructure.md) 공격 제어 규칙 참고). 현재는 항상 표시하되, `ULSMonsterCombatComponent::ShouldShowActionTelegraph()`가 전투 프로토콜 레벨 게이팅을 끼울 확장점이다(여기서 프로토콜 해금에 따라 표시 여부를 결정하도록 확장 예정).

## 현재 제한

- 데미지 숫자는 `BeforeHealth - AfterHealth`가 아니라 `ULSDamageExecutionCalculation`이 산출한 최종 데미지량을 표시한다.
- 데미지량과 치명타 여부는 `ULSDamageExecutionCalculation`의 판정 결과를 `ULSCharacterCombatComponent`가 받아 `FLSDamageNumberPayload`로 전달한다.
- 치명타 데미지 색상은 `ULSDamageNumberWidget::CriticalDamageColor`에서 따로 지정한다. 막힘, 회복, 속성별 색상은 이후 단계나 별도 확정 요구가 있을 때 `FLSDamageNumberPayload`를 확장한다.
- 데미지 숫자 표시 위젯 클래스는 `ALSPlayerControllerBase`가 생성한 `ULSPlayerHUDWidget`의 `DamageNumberWidgetClass`에 BP에서 지정한다.
