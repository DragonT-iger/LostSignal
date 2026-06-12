# RatSteal UI — HUD

> 범위: **1차**. UE UMG, 위젯은 C++ `UUserWidget` 상속 → `WBP_*`, 텍스트는 `FText`/`LOCTEXT`, 바인딩은 강제 `BindWidget`(본편 규칙 준수).

## 목적

인게임 HUD 요소를 정의한다.

## 구성 (원작 MainScene 근거)

```text
점수        우상단 — 총점(21_Score)
타이머      남은 시간(22_Timer)
포만 게이지 Slide_Bar — 베이비 포만(14_Hunger)
하트        Icon_Heart x3 — 플레이어 HP(10_Player)
프로필      플레이어/베이비 프로필 프레임(Paper_Frame)
인벤토리    슬롯 3칸(160x160) — 현재 적재(16_Inventory, 2차)
안내        제출/훔치기 등 상황 안내(선택)
```

## 1차 / 2차

```text
1차: 점수, 타이머, 하트(HP)
2차: 포만 게이지, 인벤토리 슬롯, 프로필 프레임
```

## UE 매핑 (요약)

```text
HUD            → WBP_RatStealHUD (UUserWidget 상속 ULSRatHUDWidget)
점수/타이머    → TextBlock + BindWidget, FText 포맷
하트           → 이미지 3개 토글 또는 HorizontalBox 동적
포만 게이지    → ProgressBar (fullness/1000)
인벤토리 슬롯  → 슬롯 위젯 3개(2차)
```

## 미해결 / 확인 필요

```text
- 미할당 참조 시 UE_LOG(LogLS, Warning) 누락 점검(본편 UI 규칙)
- 안내/튜토리얼 문구 범위
- 원작 UI 레이아웃 재현 vs LostSignal 톤에 맞춘 재디자인
```

## 현재 UE 구현 메모

- 폴백 HUD 인벤토리 슬롯은 고정 작물 아이콘을 항상 표시한다.
- 빈 슬롯도 작물 아이콘을 선명하게 보여주고 카운트 0을 표시한다.
- 작물이 들어오면 해당 고정 슬롯 아이콘을 선명하게 표시하고 카운트를 갱신한다.
