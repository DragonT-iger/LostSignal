#pragma once

#include "CoreMinimal.h"
#include "Data/LSSkillNodeRow.h"
#include "LSSkillNodeGraphTypes.generated.h"

struct FLSSkillNodeRef;

/** 노드 하나의 표시 상태. */
UENUM(BlueprintType)
enum class ELSSkillNodeState : uint8
{
	// 선행이 충족되지 않았다.
	Locked,
	// 선행이 충족됐다. 비용까지 볼 수 있게 되면 여기서 "비용 부족"이 갈라진다.
	Available,
	// 활성됐다.
	Activated
};

/**
 * 노드 그래프 위젯이 소비하는 표시용 스냅샷.
 *
 * 위젯이 DataTable 을 직접 모르게 하는 경계다(FLSMinimapMarkerSnapshot 과 같은 결).
 * 종류는 ELSSkillNodeKind 를 그대로 쓴다 — 표시용 enum 을 따로 두면 두 어휘가 어긋난다.
 */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSSkillNodeView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|SkillNode")
	FName NodeKey;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|SkillNode")
	ELSSkillNodeKind Kind = ELSSkillNodeKind::None;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|SkillNode")
	ELSSkillNodeState State = ELSSkillNodeState::Locked;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|SkillNode")
	int32 Ring = 0;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|SkillNode")
	FText DisplayName;

	// 연결선 목록이 여기서 나온다. 별도 연결선 데이터를 두지 않는다.
	UPROPERTY(BlueprintReadOnly, Category="LS/UI|SkillNode")
	FName Prerequisite_1;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|SkillNode")
	FName Prerequisite_2;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|SkillNode/Cost")
	FName ChipGrade;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|SkillNode/Cost")
	int32 RequiredQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|SkillNode/Cost")
	int32 CoinCost = 0;

	bool IsCore() const
	{
		return Kind == ELSSkillNodeKind::Core;
	}
};

namespace LSSkillNodeViews
{
	/**
	 * 인덱스 엔트리 + 활성 집합 -> 표시용 스냅샷.
	 *
	 * 상태 판정 규칙은 최종 로직과 같다.
	 *   Activated = 활성 집합에 있음
	 *   Available = 선행이 둘 다 비었거나, 둘 중 하나라도 활성  <- ANY (ALL 이 아니다)
	 *   Locked    = 그 외
	 *
	 * 비용 충족과 링 해금은 아직 보지 않는다. 세이브가 붙으면 Available 안에서 갈라진다.
	 */
	LOSTSIGNAL_API void BuildViews(
		const TArray<const FLSSkillNodeRef*>& Nodes,
		const TSet<FName>& ActivatedNodeKeys,
		TArray<FLSSkillNodeView>& OutViews);

	/**
	 * 활성 노드가 하나도 없을 때의 시작 집합. 코어는 시스템 활성 시 자동 활성이다(기획).
	 * 세이브가 붙기 전까지 그래프가 이 상태로 그려진다.
	 */
	LOSTSIGNAL_API void CollectAutoActivatedNodeKeys(
		const TArray<const FLSSkillNodeRef*>& Nodes,
		TSet<FName>& OutActivatedNodeKeys);
}
