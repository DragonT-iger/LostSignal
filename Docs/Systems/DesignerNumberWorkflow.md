# 기획자 수치 관리

기획자는 수치 데이터를 **엑셀 테이블**로 작성하고, 프로그래머가 DataTable로 임포트한다.

## 워크플로우

1. 기획자 → 엑셀에서 수치 작성 → 프로그래머가 DataTable로 임포트
2. 런타임에 DataTable 값 로드 → 각 캐릭터 인스턴스 컨트롤러의 UPROPERTY에 덮어씀 (캐릭터별 행 분리)
3. DataTable 행이 없으면 블루프린트 기본값(UPROPERTY 초기값) 사용

## 수치 조정 (두 경로만)

수치는 아래 두 가지로만 조정한다. `BP_GE_*` 같은 별도 GameplayEffect 블루프린트를 만들어 수치를 넣지 않는다.

1. **DataTable 편집** — 영구 반영. 쿨타임 등 GE 관련 수치도 DataTable에서 읽는다.
2. **PIE Details 패널** — PIE 중 Outliner에서 인스턴스(PlayerController 등) 선택 → Details 패널에서 직접 수정 (일시 튜닝).
