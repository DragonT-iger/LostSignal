# RatSteal System — Timer (타이머 / 라운드 종료)

> 범위: **1차**. 제한 시간/난이도는 [02_Progression.md](02_Progression.md), 종료 처리는 [01_CoreLoop.md](01_CoreLoop.md)가 소유.

## 목적

한 판의 제한 시간과 종료 트리거를 정의한다.

## 규칙

```text
시작     Playing 진입 시 카운트다운 시작
길이     3분 (확정)
표시     HUD에 남은 시간(40_UI_HUD)
종료     남은 시간 0 → End 트리거 (생존 시 점수/등급 산정, 스코어 어택)
```

## 종료 트리거 우선순위

```text
1. PlayerDead   HP 0          → 즉시 End
2. BabyStarved  포만 0        → 즉시 End
3. TimeUp       남은 시간 0   → End (생존 시 Happy/점수 등급)
```

종료 시 결과 산정 및 화면 전환은 [01_CoreLoop.md](01_CoreLoop.md) 승/패 조건과 [41_UI_Menus.md](41_UI_Menus.md)를 따른다.

## UE 매핑 (요약)

```text
타이머       → ALSRatGameMode가 보유(SetTimer 또는 Tick 누적)
종료 처리    → ALSRatGameMode::EndGame(EReason)
시간값       → C++ UPROPERTY 기본값/에디터 설정
```

## 미해결 / 확인 필요

```text
- 일시정지 중 타이머 정지 처리
- 종료 직전 마지막 제출 인정 범위(타이머 0 시점 동시 처리)
```
