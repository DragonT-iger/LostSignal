# RatSteal Overview (몰래몰래팜)

## 목적

이 문서는 미니게임 `RatSteal`(원작 "몰래몰래팜")의 컨셉과 타깃 경험을 정의하고, `Docs/MiniGame/RatSteal/` 폴더의 **문서 인덱스**를 제공한다.

`RatSteal`은 D2DGame(자체 Direct2D 엔진)으로 만든 원작을 LostSignal(UE 5.7) 안의 **독립 미니게임**으로 다시 만드는 프로젝트다. 원작 게임플레이/에셋만 가져오고 엔진 레이어는 UE로 재구현한다.

## 한 줄 핵심

> 쥐가 농부(Farmer)의 눈을 피해 밭의 농작물을 훔쳐 제출존으로 날라 **굶주린 아기쥐(Baby)를 먹이며**, 제한 시간 3분 동안 살아남아 **최대 점수**를 모으는 Top-down 스텔스 스코어 어택.

## 타깃 경험

```text
- 들킬까 말까 하는 긴장감 (Farmer 감지 반경 / 부쉬 은신)
- 짐을 많이 들수록 느려지는 위험-보상 (무게 = 감속)
- 포만 게이지가 줄어드는 시간 압박 (계속 훔쳐서 날라야 함)
- 한 판 3분, 점수 갱신을 노리는 스코어 어택 아케이드성
```

## 장르 / 형식

```text
장르   : Top-down 2D 스텔스 액션 / 아케이드
시점   : 직교(Orthographic) 쿼터뷰
플랫폼 : PC (LostSignal 내장 미니게임)
진입   : 본편 월드의 상호작용 오브젝트(오락기/단말기)에서 전용 레벨로 전환
범위   : 1차 = 핵심 루프 / 2차 = 베이비·허기·은신·인벤토리 확장
```

## 구현 방식 (요약)

- LostSignal `LostSignal` 모듈 안에 **폴더로 격리**: `Source/LostSignal/MiniGame/RatSteal/`, `Content/LostSignal/MiniGame/RatSteal/`.
- 렌더링은 **Paper2D**(Flipbook/Sprite), 충돌은 UE `UBox/USphereComponent` Overlap, 입력은 **Enhanced Input** 전용 `IMC_RatSteal`.
- 본편 GAS/ALSCharacter와 분리. 전용 `ALSRatGameMode`/`PlayerController`/`Pawn`.
- 상세 매핑은 [99_TechMapping.md](99_TechMapping.md) (구현 착수 시 작성).

## 문서 인덱스

```text
0X 게임 정의
  00_Overview        이 문서 (컨셉/인덱스)
  01_CoreLoop        코어 루프, 승/패 조건
  02_Progression     판 진행/제한시간, 난이도
  03_Controls        입력 매핑, 조작 규칙

1X 엔티티 / 메카닉
  10_Entity_Player   쥐 플레이어 (이동/훔치기/체력/포만)   [1차]
  11_Entity_Farmer   농부 AI (순찰/추적/공격, 감지 반경)   [1차]
  12_Entity_Crop     농작물 (종류/성장/가치)               [1차]
  13_Entity_Baby     아기쥐 (먹이기/굶주림 패배)           [1차]
  14_Mechanic_Hunger 포만 게이지                            [1차]
  15_Mechanic_Stealth 은신 (부쉬 hide/visible)              [1차]
  16_Mechanic_Inventory 3슬롯 인벤토리 (적재/감속)          [1차]

2X 시스템
  20_System_Spawn    농작물 스폰 규칙
  21_System_Score    점수/제출 정산
  22_System_Timer    타이머/라운드 종료
  23_System_Camera   직교 추종 카메라 / Y-Sort

3X 레벨 / 흐름
  30_Level_Layout    맵 레이아웃, 밭/제출존/은신처 배치
  31_Flow_EntryReturn 본편 진입 → 미니게임 → 복귀
  32_Tutorial        튜토리얼 씬 구성/학습 흐름

4X UI / UX
  40_UI_HUD          HUD (점수/타이머/포만/하트)
  41_UI_Menus        타이틀/일시정지/결과(엔딩)

5X 콘텐츠 / 에셋
  50_Content_Balance 밸런스 수치표 (C++ 기본값/에디터 조정 근거)
  51_Assets_Manifest 원작 에셋 ↔ UE 임포트 매핑

9X 구현 (나중)
  99_TechMapping     D2DEngine→UE 매핑, 액터/모듈 구조
```

## 단일 출처 원칙

- 기획 단계에서는 이 폴더가 게임 디자인의 단일 출처다.
- 구현 수치는 C++ `UPROPERTY` 기본값과 에디터 조정값을 기준으로 관리하고, 문서엔 "왜/의도/밸런스 근거"를 남긴다. 클래스 구조는 헤더가 단일 출처다.
- 같은 사실을 두 문서에 적지 않는다. 각 문서가 위 인덱스의 한 주제를 소유한다.

## 확정된 핵심 사양

```text
- 승리 조건 없음 = 3분 생존하며 최대 점수(스코어 어택)
- 패배: 포만 0(아기 굶주림, 스토리상 정식 결말) / HP 0
- 타이틀 화면 표시함
- 점수 등급(별점)은 신규 추가
- 1차 범위에 포만·은신·인벤토리 포함(코어 루프 전체)
- 빌드 씬은 튜토리얼 + 메인 게임 씬 2개로 한정(타이틀/일시정지/결과는 UI 오버레이)
- 게임패드 미지원(키보드 X/C 등 원작 키 유지)
- 무적은 원작 ivc_T=15(프레임) 기준, 단위/프레임 환산은 플레이로 조정
- 모든 수치는 원작 D2DGame을 그대로 따른다
- 원작 Alert 상태는 미구현(빈 함수)이라 채택하지 않음
- Rage 난이도 페이즈 없음(원작 enum값만 있고 미구현)
```
