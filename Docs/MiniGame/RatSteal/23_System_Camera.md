# RatSteal System — Camera / Y-Sort

> 범위: **1차**. 맵 크기는 [30_Level_Layout.md](30_Level_Layout.md)가 소유.

## 목적

직교(Orthographic) 추종 카메라와 2D 깊이 정렬(Y-Sort) 규칙을 정의한다.

## 카메라 (원작 CinemachineCamera)

```text
투영     직교(Orthographic) — 쿼터뷰 톱다운
추종     플레이어를 부드럽게 따라감(데드존/러프 추종)
경계     맵 밖을 비추지 않게 클램프(맵 경계 내로 제한)
```

## Y-Sort (깊이 정렬)

```text
원작     SpriteRenderer OrderInLayer = -WorldY 기준으로 앞뒤 정렬
의도     아래쪽(화면 하단=Y 작음)에 있는 오브젝트가 앞에 그려짐
대상     플레이어/농부/작물/이펙트 등 동적 오브젝트(YSort 컴포넌트)
배경     order -200000 (항상 뒤)
```

## UE 매핑 (요약)

```text
카메라    → UCameraComponent (Orthographic) + 추종 액터/스프링암, 경계 클램프
가시 범위  → OrthoWidth 950 기준으로 플레이 검증 중
Y-Sort    → 매 프레임 TranslucencySortPriority = round(-WorldY)
          또는 평면을 살짝 기울여 Z깊이로 자연 정렬 (택1, 99_TechMapping에서 확정)
배경      → 가장 낮은 sort priority 고정
```

## 현재 UE 구현 메모

- `ULSRatYSortComponent`는 렌더 컴포넌트의 월드 Z를 기준으로 `TranslucentSortPriority`를 계산한다.
- 같은 선에 겹치는 오브젝트의 깜빡임을 줄이기 위해 타입별 `SortOffset`을 사용한다.
- 기본 보정은 작물 0, 부쉬 10, 플레이어/농부 30, 던진 아이템 60이다.
- Paper2D 렌더 컴포넌트가 같은 깊이 평면에서 깜빡이지 않도록 소량의 Y-depth lane을 같이 사용한다. 작물은 0, 부쉬는 -2, 플레이어/농부는 -4, 작물 완료 파티클은 작물보다 앞인 -2를 기준으로 둔다.
- `ALSRatPlayer` 카메라는 미니게임 전용 포스트 프로세스 설정을 적용한다. 전체 노출 톤은 맵/프로젝트 기본값을 따르고, 로컬 익스포저 대비, Bloom, Vignette를 꺼서 화면 가장자리 밝아짐이 메인 게임 렌더 설정과 분리되도록 한다.
- 좌우 이동 중 스프라이트가 번져 보이지 않도록 RatSteal 카메라에서는 Motion Blur, Depth of Field, Chromatic Aberration을 0으로 고정한다.
- 맵 가장자리에서는 카메라 중심을 맵 경계 안쪽으로 클램프한다. 플레이어는 이동 가능 경계까지 갈 수 있지만, 직교 카메라가 배경 밖을 비추지 않도록 카메라 암의 상대 위치를 보정한다.
- 원본 D2D 렌더러는 스프라이트를 조명 없이 `DrawBitmap`으로 직접 그리므로, 플레이어/농부 스프라이트는 UE에서도 Unlit Paper2D 머티리얼을 사용해 바닥색·월드 조명에 색이 물들지 않도록 한다.

## 미해결 / 확인 필요

```text
- Paper2D 평면 배치: 정직교 + sort priority vs 약간 기운 평면(Z깊이) 중 택1
- 직교 카메라 줌(가시 범위) 값 — 픽셀→월드 환산과 함께 플레이로 조정(50_Balance)
- 카메라 경계 클램프용 맵 경계 정의(30_Level_Layout)
```
