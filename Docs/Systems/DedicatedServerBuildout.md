# 데디케이티드 서버 구축 계획

## 이 문서의 범위

3인 MO 데디케이티드 서버를 실제로 띄우기까지 필요한 **엔진·빌드·운영 측 작업**의 단일 출처다. 빌드 타깃, 패키징, 서버 시작 맵, 플레이어 접속·이탈, 매치 할당, 프로세스 수명, 시크릿 주입, 헤드리스 검증을 다룬다.

인접 문서와의 경계:

| 주제 | 단일 출처 |
|------|-----------|
| 레이드 입장·결과 ACK 단계별 흐름 | [RaidLevelFlow.md](RaidLevelFlow.md) |
| 아이템 저장 구조·신뢰 경계 | [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md) |
| 저장 백엔드를 서버 권위로 옮기는 계획 (**학습용**) | [SaveBackendMigration.md](SaveBackendMigration.md) |
| 이 문서 | 위를 실제로 돌릴 서버 프로세스를 만드는 일 |

> **학습용이 아니다.** [AGENTS.md](../../AGENTS.md)의 프로젝트 방향이 "싱글 먼저 → 3인 MO 데디케이티드"이므로 이 문서는 실제 로드맵이다. 반면 [SaveBackendMigration.md](SaveBackendMigration.md)의 서버 권위 저장 전환은 학습 목적이며 출시 방침이 아니다(출시는 로컬 `SaveGame` + Steam Cloud).
>
> 둘은 **양립한다.** 레이드 중에는 데디서버가 권위를 갖고, 영속 저장은 각 클라이언트 로컬 파일에 남는 구조가 현재 설계다.

**현재 구현 상태: 미착수.** 서버 빌드 타깃이 없어 데디케이티드 빌드 자체가 불가능하다.

---

## 현재 상태 (실측)

### 있는 것

```text
GameMode 4종        LSTitleGameMode / LSLobbyGameMode / LSFarmingGameMode / LSResultGameMode
                    (+ 공통 베이스 LSGameModeBase)
서버 권위 레이드    ULSRaidInventoryComponent 기반 루팅·사용·드랍·사망·탈출 확정
입장/결과 ACK       제출 ACK, 결과 저장 ACK, 각각 타임아웃 처리까지 구현됨
데디 대비 분기      NM_DedicatedServer 분기 12곳
                    렌더/위젯/프리뷰 계열이 서버에서 스킵되도록 이미 처리됨
```

레이드 자체의 서버 권위 로직은 상당 부분 서 있다. 빠진 것은 **그 로직을 담을 서버 프로세스**다.

### 없는 것

```text
서버 빌드 타깃      Source/ 에 LostSignal.Target.cs (TargetType.Game) 와
                    LostSignalEditor.Target.cs 만 존재
                    → LostSignalServer.Target.cs 부재 = 데디 빌드 불가

플레이어 접속 처리  PostLogin / Logout / GetNumPlayers 사용처 0곳
                    누가 언제 들어오고 나갔는지 다루는 코드가 없다

SeamlessTravel      bUseSeamlessTravel 사용처 0곳
                    ServerTravel 시 연결이 끊겼다 다시 붙는다

세션 / 매칭         CreateSession / FindSession / JoinSession / OnlineSubsystem 0곳
                    Build.cs 의 OnlineSubsystem 은 주석 처리된 템플릿 기본값

서버 시작 맵        Config/DefaultEngine.ini 에 ServerDefaultMap 없음
                    GameDefaultMap 은 L_Title (서버가 타이틀을 띄울 이유가 없다)
```

### 주의가 필요한 기존 구조

**① 전역 세션 상태의 순서 기반 큐**

`ALSLobbyGameMode::StartRaid`가 `ServerTravel` 직전에 각 플레이어 데이터를 `ULSSessionSubsystem`에 순서대로 넣는다.

```text
SessionSub->EnqueuePendingRaidEntry(PlayerLoadout, PlayerSafeItems, PlayerEquipment);
// 주석: "ServerTravel 이후 각 PC가 자신의 데이터를 꺼낼 수 있도록 순서대로 큐에 저장"
```

`ULSSessionSubsystem`은 GameInstance 서브시스템이라 **데디서버에는 전 플레이어가 공유하는 인스턴스가 하나뿐이다.** 순서에 의존해 자기 데이터를 꺼내는 방식은 접속 순서·재접속·중도 이탈에 취약하다. [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md)에도 "이 보호가 없으면 ServerTravel 이후 모든 플레이어 인벤토리가 첫 번째 플레이어 데이터로 덮일 수 있다"고 기록된 알려진 취약점이다.

