# RatSteal Tech Mapping (D2DEngine → UE)

> 범위: **구현 착수 시 작성/확정.** 현재는 골격. 본편 Systems 문서 스타일을 따른다. 기획 수치는 [50_Content_Balance.md](50_Content_Balance.md)를 근거로 C++ 기본값과 에디터 설정에서 관리한다.

## 목적

원작 D2DEngine(자체 Direct2D 엔진)의 게임플레이를 UE 5.7 액터 구조로 재구현하는 매핑과 모듈/폴더 구조를 정의한다.

## 엔진 매핑 (재구현 대조)

```text
GameObject              → AActor
Component/MonoBehaviour → UActorComponent / Actor 서브클래스
Transform               → Actor 내장 트랜스폼
SpriteRenderer+Animator → UPaperFlipbookComponent + Flipbook
BoxCollider/CircleCollider → UBoxComponent / USphereComponent (Overlap)
Scene/SceneManager      → Level + OpenLevel
Camera (Cinemachine)    → Orthographic UCameraComponent + 추종/클램프
InputManager            → Enhanced Input (IMC_RatSteal)
SoundManager (FMOD)     → USoundWave/USoundCue + UAudioComponent
ResourceManager / PNG   → 텍스처 임포트 → UPaperSprite
CSV/JsonParser          → C++ UPROPERTY 기본값 / 에디터 설정
Y-Sort                  → TranslucencySortPriority = -WorldY
GameManager 싱글톤      → ALSRatGameMode + ULSRatStealSubsystem
```

## 모듈 / 폴더

```text
Source/LostSignal/MiniGame/RatSteal/   C++ (LS 패밀리, Rat 그룹)
Content/LostSignal/MiniGame/RatSteal/  맵/Paper2D/DT/WBP (51_Assets)
```

## 클래스 골격 (구현 완료)

```text
LSRatTypes.h               공용 enum/구조체 + 점수·카운트 헬퍼 (LSRat 네임스페이스)
ALSRatGameMode             규칙/점수/타이머(180s)/Ready·Pause/종료·등급 (구 GameManager+Timer)
ALSRatTutorialGameMode     타이머·포만 감소 끈 튜토리얼용 (32_Tutorial)
ALSRatPlayer               쥐 Pawn — 이동/포만/HP/무적/은신/훔치기/제출 + 직교 추종 카메라
ALSRatPlayerController     Enhanced Input. IMC/IA 에셋 미할당이면 런타임에 원작 키 자동 구성
ULSRatInventoryComponent   3슬롯/적재/슬롯 전환/버리기/SpeedMultiplier (구 Inventory+Slot)
ALSRatFarmer               농부 AI enum 상태머신 + 공격 지시자/판정 (구 Farmer+AttackPattern+FarmerZone, 거리 검사로 대체)
ALSRatAttackIndicator      공격 지시자 표시 (redCircle 0.5 불투명)
ALSRatCrop                 작물 성장 Born→S→M→L, S부터 콜라이더 (구 Crop)
ALSRatSpawnManager         A/B/C 도넛 밭 스폰 (구 SpawnManager — maxRate 20/15/10, spawnTime 3/4/5)
ALSRatBush                 은신 부쉬 200x200 (구 Bush)
ALSRatSubmissionArea       제출존 521x4320 (구 SubMissionArea)
ULSRatStealSubsystem       본편↔미니게임 진입/복귀/결과 (31_Flow)
ALSRatStealCabinet         본편 진입 오브젝트 (ILSInteractable 구현)
ULSRatHUDWidget/ResultWidget/PauseWidget   WBP_* 부모 (BindWidget 강제)
```

## 좌표/평면 규약 (확정)

```text
- X-Z 평면 사용 (X=가로, Z=세로 화면축, Y=깊이 0 고정). Paper2D 기본 평면.
- 카메라: 플레이어 SpringArm(-Y, 랙 추종) + 직교 OrthoWidth 2400 (플레이로 조정)
- Y-Sort: ULSRatYSortComponent → TranslucencySortPriority = -WorldZ (원작 -WorldY 대응)
- 픽셀 1 = 1uu 가정. 환산은 플레이로 조정(50_Balance)
```

## 원작 코드에서 확정한 추가 수치 (50_Balance 미확정분 해소)

```text
밭 RECT(절반)   A ±(1070,720) / B ±(2130,1330) / C ±(3210,2110), Home ±50 (도넛 스폰)
maxRate         A 20 / B 15 / C 10
spawnTime       A 3s / B 4s / C 5s
작물 확률       전 랭크 공통 가지34/감자33/호박33 (RandomCrop(34,33,33))
버리기 쿨다운   throwTime 0.2s
작물 스케일     0.2 / 농부 스케일 0.35
스폰 회피       기존 작물 200, 내부 제외영역 여유 ±50
```

## 본편과의 분리 원칙

```text
- GAS / StateTree / 세이브 / 인벤토리 등 본편 시스템 미사용.
- 전용 GameMode/PlayerController/IMC로 입력·카메라 격리.
- 결과 전달은 ULSRatStealSubsystem 경유로만.
- LS 코드 규칙(접두사 LS, Category "LS/", FText, BindWidget, LogLS)은 준수.
```

## 콘텐츠 자동 생성 (완료)

`tools/import_ratsteal_assets.py` + `tools/build_ratsteal_content.py` (에디터 파이썬, 헤드리스 실행)로 생성:

```text
Imported/   원본 PNG/MP3/TTF 151개 임포트
Sprites/    작물 단계(plant + 종류별 S/M/L), bush, redCircle, 배경 2종
            + 시트 프레임 스프라이트 (Mole 39 / Farmer 21 / Sparkle 2)
Flipbooks/  aseprite frameTags 기반 — Mole 7클립, Farmer 5클립(idle/angryidle/walk/angrywalk/attack), Sparkle
Blueprints/ BP_RatPlayer / BP_RatFarmer / BP_RatCrop / BP_RatSpawnManager /
            BP_RatBush / BP_RatAttackIndicator / BP_RatGameMode / BP_RatTutorialGameMode
            (CDO에 플립북/스프라이트/CropVisuals/IndicatorClass/DefaultPawn 자동 할당)
Maps/       MG_RatSteal (제출존 ±3580 ×2, 부쉬 5, 농부 2, SpawnManager, 배경)
            MG_RatSteal_Tutorial (축소판, BP_RatTutorialGameMode)
```

UI는 WBP 없이도 동작: HUD/결과/일시정지 위젯이 C++에서 폴백 레이아웃을 스스로 구성
(WBP를 만들어 같은 이름(ScoreText 등)으로 바인딩하면 폴백 대신 WBP 레이아웃 사용).
입력도 IMC/IA 에셋 미할당 시 런타임에서 원작 키를 자동 구성. 결과 화면 복귀는 Enter/Space.

## 남은 작업

```text
1. 에디터에서 MG_RatSteal 플레이 확인 — 카메라 줌(OrthoWidth 2400)/배경 스케일/픽셀 환산 조정
2. (아트 패스) WBP_RatStealHUD/Result/Pause 제작 → BP_RatGameMode에 할당
3. (선택) IMC_RatSteal/IA_* 에셋 제작
4. 본편 레벨에 BP_RatStealCabinet(또는 ALSRatStealCabinet) 배치 + MiniGameLevel=MG_RatSteal 할당
5. 사운드(SFX/BGM 임포트 완료) 연결, 타이틀 오버레이(41_UI_Menus)
```

빌드 통과 (LostSignalEditor Win64 Development). PythonScriptPlugin이 .uproject에 추가됨.
