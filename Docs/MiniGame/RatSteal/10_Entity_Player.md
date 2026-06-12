# RatSteal Entity — Player (쥐)

> 범위: **1차**. 입력은 [03_Controls.md](03_Controls.md), 무게/감속은 [16_Mechanic_Inventory.md](16_Mechanic_Inventory.md), 포만은 [14_Mechanic_Hunger.md](14_Mechanic_Hunger.md), 수치는 [50_Content_Balance.md](50_Content_Balance.md)가 소유.

## 목적

플레이어 캐릭터(쥐)의 상태·능력·상호작용 규칙을 정의한다.

## 상태값 (원작 기준)

```text
State       Alive / Dead
Action      Idle / Walk / Hit / Steal      (애니메이션 상태)
Visibility  Visible / Hide                 (부쉬 안이면 Hide → Farmer 추적 해제)
HP          3 (시작)
Fullness    1000 시작 / 1000 최대          (베이비 포만, 2초당 20 고정 감소)
이동속도    기본 500 / 최소 50 / 적재 개수 지수식으로 감속 (16_Inventory)
무적 간격   원작 ivc_T = 15 (프레임 기준). 피격 후 무적 처리 기준값 (50_Balance)
콜라이더    150 x 150 BoxCollider
```

## 능력 / 행동

```text
이동      입력 방향 정규화 후 (기본속도/속도배수, 최소속도) 적용
훔치기    겹친 작물에 Steal → 인벤토리 적재 + 작물 제거 (03_Controls)
제출      SubMissionArea 진입 시 인벤토리 정산 → 점수 → FeedBaby(점수) (21_Score)
은신      Bush 태그 Overlap 시 Hide, Exit 시 Visible (15_Stealth)
피격      Farmer 공격 적중 시 HP-1, 이후 무적(원작 ivc_T=15 프레임 기준)
```

## 핵심 동작 규칙 (원작 Player.cpp 근거)

```text
포만 감소
- 2초마다 fullness -= 20 (초당 10, 고정값·가속 없음). 0 도달 시 BabyStarved 패배.
- 스토리상 "아기가 배고파서" 끝나는 정식 결말이다(13_Baby).

피격 무적
- 피격 시 HP-1 후 무적(연속 피격 방지). **원작값 ivc_T=15(프레임 기준)을 따른다.**
- 원작은 접촉 지속 중 15프레임마다 피격 판정(OnTriggerStay)으로 구현(코드상 주석 상태).
- UE 이식 시 프레임→초 변환(예: 60fps면 약 0.25s)은 픽셀→cm 환산처럼 플레이하며 미세조정.

제출 → 먹이기
- OnTrigger(SubMissionArea): score = ReceiveScore(인벤토리 아이템); FeedBaby(score)
- FeedBaby(bop): fullness = min(fullness + bop, 1000)
- 즉, 제출 점수가 곧 베이비 회복량이다. (점수와 생존이 직결)

은신
- OnTriggerEnter(Bush): Visibility = Hide
- OnTriggerExit(Bush): Visibility = Visible
- Farmer는 플레이어가 Hide면 Chase/Attack을 멈추고 Patrol로 복귀(11_Farmer).

사망
- HP가 0 이하가 되면 State = Dead → PlayerDead 패배.
```

## UE 매핑 (요약, 상세는 99_TechMapping)

```text
Player           → ALSRatPlayer (APawn 또는 ACharacter 경량)
이동/입력        → ULSRatPlayerController + Enhanced Input
스프라이트/애니  → UPaperFlipbookComponent (Idle/Walk/Steal/Hit)
콜라이더         → UBoxComponent (Overlap 이벤트)
포만/HP          → ALSRatPlayer 멤버 또는 ULSRatStatusComponent
```

본편 GAS는 쓰지 않는다(미니게임 경량화). HP/포만은 단순 멤버 + 이벤트로 처리.

## 미해결 / 확인 필요

```text
- 무적: 원작 ivc_T=15(프레임)의 초 변환은 플레이로 미세조정
- 포만은 코드상 Player 멤버지만 결말이 BabyStarved → "아기 포만"으로 표기 통일(13_Baby)
```
