#pragma once

#include "CoreMinimal.h"

struct FLSSkillNodeView;

/** 자동 배치 파라미터. 전부 정규화 반지름(코어=0, 최외곽 노드가 대략 1)이다. */
struct FLSSkillNodeLayoutParams
{
	// 1차 링의 반지름.
	float FirstRingRadius = 0.34f;

	// 링 하나당 반지름 증가분.
	float RingRadiusStep = 0.30f;

	// 같은 링 안에서 선행 깊이 1단당 반지름 증가분.
	//
	// 이게 0이면 2차 링 24개가 한 반지름에 몰려 서브->메인->강화 연결선이 원주를 따라 흐르며
	// 노드 원과 겹친다. 2차 링은 한 링 안에서 두 번 머지하므로 깊이가 3단이다.
	//
	// 실측 데이터에서 가장 가까운 두 노드는 같은 각도에 놓이는 서브(깊이1)와 강화(깊이3)이고,
	// 그 간격이 이 값의 2배다. 값을 줄이면 그 둘이 먼저 붙는다.
	float InRingDepthStep = 0.07f;
};

namespace LSSkillNodeLayout
{
	/**
	 * 선행 관계에서 방사형 좌표를 유도한다. 좌표 데이터가 없어도 그래프를 그릴 수 있게 하는 폴백이다.
	 *
	 * 기획 시트의 Slot 컬럼은 각도가 아니다 — 정수가 아니라 R2-M03 같은 라벨이고 종류별로 번호가
	 * 1부터 다시 시작한다. 사전순으로 정렬하면 2차 링이 서브 12개 / 강화 4개 / 메인 8개로 뭉쳐
	 * 실제 배치와 전혀 다른 그림이 나온다. 그래서 Slot 을 쓰지 않는다.
	 *
	 * 규칙:
	 *   반지름 = 링 + 링 안에서의 선행 깊이
	 *   각도   = 코어 자식은 360도 균등 분할, 그 아래는 부모 각도를 중심으로 섹터 폭 안에 균등 분할,
	 *            같은 링의 머지 노드는 선행들 각도의 원형 평균
	 *
	 * 원형 평균을 쓰는 이유는 1차 링이 닫힌 고리이기 때문이다 — 마지막 서브 노드가 270도와 0도를
	 * 선행으로 갖는데, 단순 산술 평균은 180도(정반대)를 내놓는다.
	 *
	 * Views 는 한 캐릭터의 노드만 담아야 한다. 여러 캐릭터를 섞으면 섹터 분할이 깨진다.
	 */
	LOSTSIGNAL_API void ComputeAutoLayout(
		const TArray<FLSSkillNodeView>& Views,
		const FLSSkillNodeLayoutParams& Params,
		TMap<FName, FVector2D>& OutNormalizedPositions);

	/**
	 * 정규화 좌표 -> 위젯 로컬 오프셋(중심 기준).
	 *
	 * 짧은 변 하나로만 스케일한다. X 와 Y 에 각각 폭과 높이를 곱하면 원형 링이 타원이 된다.
	 * 나중에 에디터 배치 툴을 만들면 툴도 이 함수를 호출해야 툴과 UI 의 모양이 일치한다.
	 */
	LOSTSIGNAL_API FVector2D ToLocalOffset(const FVector2D& Normalized, const FVector2D& LocalSize, float FillRatio);

	/** 링 배경 원을 그릴 때 쓰는 반지름 환산. ToLocalOffset 과 같은 스케일을 쓴다. */
	LOSTSIGNAL_API float ToLocalRadius(float NormalizedRadius, const FVector2D& LocalSize, float FillRatio);

	/** 링 번호에 대응하는 정규화 반지름(깊이 보정 없음). 배경 원 표시용. */
	LOSTSIGNAL_API float GetRingNormalizedRadius(int32 Ring, const FLSSkillNodeLayoutParams& Params);
}
