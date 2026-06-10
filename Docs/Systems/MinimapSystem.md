# Minimap 시스템

## 현재 구현 메모: 탐색 프로토콜 연동

- 미니맵 기본 표시, 지형지물, 플레이어 포인트, 시야각, 적, 루팅 오브젝트, 탈출구, 지역/퀘스트 표시는 `DT_Protocol`의 탐색 프로토콜 row를 기준으로 판정한다.
- 미니맵은 신호 게이지로 비활성화된 칩 슬롯을 제외한 탐색 프로토콜 합산값을 현재 레벨로 사용한다.
- 전체 장착 칩 합산값은 이전 해금 레벨로 사용해 `Protocol_Protected_Level` 보호 표시를 판정한다.
- `DT_Protocol`이 없거나 해당 row가 없으면 기존 `FLSMinimapRevealPolicy` 기본값을 fallback으로 사용한다.

## 목적

미니맵은 레이드 중 로컬 플레이어 기준으로 현재 위치, 시야각, 적, 루팅 오브젝트, 월드 드랍 아이템, 탈출구, 플레이어 시야 판정에 쓰는 지형 정보를 표시한다.

v1은 SceneCapture/RenderTarget을 쓰지 않는다. 지형은 우선 기존 시야 시스템의 `ULSVisionSurfaceComponent`와 `ULSVisionOccluderComponent`를 재사용하고, 미니맵 전용 보정이 필요한 경우에만 `ALSMinimapShapeActor`로 배치한 간단 도형을 추가로 그린다.

## 책임 분리

```text
ULSMinimapWidget
-> 로컬 플레이어 중심 / 카메라 기준 방향 좌표 변환
-> 시야 지형 / 보정 도형 / 마커 / 플레이어 포인트 / 시야각 렌더링
-> 탐색 프로토콜 기준 표시 정책 적용

ULSMinimapSubsystem
-> 월드의 미니맵 마커와 지형 도형 등록 목록 관리

ULSMinimapMarkerComponent
-> 적, 루팅 오브젝트, 월드 드랍 아이템, 탈출구의 미니맵 표시 정보 보관

ULSMinimapObstacleComponent
-> 시야를 막지는 않지만 미니맵에 보여야 하는 펜스/난간/낮은 장애물의 콜라이더 표시 정보 보관

ALSMinimapShapeActor
-> 시야 판정 데이터만으로 부족할 때 레벨 디자이너가 배치하는 보정 도형 데이터
```

`WBP_PlayerHUD`는 `ULSPlayerHUDWidget`을 부모로 쓰고, 필수 자식 위젯 `Minimap`을 `BindWidget`으로 제공해야 한다. `Minimap`의 부모 클래스는 `ULSMinimapWidget`이다.
`WBP_ChipStation`도 칩 장착/신호 게이지 조작 결과를 즉시 보여주려면 같은 부모 클래스의 자식 위젯을 `Minimap` 이름으로 배치해야 한다. 이 미니맵은 실제 월드 데이터를 복제하지 않고 고정 더미 지형/마커를 그리는 프리뷰 모드로 동작한다. 테스트 UI에서는 신호 게이지 퍼센트로 계산한 임시 탐색 레벨을 현재/이전 레벨에 같이 넘겨 순수 레벨별 표시를 확인한다.

## 표시 대상

| 대상 | 연결 방식 |
|------|-----------|
| 플레이어 현재 위치 | `ULSMinimapWidget`이 관찰 중인 로컬 Pawn을 중앙 포인트로 표시 |
| 적 | `ALSEnemyCharacter`의 `ULSMinimapMarkerComponent` |
| 루팅 오브젝트 | `ALSLootBox`의 `ULSMinimapMarkerComponent` |
| 월드 드랍 아이템 | `ALSWorldDroppedItem`의 `ULSMinimapMarkerComponent` |
| 탈출구 | `ALSExtractionZone`의 `ULSMinimapMarkerComponent` |
| 지형지물 | `ULSVisionSurfaceComponent`, `ULSVisionOccluderComponent`, `ULSMinimapObstacleComponent`, 필요 시 `ALSMinimapShapeActor` |
| 지역/위험도/퀘스트 위치 | 아직 실제 시스템 미연결. 향후 지역/퀘스트 액터 또는 데이터 소유자가 미니맵 마커/도형으로 연결 |

루팅 박스는 열리면 미니맵 마커를 숨긴다. 월드 드랍 아이템은 아이템 RowName과 수량이 유효할 때만 표시한다. 탈출구는 미니맵 가장자리 방향 표시와 거리 텍스트를 함께 표시한다.

미니맵의 위쪽 방향은 플레이어 정면을 따라 회전하지 않고, 로컬 카메라의 화면 위쪽 벡터를 지면에 투영한 방향을 따른다. 플레이어는 중앙에 고정하고, 지형/마커/시야각 부채꼴은 같은 카메라 기준 좌표계로 표시한다. 플레이어 시야는 `SightAngleDegrees` 기준의 부채꼴 면으로 미니맵 끝까지 렌더링한다.

## 탐색 프로토콜

탐색 프로토콜 값은 기존 칩 시스템과 동일하게 `LSChipStats::AggregateChipProtocolTotals()`의 `Navigation` 합산값을 사용한다.

```text
ULSMinimapWidget
-> ULSSaveSubsystem::GetChipEquipmentSlots
-> ULSSaveSubsystem::GetChipSignalGaugePercent
-> 신호 비활성 슬롯 제외
-> LSChipStats::AggregateChipProtocolTotals
-> FLSMinimapRevealPolicy 기준 표시 여부 결정
```

