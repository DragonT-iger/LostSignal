# Minimap 시스템

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

## 표시 대상

| 대상 | 연결 방식 |
|------|-----------|
| 플레이어 현재 위치 | `ULSMinimapWidget`이 관찰 중인 로컬 Pawn을 중앙 포인트로 표시 |
| 적 | `ALSEnemyCharacter`의 `ULSMinimapMarkerComponent` |
| 루팅 오브젝트 | `ALSLootBox`의 `ULSMinimapMarkerComponent` |
| 월드 드랍 아이템 | `ALSWorldDroppedItem`의 `ULSMinimapMarkerComponent` |
| 탈출구 | `ALSExtractionZone`의 `ULSMinimapMarkerComponent` |
| 지형지물 | `ULSVisionSurfaceComponent`, `ULSVisionOccluderComponent`, `ULSMinimapObstacleComponent`, 필요 시 `ALSMinimapShapeActor` |

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

적 표시는 고정 규칙이 아니다. `FLSMinimapRevealPolicy`의 기본값에 따라 숨김, 플레이어 시야각 안에서만 표시, 항상 표시 중 하나로 결정된다. 최종 임계값을 기획 데이터로 분리해야 하면 별도 DataTable을 단일 출처로 만든다.

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

- `WBP_PlayerHUD`에 `Minimap` 바인딩이 없으면 `LogLS Warning`을 남긴다.
- 마커 등록/해제는 컴포넌트와 도형 액터의 `BeginPlay`/`EndPlay`에서 처리한다.
- 미니맵은 로컬 HUD 위젯에서만 렌더링하므로 서버 권위 판단을 변경하지 않는다.
- 표시 수치의 단일 출처는 코드/설정/DataTable이다. 문서에는 실제 수치를 복붙하지 않는다.
