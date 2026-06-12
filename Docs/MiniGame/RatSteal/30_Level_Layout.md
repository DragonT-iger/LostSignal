# RatSteal Level — Layout (맵 레이아웃)

> 범위: **1차**. 스폰 규칙은 [20_System_Spawn.md](20_System_Spawn.md), 진입/복귀는 [31_Flow_EntryReturn.md](31_Flow_EntryReturn.md)가 소유.

## 목적

메인 게임 씬(MG_RatSteal)의 맵 레이아웃, 밭/제출존/은신처/둥지의 배치와 동선을 정의한다. 튜토리얼 씬은 [32_Tutorial.md](32_Tutorial.md)가 소유.

## 빌드 씬 범위

```text
빌드하는 씬은 2개:
- MG_RatSteal           메인 게임 씬 (이 문서)
- MG_RatSteal_Tutorial  튜토리얼 씬 (32_Tutorial)
타이틀/일시정지/결과는 별도 씬이 아니라 UI 오버레이(41_UI_Menus).
```

## 구성 요소 (원작 MainScene 근거)

```text
배경        Test_back_02.png (order -200000)
밭          farm_A / farm_B / farm_C (랭크별 작물 스폰 영역, 20_Spawn)
제출존      SubMissionArea x2 — 맵 좌우 끝(원작 x = ±3580)
은신처      Bush (원작 예시 (-100,0)) — 다수 배치(15_Stealth)
둥지/홈     Home(RECT) — 베이비/제출 관련(13_Baby, 확인 필요)
시작 위치   플레이어 스폰 지점, 농부 초기 위치
```

## 동선 설계 의도

```text
- 밭(작물) ↔ 제출존이 떨어져 있어 "운반" 자체가 플레이의 핵심
- 제출존이 좌우 양 끝 → 가까운 쪽으로 나르는 동선 선택
- 농부 순찰 반경(350) / 추적 반경(650)이 밭과 동선에 겹치게 배치해 긴장 유발
- 부쉬를 동선 중간에 배치해 도주 분기 제공
```

## 좌표 메모 (원작 픽셀)

```text
제출존     x = +3580, x = -3580 (좌우 끝)
부쉬 예시  (-100, 0)
플레이어   스케일 0.35
```

UE 이식 시 픽셀→월드 단위 환산은 고정 계수로 못 박지 않고 플레이로 조정한다(원작 엔진 단위가 비표준일 수 있음, 50_Balance). 23_System_Camera 가시 범위와 함께 본다.

## UE 매핑 (요약)

```text
맵           → Content/LostSignal/MiniGame/RatSteal/Maps/MG_RatSteal
밭/제출존    → 마커 액터 또는 트리거 볼륨(USphere/UBox)
배경         → Paper2D 배경 스프라이트(최하단 sort)
경계         → 카메라 클램프용 경계(23_Camera)
```

## 미해결 / 확인 필요

```text
- 맵 실제 크기/비율, 밭 3개 배치 좌표
- 제출존 2개 유지 vs 단일 둥지 제출
- 부쉬 개수/배치(15_Stealth, 도주 동선)
- Home(RECT)의 정확한 역할
```
