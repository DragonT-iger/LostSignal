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
Y-Sort    → 매 프레임 TranslucencySortPriority = round(-WorldY)
          또는 평면을 살짝 기울여 Z깊이로 자연 정렬 (택1, 99_TechMapping에서 확정)
배경      → 가장 낮은 sort priority 고정
```

## 미해결 / 확인 필요

```text
- Paper2D 평면 배치: 정직교 + sort priority vs 약간 기운 평면(Z깊이) 중 택1
- 직교 카메라 줌(가시 범위) 값 — 픽셀→월드 환산과 함께 플레이로 조정(50_Balance)
- 카메라 경계 클램프용 맵 경계 정의(30_Level_Layout)
```
