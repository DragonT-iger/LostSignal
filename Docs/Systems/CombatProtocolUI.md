# 전투 프로토콜 UI

## 목적

전투 프로토콜 UI는 전투 중 플레이어에게 공개되는 전투 정보를 프로토콜 단계별로 관리한다.

전투 판정, 데미지 계산, 체력 변경은 [CombatImplementationFlow.md](CombatImplementationFlow.md)와 [SkillSystemStructure.md](SkillSystemStructure.md)를 단일 출처로 둔다. 이 문서는 전투 결과를 어떤 UI로 보여줄지와 프로토콜 단계별 해금 기준만 다룬다.

## 단계 구성

전투 프로토콜은 5단계로 구성한다.

| 단계 | UI 범위 | 상태 |
|------|---------|------|
| 1 | 데미지 수치 표시 | 구현 대상 |
| 2 | 미정 | 보류 |
| 3 | 미정 | 보류 |
| 4 | 미정 | 보류 |
| 5 | 미정 | 보류 |

2~5단계는 기획 범위가 확정되면 이 문서에 추가한다. 아직 확정되지 않은 기능명이나 수치는 문서에 임의로 적지 않는다.

## 1단계: 데미지 수치 표시

1단계에서 해금되는 기능은 적 또는 플레이어가 피해를 받을 때 해당 월드 위치 주변에 데미지 숫자를 표시하는 것이다.

분류는 월드 위치 기반 UI다. 다만 실제 렌더링은 각 클라이언트의 `WBP_PlayerHUD`에서 월드 좌표를 화면 좌표로 투영해 표시한다. 짧게 많이 생성되는 UI라서 대상 액터마다 `WidgetComponent`를 붙이지 않고, HUD가 `ULSDamageNumberWidget` 풀을 관리한다.

## 해금 기준

데미지 수치 표시는 `ELSProtocolType::Battle` 프로토콜의 1단계 기능이다.

`DT_ProtocolUnlock`에는 다음 의미의 row를 둔다.

```text
Protocol_Enable_Type = Battle
Protocol_Enable_Name = Damage_Number
Protocol_Required_Level = 1
```

정확한 row 이름은 프로토콜 데이터 테이블 규칙을 따른다. UI 코드는 `ULSGameDataSubsystem::FindProtocolUnlockRowByEnableName(ELSProtocolType::Battle, "Damage_Number")`로 조회한다.

테스트 중에는 `LSTestBattleProtocol 1`로 1단계 표시 여부를 확인한다.

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

## 현재 제한

- 1차 구조에서는 `BeforeHealth - AfterHealth`로 실제 적용 데미지만 표시한다.
- 치명타 여부는 아직 `ULSDamageExecutionCalculation` 밖으로 전달되지 않으므로 `bCritical=false`로 시작한다.
- 치명타 색상, 막힘, 회복, 속성별 색상은 이후 단계나 별도 확정 요구가 있을 때 `FLSDamageNumberPayload`를 확장한다.
- 데미지 숫자 표시 위젯 클래스는 `ALSPlayerControllerBase`가 생성한 `ULSPlayerHUDWidget`의 `DamageNumberWidgetClass`에 BP에서 지정한다.