`DT_Protocol`의 탐색 프로토콜 row가 있으면 row의 `Protocol_Required_Level`과 `Protocol_Protected_Level`이 표시 여부의 단일 출처다. row가 없을 때만 `FLSMinimapRevealPolicy` 기본값을 fallback으로 사용한다.

| 해금 항목 | 현재 연결 상태 |
|-----------|----------------|
| `Minimap` | 미니맵 UI 활성화, 지형지물 표시 |
| `Exit_Point` | 탈출구 위치 표시 |
| `Player_Point` | 플레이어 현재 위치 표시 |
| `Minimap_View_Angle` | 플레이어 미니맵 시야각 표시 |
| `Minimap_Region`, `Region_Quest` | 지역 위치, 해당 지역 퀘스트 위치 표시. 실제 시스템 연결 전에는 프리뷰 더미 마커로 표시 |
| `Quest` | 전체 퀘스트 위치 표시. 실제 시스템 연결 전에는 프리뷰 더미 마커로 표시 |
| `Quest_Distance` | 탈출구 및 퀘스트 거리 표시 |
| `Minimap_View_Angle_Enemy` | 플레이어 시야각 안 몬스터 표시 |
| `Minimap_View_Angle_Looting_Object` | 플레이어 시야각 안 루팅 오브젝트/월드 드랍 아이템 표시 |
| `Minimap_Enemy` | 미니맵 범위 안 모든 몬스터 표시. 정보 유지는 `Protocol_Protected_Level`로 판정 |
| `Minimap_Looting_Object` | 미니맵 범위 안 모든 루팅 오브젝트/월드 드랍 아이템 표시. 정보 유지는 `Protocol_Protected_Level`로 판정 |

### 테스트 UI 프리뷰 단계

`WBP_ChipStation`의 미니맵 프리뷰는 실제 월드 데이터를 쓰지 않고 `ULSMinimapWidget::DrawPreviewData()`의 고정 더미 데이터를 그린다.

| 탐색 레벨 | 프리뷰 표시 |
|-----------|-------------|
| Lv.1 | 미니맵 UI, 지형지물, 탈출구 위치 |
| Lv.2 | 플레이어 현재 위치, 미니맵 시야각 |
| Lv.3 | 지역 위치, 해당 지역 퀘스트 위치 |
| Lv.4 | 전체 퀘스트 위치, 탈출구 및 퀘스트 거리 |
| Lv.5 | 미니맵 시야각 안 몬스터 |
| Lv.6 | 미니맵 시야각 안 아이템 |
| Lv.8 | 미니맵 상 모든 몬스터, 아이템 위치 |

`Protocol_Navigation_11`과 `Protocol_Navigation_12`는 `Protocol_Protected_Level=5`이므로 실제 장착 칩 상태에서 이전 탐색 레벨이 Lv.8 이상이면 현재 탐색 레벨이 Lv.5까지 내려가도 전체 몬스터/아이템 정보가 사라지지 않는다. 칩 스테이션 테스트 UI는 순수 레벨 확인용이므로 이전 레벨도 현재 레벨과 같게 둔다.

## 지형 표시

미니맵 지형은 `ULSVisionSubsystem`에 등록된 시야 판정 데이터를 먼저 사용한다. `ULSVisionSurfaceComponent`가 있으면 대상 프리미티브의 월드 바운드를 합산한 뒤 보라색 면으로 채우고, 같은 액터에 `ULSVisionOccluderComponent`가 함께 있으면 중복 표현을 피하기 위해 오클루더 선분은 생략한다.

`ULSVisionSurfaceComponent` 없이 `ULSVisionOccluderComponent`만 있는 액터는 오클루더 선분을 보라색 선으로 그린다. 지형 면과 선분은 미니맵 원 반경 안에서만 그려서 바깥으로 삐져나가지 않게 한다. 이 방식은 플레이어 시야를 실제로 가리는 벽, 장애물, 차폐물과 미니맵 지형 표시를 같은 데이터에서 관리하기 위한 기본 경로다.

펜스나 난간처럼 구멍이 있어 시야를 막지 않는 물체는 `ULSMinimapObstacleComponent`를 붙여 미니맵 전용으로 표시한다. 이 컴포넌트는 지정한 `TargetPrimitives`를 우선 사용하고, 비어 있으면 owner의 Pawn Block 콜라이더를 수집해 보라색 외곽선으로 그린다. 시야 판정에는 영향을 주지 않는다.

StaticMeshActor 기반 펜스는 Outliner에서 액터를 선택한 뒤 우클릭 메뉴의 `LostSignal > Add Minimap Obstacle`로 `ULSMinimapObstacleComponent`를 추가할 수 있다.

`ALSMinimapShapeActor`는 시야 판정 데이터만으로 표현하기 어려운 안내선이나 추상화된 영역을 보정할 때 사용한다.

`ALSMinimapShapeActor`는 다음 도형을 지원한다.

| 도형 | 용도 |
|------|------|
| Box | 방, 건물, 넓은 장애물 |
| Circle | 원형 지형/범위 |
| Polyline | 통로, 벽선, 길 안내 |

지형 도형 색은 각 액터의 `FillColor`로 정한다. 기본은 보라색 계열이다.

## 주의 사항

- `WBP_PlayerHUD` 또는 `WBP_ChipStation`에 `Minimap` 바인딩이 없으면 `LogLS Warning`을 남긴다.
- 마커 등록/해제는 컴포넌트와 도형 액터의 `BeginPlay`/`EndPlay`에서 처리한다.
- 미니맵은 로컬 HUD 위젯에서만 렌더링하므로 서버 권위 판단을 변경하지 않는다.
- 표시 수치의 단일 출처는 코드/설정/DataTable이다. 문서에는 실제 수치를 복붙하지 않는다.
