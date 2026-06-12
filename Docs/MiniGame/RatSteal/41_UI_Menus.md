# RatSteal UI — Menus (타이틀 / 일시정지 / 결과)

> 범위: **1차(결과/일시정지) ~ 선택(타이틀)**. 종료 조건은 [01_CoreLoop.md](01_CoreLoop.md), 진입/복귀는 [31_Flow_EntryReturn.md](31_Flow_EntryReturn.md)가 소유.

## 목적

미니게임의 화면 전환 UI(타이틀/일시정지/결과)를 정의한다.

## 화면 (타이틀 채택)

```text
타이틀       시작/설정/나가기 (원작 TestTitleScene, StartButton/SettingButton/QuitButton)
일시정지     계속/재시작/나가기
결과(엔딩)   종료 사유별 결과 + 점수/등급 요약 → 본편 복귀
```

타이틀 화면은 **표시한다**(확정). 단 별도 씬이 아니라 **UI 오버레이**다(빌드 씬은 튜토리얼+메인 2개, 30_Level). 흐름: 캐비닛 진입 → 타이틀 → (튜토리얼) → Ready → 게임 → 결과 → 복귀.

## 결과 화면 (종료 사유별)

```text
TimeUp(생존)  3분 생존 완료 — 총점 + 점수 등급(★) 표기 (스코어 어택)
BabyStarved   아기쥐 굶주림 결말 — 스토리상 정식 엔딩 연출 + 도달 점수
PlayerDead    플레이어 사망 — 도달 점수
```

표기 항목:

```text
- 총점, 종류별 수집 수(가지/감자/호박)
- 점수 등급(별점) — 신규 추가(원작 없음, 02_Progression 컷 수치)
- 본편 복귀 버튼(31_Flow)
```

## 점수 등급 (신규)

```text
생존 종료 시 총점 구간으로 ★/★★/★★★ 표기.
패배 시 등급 표기 정책은 02_Progression에서 확정.
```

## UE 매핑 (요약)

```text
메뉴 위젯   → WBP_RatStealTitle / WBP_RatStealPause / WBP_RatStealResult
버튼        → 원작 StartButton/SettingButton/QuitButton 대응
복귀        → ULSRatStealSubsystem 경유 OpenLevel(31_Flow)
```

## 미해결 / 확인 필요

```text
- 설정(Setting) 화면 이식 범위(원작 AcceptSetting/QuitSetting)
- 재시작이 미니게임 레벨 리로드 vs 상태 리셋
- 패배 시 등급 표기 정책(02_Progression)
```
