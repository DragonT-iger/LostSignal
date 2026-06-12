# RatSteal System — Score (점수 / 제출 정산)

> 범위: **1차**. 제출존 위치는 [30_Level_Layout.md](30_Level_Layout.md), 포만 회복 연계는 [14_Mechanic_Hunger.md](14_Mechanic_Hunger.md)가 소유.

## 목적

작물 제출 시 점수 정산과 누적 규칙을 정의한다.

## 점수표 (원작 GameManager, 단일 출처)

```text
가지(Eggplant)  25
감자(Potato)    10
호박(Pumpkin)   50
```

```text
curScore = (가지카운트 * 25) + (감자카운트 * 10) + (호박카운트 * 50)
totalscore += curScore
```

여기서 "카운트"는 슬롯 누적 카운트이며 **작물 크기가 이미 반영**되어 있다(S+1/M+6/L+9, 12_Crop/16_Inventory). 즉 크기가 클수록 점수↑. 예: L 호박 1개 = 9 × 50 = 450점.

## 제출 흐름 (원작 근거)

```text
플레이어가 SubMissionArea 진입
- score = GameManager.ReceiveScore( Inventory.SubMissonItem() )
- 인벤토리의 슬롯 데이터(종류/개수)를 합산해 curScore 산출
- totalscore에 누적
- 반환된 score로 FeedBaby(score) → 포만 회복(14_Hunger)
- 제출 후 인벤토리 비움(슬롯 정리)
```

→ **점수 = 베이비 회복량.** 점수 시스템이 생존과 직결된다.

## 누적 카운트

```text
ep_count / pt_count / pk_count   종류별 누적 수집 수
totalscore                       총점
```

결과 화면(41_UI_Menus)에서 종류별 수집 수/총점 표기에 사용.

## UE 매핑 (요약)

```text
GameManager.ReceiveScore → ALSRatGameMode::SubmitInventory(...)
점수 테이블              → C++ 기본값/헬퍼 함수 (종류별 점수)
HUD 점수 표시           → 40_UI_HUD
```

## 미해결 / 확인 필요

```text
- 제출은 원작 기준 전량(모든 슬롯 ThrowAll) → 유지 여부
- 콤보/연속 제출 보너스 도입 여부(신규 디자인)
- 점수 등급 컷(02_Progression, 신규)
```
