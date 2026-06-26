# AGENTS.md — LostSignal (UE 5.7)

## 프로젝트 핵심 정보

- **장르:** 익스트렉션 액션 (Void Diver 계열) — 근접 전투, Top Down 쿼터뷰, 카툰 렌더링
- **엔진:** UE 5.7 / C++ 전용 / PC / Deferred / 3D
- **팀:** 프로그래머 2 / 아트 2 / 기획 3
- **버전관리:** git-svn (`git svn rebase` / `git svn dcommit`)
- **로컬라이징:** 한국어(기본) / 영어
- **서버:** 싱글 먼저 → 3인 MO 데디케이티드
- **클래스 접두사:** `LS` (예: `ALSCharacter`, `ULSCombatComponent`)

---

## 작업 원칙

- **선 설계, 후 구현:** 코드를 바로 작성하지 않는다. 비자명한 작업은 구현 전에 설계 옵션·트레이드오프·세부 결정을 먼저 논의하고 승인을 받는다. (단순·자명한 수정은 바로 진행)
- **모호하면 범위 확인:** 요청이 모호하면 코드를 짜기 전에 작업 범위를 먼저 확인한다.
- **연구 우선:** 새로 짜기 전에 UE API나 기존 C++ 코드에 이미 있는지 먼저 찾는다. 있으면 재사용한다.
- **Git:** 별도 작업 브랜치를 만들지 않고 `master`에 직접 작업한다. (git 명령은 사용자가 직접 실행)
- **언어:** 응답·주석·커밋 메시지는 한국어로 작성한다.

---

## 역할 분리

| 역할 | 허용 | 금지 |
|------|------|------|
| 프로그래머 | C++ 전용 | BP 로직 |
| 아트 | WBP_* 레이아웃, UMG 타임라인 애니메이션 | 게임 로직 |
| 기획자 | DataTable 수치 | 새 노드 추가 |

---

## 코드 규칙

- **Category:** 모든 UPROPERTY의 Category는 반드시 `"LS/"` 하위로. 예: `Category="LS/Combat"`, `Category="LS/Stats"`. 루트 `"LS/"` 그대로 써도 됨.
- **입력:** Enhanced Input System 전용 (Legacy 금지). `IA_Move/Attack/Dodge/Interact/Skill1~3`, `IMC_Default`
- **네트워크:** 싱글에서도 `if (!HasAuthority()) return;` 습관화 (멀티 전환 대비)
- **UI:** `FText`/`LOCTEXT` 필수 (`FString` 금지). 모든 위젯 C++ 상속 `UUserWidget` → `WBP_*`. UMG 위젯 바인딩은 항상 강제 `BindWidget` 사용 (`BindWidgetOptional` 금지).
- **UI 로그:** 위젯 클래스, 필수 참조, 설정값이 미할당이면 `UE_LOG(LogLS, Warning, ...)`로 반드시 남긴다.
- **DataTable:** 접두사 `DT_`, Row 구조체 `FLS~`. 수치 하드코딩 금지 — 기획자가 DataTable 편집
- **로그:** `UE_LOG(LogLS, ...)` 카테고리 통일. `GEngine->AddOnScreenDebugMessage` 커밋 금지
- **에셋 참조:** `ConstructorHelpers`로 WBP/에셋 경로 하드코딩 지양. `UPROPERTY(EditDefaultsOnly)`/`TSubclassOf`로 열고 BP에서 매핑
- **초기화:** 초기화 로직은 C++ 생성자에서 처리. BP(블루프린트)에서는 메시·이펙트·사운드 등 에셋 경로 매핑만 수행
- **GAS:** 체력/스태미나/스킬/상태이상은 GAS로 처리. 구조·태그·컴포넌트 매핑은 [SkillSystemStructure.md](Docs/Systems/SkillSystemStructure.md) 참고. 수치 변경은 반드시 GameplayEffect로.

---

## 파일 분리

- **함수:** 50줄 이하로 유지한다. 초과하면 분리한다.
- **헤더(.h):** 250줄을 초과하면 분리를 검토한다.
- **구현(.cpp):** 400줄을 초과하면 분리를 검토한다.

---

## 참고 문서

이 섹션이 `Docs/`의 인덱스다. MD 파일을 추가/제거할 때 여기도 같이 갱신한다.

### 문서별 담당 (각 문서가 그 주제의 단일 출처)

