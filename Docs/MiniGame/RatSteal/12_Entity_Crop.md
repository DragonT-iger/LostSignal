# RatSteal Entity — Crop (농작물)

> 범위: **1차**. 점수는 [21_System_Score.md](21_System_Score.md), 스폰/성장속도는 [20_System_Spawn.md](20_System_Spawn.md), 수치는 [50_Content_Balance.md](50_Content_Balance.md)가 소유.

## 목적

수집 대상인 농작물의 종류, 성장 단계, 가치를 정의한다.

## 종류 (원작 Crops enum)

```text
Eggplant  가지   제출 점수 25
Potato    감자   제출 점수 10
Pumpkin   호박   제출 점수 50
Nothing   (없음 / 미스폰)
```

점수 정산식은 [21_System_Score.md](21_System_Score.md)가 단일 출처. (가지25 / 감자10 / 호박50)

## 성장 단계 (Size enum)

```text
Born → S → M → L   (시간 경과로 성장)
- Born: 막 심긴 상태. 콜라이더 비활성 → 훔치기 불가
- S   : 콜라이더 활성 → 이때부터 훔치기 가능
- M   : 중간
- L   : 최대(이펙트 표시). 큰 작물일수록 가치/무게 큼(연계: 16_Inventory)
```

성장 시간은 밭 랭크별로 다르다 → [20_System_Spawn.md](20_System_Spawn.md).

### 크기 → 점수/감속 (중요)

훔칠 때 작물 크기가 인벤토리 슬롯 카운트로 누적된다(원작 Slot::AddItem).

```text
S 작물 → +1 카운트
M 작물 → +6 카운트
L 작물 → +9 카운트
```

점수·감속 모두 이 카운트 기준이라 **큰 작물일수록 점수↑이자 감속↑**.
예: L 호박 1개 = 9카운트 × 50점 = 450점. (21_Score / 16_Inventory)

## 밭 랭크 (FarmRank)

```text
Rank_A   최대 성장 L까지
Rank_B   최대 성장 L까지
Rank_C   최대 성장 M까지 (저급 밭)
```

랭크는 작물 종류 확률과 성장 속도를 결정한다(스폰 매니저 소유).

## 동작 규칙 (원작 Crop.cpp 근거)

```text
성장
- isSpawn이 true이고 현재 크기 < 최대 크기일 때만 성장
- 단계별 누적 시간이 growSpeed_X를 넘으면 다음 크기로 전환하며 스프라이트 교체
- S 도달 시 BoxCollider 활성(훔치기 가능)
- L 도달 시 이펙트 오브젝트 활성 + Y정렬 보정

훔치기
- 플레이어가 Steal하면 종류/크기를 인벤토리에 넘기고 작물 오브젝트 제거(이펙트 포함)
```

## UE 매핑 (요약)

```text
Crop         → ALSRatCrop (AActor)
성장         → 타이머/누적시간 → 단계별 UPaperSprite 교체
표시 크기     → 단계별 Visual Scale을 함께 조정해 Born/S/M/L 성장감을 보강
콜라이더     → UBoxComponent (S 단계부터 활성)
종류/랭크/크기 → FLSRatCropRow (DataTable) 또는 데이터 에셋
이펙트       → UPaperFlipbook(반짝임) 자식
```

## 미해결 / 확인 필요

```text
- 크기→카운트 누적량(S+1/M+6/L+9)을 그대로 유지할지(점수 스케일이 큼 — L 호박 450점)
- "Nothing" 처리 방식(스폰 실패/빈 칸)
```
