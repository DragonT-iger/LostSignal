# RatSteal Flow — Entry / Return (진입 / 복귀)

> 범위: **1차**. 본편 레벨 흐름은 [../../Systems/RaidLevelFlow.md](../../Systems/RaidLevelFlow.md) 참고(레벨 전환 패턴 차용).

## 목적

본편 LostSignal에서 미니게임으로 진입하고, 종료 후 본편으로 복귀하는 흐름을 정의한다. 진입 방식은 **월드 내 상호작용 오브젝트**.

## 흐름

```text
1. 본편 월드에 RatSteal 캐비닛(오락기/단말기) 액터 배치
2. 플레이어가 본편 IA_Interact로 캐비닛 상호작용
3. 캐비닛이 복귀 정보(현재 맵/위치/상태)를 서브시스템에 저장
4. RatSteal 레벨로 전환(OpenLevel) → 타이틀(오버레이)
5. 첫 플레이/선택 시 튜토리얼 씬(MG_RatSteal_Tutorial, 32_Tutorial)
6. 메인 게임 씬(MG_RatSteal)에서 미니게임 진행(01_CoreLoop)
7. 종료(시간초과/패배) → 결과 오버레이(41_UI_Menus)
8. 저장된 본편 맵/위치로 복귀(OpenLevel)
9. 필요 시 결과(점수)를 서브시스템 경유로 본편에 전달
```

## 책임 분리

```text
ALSRatStealCabinet (본편)
- 상호작용 진입점. 복귀 정보 저장 후 레벨 전환 요청.

ULSRatStealSubsystem (UGameInstanceSubsystem)
- 복귀 맵/위치 보관, 미니게임 결과(점수 등) 보관.
- 본편↔미니게임 사이의 유일한 데이터 통로(느슨한 결합).

ALSRatGameMode (미니게임 레벨)
- 미니게임 규칙/종료. 종료 시 서브시스템에 결과 기록 후 복귀 요청.
```

## 격리 원칙

```text
- 미니게임은 본편 GAS/세이브/인벤토리에 직접 접근하지 않는다.
- 결과 전달은 서브시스템 경유로만. 본편 진행에 영향 주지 않는 것이 기본.
- 미니게임 전용 GameMode/PlayerController/IMC로 본편 입력·카메라와 분리.
```

## UE 매핑 (요약)

```text
레벨 전환   → UGameplayStatics::OpenLevel(BySoftObjectPtr)
복귀 정보   → ULSRatStealSubsystem (GameInstance 수명)
진입 오브젝트 → ALSRatStealCabinet + 본편 상호작용 인터페이스
```

## 미해결 / 확인 필요

```text
- 진입 시 본편 일시정지/스트리밍 처리(별도 맵이므로 OpenLevel 기본)
- 결과 보상이 본편에 영향 줄지(점수만 기록 vs 아이템 보상)
- 캐비닛 배치 위치(본편 어느 레벨)
```
