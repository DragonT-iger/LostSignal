# RatSteal Tutorial (튜토리얼 씬)

> 범위: **1차**. 빌드하는 두 씬 중 하나(다른 하나는 메인 [30_Level_Layout.md](30_Level_Layout.md)). 조작은 [03_Controls.md](03_Controls.md)가 소유.

## 목적

플레이어가 메인 게임에 들어가기 전에 핵심 조작·규칙을 익히는 튜토리얼 씬(MG_RatSteal_Tutorial)을 정의한다.

## 학습 목표 (코어 루프 순서대로)

```text
1. 이동        WASD/방향키로 쥐 이동
2. 훔치기      다 자란 작물에 닿아 Z → 인벤토리 적재(작물 제거)
3. 적재-감속   많이 들수록 느려짐 체감(16_Inventory)
4. 버리기      C로 짐을 버리면 즉시 빨라짐(도주 수단)
5. 제출-먹이기 제출존에 들어가 점수 획득 → 아기 포만 회복(21_Score/14_Hunger)
6. 은신        부쉬에 들어가면 Farmer가 못 봄(15_Stealth)
7. Farmer 회피 감지 반경/추적/공격을 안전하게 체험
```

## 진행 방식

```text
- 단계별 안내 문구(FText) + 목표 달성 시 다음 단계로.
- 안전한 환경: 초반 단계는 Farmer 비활성/순한 상태, 마지막 단계에서만 추적 체험.
- 포만/타이머는 튜토리얼에서 느리게 또는 정지(학습 방해 방지) — 정책 확인 필요.
- 완료 후 메인 게임 씬으로 전환(31_Flow). 스킵 가능.
```

## 구성 요소

```text
축소된 밭 1~2개, 작물 소량, 제출존 1개, 부쉬 1개, Farmer 1마리(후반 단계).
메인 씬과 같은 액터/규칙을 재사용하되 수치만 완화.
```

## UE 매핑 (요약)

```text
씬          → Content/LostSignal/MiniGame/RatSteal/Maps/MG_RatSteal_Tutorial
진행 제어   → ALSRatTutorialGameMode (ALSRatGameMode 상속 또는 플래그)
안내 UI     → WBP_RatStealTutorial (단계 문구/하이라이트)
```

## 미해결 / 확인 필요

```text
- 튜토리얼 중 포만/타이머 정지 vs 완화
- 강제 1회 vs 매번 스킵 가능
- 단계 수/순서 최종 확정(위 7단계 기준)
```