같은 함수에 `bHasLegacySessionLoadout` / `MirrorRaidSessionState`처럼 단일 플레이어 시절의 전역 미러링 경로도 남아 있다. 3인 환경에서 전역 미러는 정의상 누군가에겐 틀린 값이다.

→ **PlayerState 또는 안정적인 플레이어 키 기반으로 교체해야 한다.** 데디 전환의 선행 정리 항목.

**② 글로벌 기본 GameMode가 블루프린트**

```text
GlobalDefaultGameMode=/Game/TopDown/Blueprints/BP_TopDownGameMode
```

C++ 전용 프로젝트인데 템플릿 잔재인 BP GameMode가 전역 기본값이다. 서버가 맵별 GameMode 지정 없이 뜨면 이게 걸린다. 에셋 위치(`/Game/TopDown/`)도 `Content/LostSignal/` 규칙 밖이다.

---

## 작업 항목

### A. 빌드 / 패키징

```text
A1  LostSignalServer.Target.cs 추가 (TargetType.Server)
    ExtraModuleNames 는 Game 타깃과 동일하게 맞추되 서버 불필요 모듈 검토
A2  서버 타깃 컴파일 통과
    에디터 전용 / 렌더 전용 코드가 서버 타깃에서 링크되는지 확인
    LostSignalVisionShaders 등 렌더 모듈의 서버 빌드 가부 판단
A3  데디케이티드 서버 패키징 커맨드 확립 (기존 클라 패키징과 별도)
A4  서버 빌드 산출물에 클라 전용 에셋이 빠지는지 / 용량 확인
```

A2가 실질적 관문이다. `NM_DedicatedServer` 런타임 분기는 12곳 있지만, 그건 **실행 시 스킵**이지 **컴파일 시 제외**가 아니다. 서버 타깃에서 처음 빌드하면 렌더/UMG 의존이 드러난다.

### B. 서버 부팅 / 맵

```text
B1  ServerDefaultMap 지정 (레이드 맵 또는 대기용 빈 맵)
B2  GlobalDefaultGameMode 를 C++ 클래스로 교체 (BP_TopDownGameMode 제거)
B3  맵별 GameMode 매핑 확인 (GameModeMapPrefixes 또는 월드 세팅)
B4  서버 커맨드라인 인자 정리 (포트, 로그, 맵)
```

### C. 플레이어 접속 / 이탈

현재 완전히 비어 있는 영역이다.

```text
C1  PostLogin  - 플레이어 입장 시 PlayerState 초기화, 접속 인원 카운트
C2  Logout     - 중도 이탈 시 레이드 상태 처리 (진행 중이면 어떤 결과로 확정할지)
C3  재접속 정책 - 허용할지, 허용하면 세션 상태를 어떻게 복구할지
C4  최소/최대 인원 판정 - 3인 MO 기준 시작 조건
```

C2가 설계 결정을 요구한다. [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md)의 테스트 체크리스트에도 "강제 종료/연결 해제 시 어떤 결과를 저장할지 정책이 명확한지"가 미해결로 남아 있다.

### D. 여행 / 세션 상태

```text
D1  bUseSeamlessTravel 도입 여부 판단
    끄면 ServerTravel 마다 연결이 끊겨 재접속 처리가 필요하다
D2  EnqueuePendingRaidEntry 순서 기반 큐를 플레이어 키 기반으로 교체
D3  MirrorRaidSessionState / bHasLegacySessionLoadout 등 단일 플레이어 전역 경로 제거
D4  ULSSessionSubsystem 의 남은 전역 상태 정리 (데디에서는 인스턴스가 하나뿐)
```

D2·D3는 데디 전환의 **선행 조건**이다. 서버 타깃만 만들고 이 정리를 건너뛰면 3인 테스트에서 인벤토리가 섞인다.

### E. 매치 할당 / 접속

```text
E1  클라이언트가 서버 주소를 어떻게 얻는가 (직접 입력 / 세션 검색 / 매치메이커)
E2  ✅ 결정됨 — 로비 UI·경제는 클라 로컬, 데디서버는 레이드 매치 전용 (아래 참조)
E3  레이드 종료 후 클라이언트 복귀 경로 (서버 유지 / 해산 후 로컬 복귀)
E4  OnlineSubsystem(Steam) 도입 여부 — 세션 검색·NAT 통과에 필요할 수 있음
```

#### E2 결정 (출시 방침)

```text
로비 UI · 로비 경제   클라이언트 로컬        제작 · 상점 · 창고 · 장착을 클라가 확정
레이드 판정           매치 전용 데디서버      루팅 · 사용 · 드랍 · 사망 · 탈출을 서버가 확정
영구 저장             로컬 SaveGame + Steam Cloud
```

즉 **로비를 위해 서버 프로세스를 띄우지 않는다.** 데디서버는 레이드 매치 동안에만 존재한다.

