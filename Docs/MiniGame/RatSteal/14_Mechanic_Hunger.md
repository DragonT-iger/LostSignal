# RatSteal Mechanic — Hunger (포만 게이지)

> 범위: **1차** (시간 압박의 핵심). HUD 표시는 [40_UI_HUD.md](40_UI_HUD.md), 수치는 [50_Content_Balance.md](50_Content_Balance.md)가 소유.

## 목적

시간 압박을 만드는 포만(fullness) 게이지 규칙을 정의한다.

## 규칙 (원작)

```text
시작값   1000
최대값   1000
감소     2초마다 -20 (초당 10) — 고정값, 시간 경과 가속 없음(코드 확인 완료)
회복     제출 시 점수만큼 (FeedBaby) — 21_Score 연계
패배     0 도달 시 BabyStarved (스토리상 정식 결말, 13_Baby)
```

## 설계 의도

```text
- 계속 훔쳐서 날라야만 게이지를 유지 → 정적 플레이 방지
- 제출 점수 = 회복량 이므로, 고득점 작물(호박50)일수록 생존에도 유리
```

## 확정 / 미정

```text
확정: 1차 포함. 감소율 고정(가속 없음). 패배는 BabyStarved(아기 굶주림) 정식 결말.
연계: 원작 HungryGauge는 Slide_Bar UI 바인딩 → 40_UI_HUD.
```