| 문서 | 단일 출처로 담당하는 내용 |
|------|---------------------------|
| [Docs/Systems/ItemSaveNetworkStructure.md](Docs/Systems/ItemSaveNetworkStructure.md) | 아이템 저장·슬롯·네트워크 구조 |
| [Docs/Systems/InventoryLogic.md](Docs/Systems/InventoryLogic.md) | 인벤토리 UI·슬롯 조작 로직 |
| [Docs/Systems/LootDropDataTable.md](Docs/Systems/LootDropDataTable.md) | 루트 드랍·데이터 테이블 파이프라인 |
| [Docs/Systems/RaidLevelFlow.md](Docs/Systems/RaidLevelFlow.md) | 레이드 레벨 플로우·결과 저장 ACK |
| [Docs/Systems/CombatImplementationFlow.md](Docs/Systems/CombatImplementationFlow.md) | 전투 입력·데이터 조회·서버 판정·GAS 적용 흐름 |
| [Docs/Systems/CombatProtocolUI.md](Docs/Systems/CombatProtocolUI.md) | 전투 프로토콜 단계별 UI 해금·표시 구조 |
| [Docs/Systems/SkillSystemStructure.md](Docs/Systems/SkillSystemStructure.md) | 스킬·GAS·DataAsset·쿨타임·강화 구조 |
| [Docs/Systems/ChipSystem.md](Docs/Systems/ChipSystem.md) | 칩 데이터·장착·신호 게이지·프로토콜·칩 전투 스탯 GAS 연동 |
| [Docs/Systems/MonsterAIControlStructure.md](Docs/Systems/MonsterAIControlStructure.md) | 몬스터 AI 제어·StateTree 전이 구조 |
| [Docs/Systems/MinimapSystem.md](Docs/Systems/MinimapSystem.md) | 미니맵 표시 대상·지형 도형·탐색 프로토콜 연동 구조 |
| [Docs/Systems/UILayerStructure.md](Docs/Systems/UILayerStructure.md) | UI 레이어 Z-order·공유 풀스크린 블러 표시 규칙 |

### 트러블슈팅 (버그 원인·수정 기록)

증상이 재현되면 먼저 읽는 디버깅 기록. 같은 함정에 다시 빠지지 않기 위한 문서다.

| 문서 | 담당 내용 |
|------|-----------|
| [Docs/Troubleshooting/UIDragDropPackagedBuild.md](Docs/Troubleshooting/UIDragDropPackagedBuild.md) | 패키지 빌드에서 UMG 드래그앤드롭 입력이 죽는 문제(DefaultDragVisual=this) 원인·진단·수정 |

### 미니게임 (RatSteal — 몰래몰래팜)

원작 D2DGame을 LostSignal 내장 미니게임으로 이식하는 별도 문서군. 폴더 전체가 기획의 단일 출처이며, 진입점은 Overview다.

| 문서 | 단일 출처로 담당하는 내용 |
|------|---------------------------|
| [Docs/MiniGame/RatSteal/00_Overview.md](Docs/MiniGame/RatSteal/00_Overview.md) | RatSteal 컨셉·범위·**폴더 내 문서 인덱스**(나머지 문서는 여기서 색인) |

---

## 문서 워크플로우

- 작업 전 관련 `Docs/*.md`가 있으면 먼저 읽는다.
- 코드를 변경한 뒤 영향받는 문서를 같은 커밋에서 갱신한다 (문서-코드 분리 커밋 금지). 코드가 사실, 문서는 그 기록이다.
- 기존 MD가 다루지 않는 새 시스템이면, 새 MD를 만들기 전에 어떤 파일명으로 어떤 범위를 담을지 먼저 합의한다.
- 새 MD를 만들면 위 "참고 문서" 표에 한 줄 등록한다. 별도 인덱스 파일은 만들지 않는다.
- MD가 코드와 어긋난 것을 발견하면 즉시 MD를 사실에 맞게 고친다.

### 단일 출처 (Single Source of Truth)

- 같은 사실을 여러 문서에 적지 않는다. **한 문서가 그 주제를 "소유"하고, 나머지는 값/내용을 복붙하지 않고 링크로 참조한다.**
- 새 내용·변경은 그 주제의 소유처에만 적는다. 다른 문서에서는 수치·정의를 단정하지 말고 소유처를 가리키거나 의미만 정성적으로 쓴다.
- 소유 맵:
  - 게임 수치(스탯·경제·쿨타임 등) → DataTable(`DT_`, Row 구조체 `FLS~`). 문서엔 수치 복붙 금지, 의미만.
  - 클래스/필드 구조 → C++ 헤더(`ALSCharacter.h` 등). 문서엔 코드 복붙 금지, 링크만.
  - 프로젝트 규칙(작업 원칙·코드 규칙·역할 분리·금지 사항) → 이 AGENTS.md.
  - 미니게임 RatSteal 기획 → `Docs/MiniGame/RatSteal/`. 수치는 C++ `UPROPERTY` 기본값과 에디터 조정값으로 관리하고, 문서엔 의도/근거를 남긴다. 클래스 구조는 C++ 헤더가 단일 출처다.

---

## Diff 최소화 규칙

- 요청 범위 밖의 리팩터링·정렬·포맷·이름 변경 금지. 필요한 줄/함수만 수정하고 기존 스타일은 유지한다.
- 기존 로직을 대체하기 전에 현재 흐름을 먼저 설명한다. 동작이 바뀌면 변경 전/후를 짧게 기록한다. (큰 구조 변경은 "작업 원칙" 참고)
- `git diff` 확인은 대규모·다중 파일 변경이나 요청이 있을 때만 한다.

---

## 금지 사항

> 코드 규칙·역할 분리에서 이미 다룬 항목은 빼고, 그 외 금지만 모음.

- Content 루트에 에셋 직접 배치 (반드시 `Content/LostSignal/` 하위)
- 안 쓰는 코드 커밋 (주석 처리된 코드, 미사용 `#include`, 빈 함수 등 — 발견 시 삭제)