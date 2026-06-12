# RatSteal Entity — Baby (아기쥐)

> 범위: **1차**. 포만 게이지는 [14_Mechanic_Hunger.md](14_Mechanic_Hunger.md)가 소유.

## 목적

플레이어가 돌봐야 하는 아기쥐(Baby)와 굶주림 패배 조건을 정의한다.

## 알려진 사실 (원작)

```text
- Player에 Baby 컴포넌트가 부착되어 있다(동행).
- FeedBaby(점수): 제출 점수만큼 포만(fullness) 회복, 최대 1000.
- 포만이 0에 도달하면 EndReason::BabyStarved 패배.
- 포만은 2초당 20 감소(초당 10, 고정).
- 스토리상 "아기가 배고파서 끝나는" 것이 정식 결말이다(핵심 동기).
```

## 미정 (2차에서 확정)

```text
- 아기쥐의 시각적 표현/위치(플레이어 등에 업힘 vs 둥지에 있음)
- 둥지(Home) 개념과 제출존의 관계 (원작 SpawnManager에 Home RECT 존재)
- 굶주림 단계별 연출(경고 UI/사운드)
```
