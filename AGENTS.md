# AGENTS.md — LostSignal (UE 5.7)

## 프로젝트 핵심 정보

- **장르:** 익스트렉션 액션 (Void Diver 계열) — 근접 전투, Top Down 쿼터뷰, 카툰 렌더링
- **엔진:** UE 5.7 / C++ 전용 / PC / Deferred / 3D
- **팀:** 프로그래머 2 / 아트 2 / 기획 3
- **버전관리:** git-svn (`git svn rebase` / `git svn dcommit`)
- **로컬라이징:** 한국어(기본) / 영어
- **서버:** 싱글 먼저 → 3인 MO 데디케이티드
- **클래스 접두사:** `LS` (예: `ALSCharacter`, `ULSCombatComponent`)

> 프로그래머: 언리얼 첫 프로젝트, Unity/자체엔진 경력.
> AI: 언리얼 개념 첫 등장 시 Unity 비교로 설명.

---

## 역할 분리

| 역할 | 허용 | 금지 |
|------|------|------|
| 프로그래머 | C++ 전용 | BP 로직 |
| 아트 | WBP_* 레이아웃, UMG 타임라인 애니메이션 | 게임 로직 |
| 기획자 | DataTable 수치, GameplayEffect 에셋 수치 | 새 노드 추가 |

---

## 코드 규칙

- **네이밍:** `A`=Actor, `U`=UObject, `I`=Interface, `F`=Struct, `E`=Enum. UPROPERTY `Category` 필수.
- **입력:** Enhanced Input System 전용 (Legacy 금지). `IA_Move/Attack/Dodge/Interact/Skill1~3`, `IMC_Default`
- **네트워크:** 싱글에서도 `if (!HasAuthority()) return;` 습관화 (멀티 전환 대비)
- **UI:** `FText`/`LOCTEXT` 필수 (`FString` 금지). 모든 위젯 C++ 상속 `UUserWidget` → `WBP_*`
- **DataTable:** 접두사 `DT_`, Row 구조체 `FLS~`. 수치 하드코딩 금지 — 기획자가 DataTable 편집
- **로그:** `UE_LOG(LogLS, ...)` 카테고리 통일. `GEngine->AddOnScreenDebugMessage` 커밋 금지
- **초기화:** 초기화 로직은 C++ 생성자에서 처리. BP(블루프린트)에서는 메시·이펙트·사운드 등 에셋 경로 매핑만 수행

---

## GAS 시스템 (핵심)

체력/스태미나/스킬/상태이상/멀티복제 전부 GAS로 처리.

| 구성요소 | LostSignal 사용처 |
|----------|-------------------|
| `AbilitySystemComponent` | 모든 캐릭터에 부착 |
| `ULSAttributeSet` | Health, MaxHealth, Stamina |
| `GameplayAbility` | 콤보, 구르기, 스킬 |
| `GameplayEffect` | 데미지, 힐, 쿨타임, 버프 |
| `GameplayTag` | 상태 플래그 (아래 참고) |
| `GameplayCue` | 비주얼/사운드 전용 (로직 금지) |

**태그:** `LS.State.{Dash/Invincible/Dead}` · `LS.Combat.{ComboWindow/Attacking}` · `LS.Debuff.Stunned`
**규칙:** 수치 변경은 반드시 GameplayEffect 통해서.

---

## Core 클래스

| 클래스 | 역할 |
|--------|------|
| `ALSGameMode` | 게임 규칙, 스폰, 탈출 판정 (서버 전용) |
| `ALSGameState` | 게임 진행 상태 (복제) |
| `ULSGameInstance` | 로비↔인게임 유지 데이터 |
| `ALSPlayerState` | 개별 플레이어 정보 (복제) |
| `ALSPlayerController` | 입력→캐릭터, UI 관리 |

---

## 파일 구조

**Source:**
```
Source/LostSignal/
├── Core/        GameMode, GameState, GameInstance, PlayerState, PlayerController
├── Characters/  ALSCharacter(베이스), Player, Enemy
├── GAS/         AttributeSet, Abilities, Effects, Tags
├── Combat/      CombatComponent, HitBox
├── Weapons/     WeaponBase
├── AI/          AIController, BT Tasks/Decorators/Services
├── Camera/      쿼터뷰, CameraShake
├── Input/       InputComponent, InputAction 바인딩
├── UI/          HUD, Inventory, Shop, SkillTree, Lobby, Result
├── Items/       ItemPickup, InventoryComponent
└── Utils/
```

**Content:**
```
Content/LostSignal/
├── Characters/  Mesh, AnimBP, Montage
├── Weapons/     Mesh, 이펙트
├── VFX/         Niagara, GameplayCue
├── UI/          WBP_*, 아이콘, 폰트
├── Maps/        레벨
├── Data/        DataTables/, GAS/(GE·GA 에셋), Input/(IA·IMC)
├── Audio/       SFX/, Music/
├── Materials/
└── Textures/
```

> 에셋은 반드시 `Content/LostSignal/` 하위에. 루트 Content에 직접 넣지 말 것.

---

## 금지 사항

- GAS 수치 직접 변경 (GameplayEffect 우회 금지)
- BP 이벤트그래프에 게임 로직
- UI 텍스트에 FString
- Tick에서 비싼 연산
- `HasAuthority()` 없는 상태 변경
- Legacy Input System 사용
- Content 루트에 에셋 직접 배치
- DataTable 값 코드에서 하드코딩 금지
- Unreal MCP 하드코딩 금지 발견시 삭제
- 안 쓰는 코드 커밋 (주석 처리된 코드, 미사용 #include, 빈 함수 등 — 발견 시 삭제)

---

## 토큰 절약

- 코드 예시는 관련 함수/변수만 — 전체 클래스 금지
- 설명은 Unity 비교 한 줄로 가능하면 그렇게
- 에러 해결은 에러 줄 ±10줄만 참고
- 긴 헤더 파일 통째로 붙여넣기 금지
