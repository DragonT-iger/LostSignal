# RatSteal Mechanic — Inventory (인벤토리 / 적재 감속)

> 범위: **1차** (적재-감속 위험-보상의 핵심). 제출 정산은 [21_System_Score.md](21_System_Score.md), 작물 크기는 [12_Entity_Crop.md](12_Entity_Crop.md)가 소유. **수치는 원작을 그대로 따른다.**

## 목적

훔친 작물을 담는 인벤토리, 적재량에 따른 감속, 버리기(도주) 규칙을 정의한다.

## 슬롯 / 적재 (원작)

```text
슬롯 수    3 (Slot1/2/3, maxSlotNum=2)
적재       AddCrop(종류, 크기) — 같은 종류는 같은 슬롯에 누적, 종류당 1슬롯
슬롯 전환  ChangeSlot (X) — 0→1→2→0 순환
버리기     ThrowItem (C) — 현재 슬롯 카운트 1 감소
제출       SubMissonItem() → 모든 슬롯 ThrowAll → 점수 정산(21_Score) 후 비움
```

## 크기 → 카운트 단위 (중요)

원작 `Slot::AddItem`: 훔친 작물 크기가 슬롯 카운트에 누적된다.

```text
S 작물 →  +1 카운트
M 작물 →  +6 카운트   (size 2 × 3)
L 작물 →  +9 카운트   (size 3 × 3)
```

→ 이 카운트가 **점수와 감속 양쪽의 기준**이다. 큰 작물일수록 점수↑이자 감속↑.

## 감속 모델 (원작 — 실제 적용되는 식)

이동속도는 **적재 아이템 "개수"의 지수식**으로 정해진다. (무게값이 아니라 이 식이 실제 이동에 쓰임)

```text
SpeedMultiplier = (1 + 0.01)^감자카운트
                × (1 + 0.02)^가지카운트
                × (1 + 0.03)^호박카운트

이동속도 = max( 기본속도(500) / SpeedMultiplier, 최소속도(50) )

종류별 1카운트당 감속률: 감자 1% / 가지 2% / 호박 3% (복리)
```

예: 호박 L 1개(=9카운트) → SpeedMultiplier = 1.03^9 ≈ 1.30 → 속도 ≈ 500/1.30 ≈ 385.

## 버리기 → 즉시 가속 (도주)

```text
ThrowItem(C): 현재 슬롯 카운트 -1
→ 적재 카운트 감소 → SpeedMultiplier 즉시 하락 → 이동속도 즉시 상승
→ 추격당할 때 짐을 버려 순간적으로 빨라지는 도주 수단(점수 손실과 트레이드오프)
```

별도 부스트 변수가 아니라 위 감속식에서 **창발적으로** 발생한다.

## 원작 미사용 코드 (주의)

```text
GetWeight() : 종류 기본무게(가지25/감자10/호박50)에 카운트구간(count/3)별 계수
              (0→1.0, 1→0.9, 2→0.8, 3+→0.7)를 곱해 합산. → 화면/디버그용.
weightDivisor(300), weightMult = 1 + weight/300 : PlayerController에서 계산만 하고
              실제 이동엔 사용하지 않음(죽은 코드).
```

이식 시 **실제 적용식(SpeedMultiplier)만 채택**하고 GetWeight 경로는 가져오지 않는다. (살릴지 여부는 확인)

## UE 매핑 (요약)

```text
Inventory      → ULSRatInventoryComponent
슬롯           → 3슬롯 데이터(FLSRatSlotData: type/count/isEmpty)
감속           → ALSRatPlayer 이동 계산에 SpeedMultiplier 반영
보너스 계수    → C++ UPROPERTY 기본값/에디터 설정(감자0.01/가지0.02/호박0.03)
```

## 미해결 / 확인 필요

```text
- 버리기 "순간 가속"이 위 창발적 동작 그대로면 OK / 별도 대시 부스트를 의도하면 신규 명시
- GetWeight 경로(미사용) 도입 여부
```
