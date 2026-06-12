# RatSteal Assets Manifest (원작 에셋 ↔ UE 임포트 매핑)

> 범위: 에셋 인벤토리. 원작 팀(=본 프로젝트 팀) 소유 에셋이라 사용 가능. UE 임포트 경로는 `Content/LostSignal/MiniGame/RatSteal/` 하위.

## 목적

원작 D2DGame 에셋을 식별하고 UE 임포트 형태로 매핑한다. (스프라이트/플립북/사운드/데이터)

## 임포트 폴더 구조 (예정)

```text
Content/LostSignal/MiniGame/RatSteal/
  Maps/        MG_RatSteal
  Art/
    Sprites/   캐릭터/작물/배경 스프라이트
    Flipbooks/ 애니메이션(플립북)
  Audio/       BGM / SFX
  Data/        현재 미사용 (수치는 C++ 기본값/에디터 설정)
  UI/          WBP_RatSteal*
```

## 캐릭터 / 오브젝트 (원작 → UE)

```text
농부          farmer_final.json (idle/angryidle/walk/angrywalk/attack)
              → UPaperFlipbook 5종
플레이어(쥐)  Idle/Walk/Steal(/Hit) 애니메이션 → UPaperFlipbook
아기쥐(Baby)  (확인 필요)
작물          가지/감자/호박 × 크기 4단계(Born/S/M/L) 스프라이트 + 성장 이펙트
배경          Test_back_02.png → 배경 스프라이트
부쉬          Bush 스프라이트
```

## UI / 아이콘

```text
하트          Icon_Heart.png → HUD 하트 3
프로필 프레임  Paper_Frame.png → 플레이어/베이비 프로필
슬롯          인벤토리 슬롯(160x160) 스프라이트
포만 게이지   Slide_Bar 스프라이트
타이틀 버튼   Start/Setting/Quit 버튼 스프라이트
```

## 사운드 (원작 SoundManager → UE)

```text
원작은 FMOD 사용 → UE에서는 원본 음원 파일을 USoundWave로 임포트.
BGM / 발걸음 / 훔치기 / 제출 / 농부 공격 / 패배·승리 등 (목록화 필요)
```

## 데이터 (원작 CSV/JSON → C++ 기본값/에디터 설정)

```text
성장속도/스폰 → ALSRatSpawnManager UPROPERTY 기본값/에디터 설정
작물 점수      → LSRat 점수 헬퍼
규칙/시간      → ALSRatGameMode UPROPERTY 기본값/에디터 설정
```

## 작업 메모

```text
- 원작 스프라이트 시트 → UE 텍스처 임포트 → UPaperSprite 추출 → 프레임 묶어 Flipbook.
- farmer_final.json 등 원작 애니 클립의 프레임 구간 정보를 Flipbook 프레임/FPS로 변환.
- 픽셀 단위 PPU(Pixels Per Unit) 기준을 정해 스케일 일관화.
```

## 미해결 / 확인 필요

```text
- 원작 에셋 원본 위치(리포에 미포함된 바이너리 에셋 경로)
- 아기쥐/이펙트 스프라이트 존재 여부
- 사운드 전체 목록
- PPU/스케일 기준
```