이 구성은 [ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md)에 이미 기록된 현재 신뢰 경계와 정확히 일치한다 — 레이드 입장 시 제출 데이터는 신뢰하고, 레이드 중에는 서버 상태만 신뢰하며, 결과는 서버가 계산해 클라 로컬 세이브에 반영한다. 즉 **E2는 새 아키텍처를 도입하는 결정이 아니라 현재 구조를 유지하는 결정**이고, 이 문서가 추가하는 것은 그 레이드 권위를 담을 실제 서버 프로세스뿐이다.

로비가 메뉴 화면이라는 점도 이를 뒷받침한다. `ALSLobbyGameMode`는 `DefaultPawnClass = nullptr`로 폰 스폰을 막고 `BeginPlay`에서 곧바로 WBP를 띄운 뒤 UI 입력 모드로 전환한다. 캐릭터가 돌아다니는 3D 공간이 아니며, 기획도 로비를 UI로만 유지하기로 확정했다. **공유 3D 로비 허브는 범위 밖이다.**

##### 백엔드 경제 권위는 이 문서의 결정이 아니다

로비 경제를 HTTP 백엔드가 검증·저장하는 구성은 **출시 방침과 양립하지 않는다.** 출시 방침은 로컬 `SaveGame`을 영구 저장 원본으로 두고 백엔드 경제 권위를 두지 않기 때문이다. 그 구성은 [SaveBackendMigration.md](SaveBackendMigration.md)의 **학습용 3단계**로만 존재하며, 채택하려면 로컬 세이브 출시 방침 자체를 폐기해야 한다.

정리하면 경계는 이렇다.

```text
이 문서 (실제 로드맵)          레이드를 돌릴 데디서버 프로세스를 만든다
                               로비 경제는 건드리지 않는다

SaveBackendMigration (학습용)  로비 경제까지 서버 권위로 옮기는 연습
                               그 안에서 "UE 데디 로비 vs 백엔드 서비스" 비교를 다룬다
```

나중에 공유 허브나 백엔드 경제 권위를 실제로 채택하게 되면, 그건 이 절을 고치는 별도 결정이다.

### F. 운영

```text
F1  프로세스 수명 - 레이드 종료 후 서버를 내릴지 재사용할지, 유휴 타임아웃
F2  런타임 시크릿 주입 - 커맨드라인/환경변수/설정 파일 중 무엇으로
                        클라 패키징에 절대 포함되지 않을 것
F3  로그 수집 - LogLS 카테고리를 서버에서 어떻게 남기고 회수할지
F4  크래시 처리 - 레이드 중 서버 크래시 시 참가자 결과 정책
```

### G. 검증

```text
G1  헤드리스 실행 확인 (렌더 없이 뜨는지)
G2  서버 1 + 클라 2~3 로컬 접속 테스트
G3  레이드 입장 payload 가 플레이어별로 안 섞이는지  ← D2/D3 회귀 테스트
G4  중도 이탈 / 재접속 시나리오
G5  결과 저장 ACK 타임아웃 경로
```

G3은 기존 테스트 체크리스트([ItemSaveNetworkStructure.md](ItemSaveNetworkStructure.md)의 `2인 테스트 준비 후`)와 겹치므로 그쪽을 재사용한다.

---

## 단계 제안

| 단계 | 범위 | 끝나면 |
|---|---|---|
| **D-1** | A1~A2 + B1~B2 | 서버 타깃이 컴파일되고 헤드리스로 뜬다 |
| **D-2** | D2~D4 (전역 상태 정리) | 3인 데이터가 안 섞이는 구조 |
| **D-3** | C1~C4 (접속/이탈) | 입장·이탈이 정의된 동작을 한다 |
| **D-4** | E1~E3 (접속 경로) | 클라가 실제로 붙어 레이드를 완주 |
| **D-5** | F + G | 운영·검증 |

**D-2를 D-3보다 먼저 두는 이유:** 전역 상태 정리를 미루면 접속 처리를 붙이는 순간 인벤토리 혼선이 접속 버그처럼 보여 원인 추적이 어려워진다. 이미 알려진 취약점이므로 먼저 닫는다.

D-1은 나머지와 독립적이고 되돌리기 쉬우므로 단독 착수 가능하다.

---

## 미결정 사항

```text
- C2  레이드 중 연결 해제 시 결과 확정 정책
- C3  재접속 허용 여부
- D1  SeamlessTravel 사용 여부
- E4  OnlineSubsystem(Steam) 도입 시점 — 세션 검색·NAT 필요 여부에 달림
- F1  서버 프로세스 재사용 vs 매치마다 신규
- A2  렌더 모듈(LostSignalVisionShaders)의 서버 타깃 빌드 가부
```
