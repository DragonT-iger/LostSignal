# Unreal MCP 설치 가이드

Claude Code(AI)로 언리얼 에디터를 직접 제어하는 MCP 연동 방법입니다.

---

## 이게 뭔가요?

Claude에게 말로 요청하면 언리얼 에디터에서 직접 실행됩니다.

```
예시:
"플레이어 위치에 큐브 스폰해줘"
"BP_Enemy에 Health 변수 추가해줘"
"레벨에 있는 액터 목록 알려줘"
```

구조: `언리얼 에디터 ↔ Python 서버 ↔ Claude Code`

---

## 필요한 것

- Unreal Engine 5.5 이상
- Python 3.10 이상
- [uv](https://github.com/astral-sh/uv) (Python 패키지 매니저)
- Claude Code

---

## 설치 순서

### 1. 레포 클론

아무 폴더에나 받으면 됩니다. 프로젝트 안에 넣을 필요 없습니다.

```bash
cd C:\Users\본인계정
git clone https://github.com/chongdashu/unreal-mcp.git
```

---

### 2. 플러그인을 언리얼 프로젝트에 복사 

```bash
# Plugins 폴더가 없으면 먼저 생성
mkdir "C:\경로\내프로젝트\Plugins"

# 플러그인 복사
xcopy "C:\Users\본인계정\unreal-mcp\MCPGameProject\Plugins\UnrealMCP" "C:\경로\내프로젝트\Plugins\UnrealMCP" /E /I
```

---

### 3. .uproject에 플러그인 등록 ## 이건 팀원은 안해도 됨 프로젝트 클론하면 자동으로 들어감

`내프로젝트.uproject` 파일을 메모장으로 열어서 `Plugins` 배열에 추가합니다.

```json
"Plugins": [
  ...기존 항목들...,
  {
    "Name": "UnrealMCP",
    "Enabled": true
  }
]
```

---

### 4. 코드 수정 (UE 5.5+ 호환 패치)

플러그인이 UE 5.5 기준으로 작성되어 있어서 최신 버전에서는 아래 수정이 필요합니다.

**`Plugins/UnrealMCP/Source/UnrealMCP/Private/MCPServerRunnable.cpp`**

아래 줄을 삭제합니다 (전역 변수명 충돌):
```cpp
// 이 줄 삭제
const int32 BufferSize = 8192;
```

**`UnrealMCPBlueprintCommands.cpp`** 및 **`UnrealMCPBlueprintNodeCommands.cpp`**

`ANY_PACKAGE`가 UE 5.4+에서 제거됐습니다. 전부 교체합니다:
```cpp
// 변경 전
FindObject<UClass>(ANY_PACKAGE, *ClassName)

// 변경 후
FindFirstObject<UClass>(*ClassName)
```

---

### 5. 빌드

1. `.uproject` 파일 **우클릭** → `Generate Visual Studio project files`
2. Visual Studio 실행
3. 빌드 타겟: `Development Editor / Win64`
4. `Ctrl+Shift+B` 빌드

---

### 6. Python 서버 셋업

```bash
cd C:\Users\본인계정\unreal-mcp\Python

uv venv
uv pip install -e .
```

---

### 7. Claude Code에 MCP 서버 등록

프로젝트 폴더에서 터미널 열고 아래 명령 실행:

```bash
claude mcp add unrealMCP -- uv --directory "C:/Users/본인계정/unreal-mcp/Python" run unreal_mcp_server.py
```

---

## 매번 사용할 때

> **순서 중요합니다**

1. **언리얼 에디터 먼저 실행** (프로젝트 열기)
2. **Claude Code 실행** (또는 재시작)
3. Claude에게 요청

---

## 안 될 때

| 증상 | 해결 |
|------|------|
| Claude가 unrealMCP 툴을 모름 | Claude Code 재시작 |
| Python 서버 연결 실패 | 언리얼 에디터를 먼저 켜세요 |
| 빌드 에러 `ANY_PACKAGE` | 4단계 코드 수정 확인 |
| 빌드 에러 `BufferSize` | 4단계 코드 수정 확인 |

에러 로그: `unreal-mcp/Python/unreal_mcp.log` 파일 확인

---

## 참고

- 레포: https://github.com/chongdashu/unreal-mcp
- 아직 Experimental 단계 (API 변경 가능)
- Editor 전용 플러그인이라 패키징/배포에는 포함 안 됨

---

## Windows + Python 3.12 설정 (2026-04-01 업데이트)

### uv 설치 (Windows PowerShell)

```powershell
powershell -ExecutionPolicy Bypass -Command "irm https://astral.sh/uv/install.ps1 | iex"
```

- 설치 경로: `C:\Users\<사용자>\.local\bin\uv.exe`
- 새 터미널을 열어야 `uv` 명령이 인식됨

### Python 3.12 가상환경 생성

시스템 Python이 3.14여도 **3.12로 명시 지정**해야 합니다.
(pydantic-core v2.27.2가 Python 3.13까지만 지원)

```bash
cd C:/Users/<사용자>/unreal-mcp/Python

# 기존 venv 삭제
rm -rf .venv

# Python 3.12로 새로 생성 (uv가 자동 다운로드)
uv venv --python 3.12

# 의존성 설치
uv pip install -e .
```

### Claude Code MCP 설정

프로젝트의 `~/.claude.json` 에서 해당 프로젝트 섹션에 추가:

```json
"mcpServers": {
  "unrealMCP": {
    "type": "stdio",
    "command": "uv",
    "args": [
      "--directory",
      "C:/Users/<사용자>/unreal-mcp/Python",
      "run",
      "unreal_mcp_server.py"
    ],
    "env": {}
  }
}
```

Claude Code 재시작 후 언리얼 에디터가 열려 있으면 자동 연결됩니다.

### C++ 플러그인 개선사항

`spawn_actor` 명령에 선택적 `mesh_path` 파라미터 추가:

**UnrealMCPEditorCommands.cpp - HandleSpawnActor()**

StaticMeshActor 생성 후 메시 자동 할당:

```cpp
if (ActorType == TEXT("StaticMeshActor"))
{
    AStaticMeshActor* SMActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
    NewActor = SMActor;

    // Optional: set mesh if mesh_path param provided
    FString MeshPath;
    if (SMActor && Params->TryGetStringField(TEXT("mesh_path"), MeshPath))
    {
        UStaticMesh* Mesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *MeshPath));
        if (Mesh)
        {
            SMActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
        }
    }
}
```

**사용 예:**

```json
{
  "type": "SPAWN_ACTOR",
  "params": {
    "name": "MyCube",
    "type": "StaticMeshActor",
    "location": [0, 0, 400],
    "mesh_path": "/Engine/BasicShapes/Cube.Cube"
  }
}
```

---

## 트러블슈팅

| 문제 | 해결 |
|------|------|
| `uv` 명령 안 됨 | 새 PowerShell 열기 (PATH 갱신) |
| pydantic-core 빌드 실패 | Python 3.14 → 3.12로 변경 (`uv venv --python 3.12`) |
| MCP 서버 연결 실패 | 언리얼 에디터 먼저 실행 (포트 55557 대기 중) |
| Claude Code가 unrealMCP 못 찾음 | Claude Code 재시작 |
