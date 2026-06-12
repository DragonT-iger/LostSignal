# RatSteal System — Spawn (농작물 스폰)

> 범위: **1차**. 작물 정의는 [12_Entity_Crop.md](12_Entity_Crop.md), 수치는 [50_Content_Balance.md](50_Content_Balance.md)가 소유.

## 목적

밭에 농작물이 시간에 따라 생성·성장하는 규칙을 정의한다.

## 밭 구조 (원작 SpawnManager)

```text
밭 영역    farm_A / farm_B / farm_C (RECT) + Home (RECT)
랭크별 밭  A / B / C — 각 밭이 자기 랭크의 작물을 스폰
스폰 간격  밭별 spawnTime마다 maxRate까지 작물 채움
충돌 회피  신규 스폰 위치는 기존 작물과 spawnRange(200) 이상 떨어뜨림
```

## 성장 속도 (랭크별, 초)

```text
       Born→S   S→M    M→L
RankA    5       7      7
RankB    5       7     12
RankC    5      12      0(=M 최대, L 없음)
```

(원작 SpawnManager 기본값. 구현 수치는 C++ 기본값과 에디터 설정으로 관리하고, 근거는 [50_Content_Balance.md](50_Content_Balance.md)를 따른다.)

## 작물 종류 확률

```text
밭 랭크마다 RandomCrop(가지확률, 감자확률, 호박확률)로 종류 결정.
→ 고랭크 밭일수록 고가치(호박) 비중↑ 형태로 설계(구체 확률 확정 필요).
```

## 동작 흐름 (원작 근거)

```text
Update
- 각 밭(FarmData)마다 elapsedTime 누적
- spawnTime 도달 + 현재 작물 수 < maxRate 이면 CreateNewCrop(rank)
  - CreateSpawnPoint: 밭 RECT 내부, 기존 작물과 spawnRange 이상 떨어진 위치
  - SetCropType: 랭크별 확률로 종류 결정
  - SetCropData: 성장속도/스프라이트/이펙트 주입 → isSpawn=true
```

## UE 매핑 (요약)

```text
SpawnManager → ALSRatSpawnManager (AActor) 또는 GameMode 책임
밭 영역      → 박스 볼륨/AActor 마커 (farm_A/B/C, Home)
스폰 규칙    → 타이머 + 거리 검사
수치        → C++ UPROPERTY 기본값 / 에디터 설정 (랭크별 maxRate/spawnTime/growSpeed)
```

## 미해결 / 확인 필요

```text
- 밭별 maxRate / spawnTime 구체값(원작 초기화부 추가 확인 필요)
- 랭크별 작물 확률 구체값
- Home(RECT)의 역할(둥지/제출존과의 관계) → 30_Level_Layout / 13_Baby
```
