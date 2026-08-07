#pragma once

#include "CoreMinimal.h"
#include "Data/LSSkillNodeRow.h"

class UDataTable;

/**
 * 노드 하나를 종류와 무관하게 다루기 위한 인덱스 엔트리.
 *
 * 기획 시트가 종류별로 5테이블로 나뉘어 있으므로, 선행 조회·그래프 순회·UI 배치가 전부
 * 테이블을 넘나들게 된다. 그 비용을 여기서 한 번에 갚는다 — 해석·UI 계층은 테이블이
 * 5개라는 사실을 모른다.
 *
 * USTRUCT 이 아니다. 복제 대상이 아니고 원본 row 포인터를 들고 있다(FLSChipProtocolTotals 와 같은 결).
 */
struct FLSSkillNodeRef
{
	FName NodeKey;

	ELSSkillNodeKind Kind = ELSSkillNodeKind::None;

	int32 CharacterID = 0;

	int32 Ring = 0;

	FName Slot;

	FText NodeName;

	// 코어는 둘 다 비어 있다. 둘 중 하나만 활성이면 충족(ANY).
	FName Prerequisite_1;
	FName Prerequisite_2;

	FName ChipGrade;
	int32 RequiredQuantity = 0;
	int32 CoinCost = 0;

	// Kind 별 원본 row. 소유하지 않는다 — DataTable 이 소유하며 재임포트 시 인덱스를 다시 빌드해야 한다.
	const void* Payload = nullptr;

	bool HasPrerequisite() const
	{
		return !Prerequisite_1.IsNone() || !Prerequisite_2.IsNone();
	}

	// Kind 가 맞지 않으면 nullptr. 잘못된 캐스팅을 호출부에서 막지 않아도 되게 한다.
	const FLSSkillNodeStatRow* AsStat() const
	{
		return (Kind == ELSSkillNodeKind::MainStat || Kind == ELSSkillNodeKind::SubStat)
			? static_cast<const FLSSkillNodeStatRow*>(Payload)
			: nullptr;
	}

	const FLSSkillNodeEnhanceRow* AsEnhance() const
	{
		return Kind == ELSSkillNodeKind::SkillEnhance
			? static_cast<const FLSSkillNodeEnhanceRow*>(Payload)
			: nullptr;
	}

	const FLSSkillNodeEvolveRow* AsEvolve() const
	{
		return Kind == ELSSkillNodeKind::SkillEvolve
			? static_cast<const FLSSkillNodeEvolveRow*>(Payload)
			: nullptr;
	}
};

/** 노드 키 -> 엔트리 통합 인덱스. 개별 노드 테이블을 대신하는 유일한 조회처다. */
struct FLSSkillNodeIndex
{
	TMap<FName, FLSSkillNodeRef> Nodes;

	// 캐릭터별 노드 키. 값은 Nodes 의 키이며 Ring -> 노드 키 사전순으로 정렬해 둔다.
	// GetRowMap 의 순회 순서가 보장되지 않으므로 표시 순서를 여기서 고정한다.
	TMap<int32, TArray<FName>> NodeKeysByCharacter;

	void Reset()
	{
		Nodes.Reset();
		NodeKeysByCharacter.Reset();
	}

	bool IsEmpty() const
	{
		return Nodes.IsEmpty();
	}

	const FLSSkillNodeRef* Find(const FName NodeKey) const
	{
		return Nodes.Find(NodeKey);
	}
};

namespace LSSkillNodes
{
	/**
	 * Stat_Field 가 가리킬 수 있는 토큰 목록. 이 배열이 단일 출처다.
	 *
	 * 토큰이 FLSCharacterStatRow 의 필드명과 항상 같지는 않다 — 기획 시트가 Char_HP_Recovery 를
	 * 쓰는데 코드 필드는 Char_Recovery 다. 토큰을 어트리뷰트로 번역하는 지점이 한 곳이면
	 * 어휘가 이원화되지 않으므로, 시트 토큰을 그대로 받는다.
	 */
	LOSTSIGNAL_API const TArray<FName>& GetKnownStatFields();

	/** Parameter_Field 가 가리킬 수 있는 스킬 row 필드 목록(Skill_Multiplier / Range_X / Skill_Cooldown). */
	LOSTSIGNAL_API const TArray<FName>& GetKnownParameterFields();

	/**
	 * 5테이블에서 통합 인덱스를 만든다.
	 *
	 * 잘못된 row 는 전체 로드를 실패시키지 않고 그 row 만 버리고 경고한다(skip-and-warn).
	 * 테이블이 null 이면 그 종류를 건너뛴다 — 미설정 경고는 호출부(LogMissingTables)가 담당한다.
	 */
	LOSTSIGNAL_API void BuildIndex(
		FLSSkillNodeIndex& OutIndex,
		const UDataTable* CoreTable,
		const UDataTable* MainStatTable,
		const UDataTable* SubStatTable,
		const UDataTable* EnhanceTable,
		const UDataTable* EvolveTable);

#if WITH_EDITOR
	/**
	 * 그래프 무결성 검사. 경고 로그만 남기고 동작은 바꾸지 않는다.
	 *
	 * 함정 둘을 지킨다.
	 *  - 사이클·도달성은 반드시 유향으로 돈다. 1차 링이 랩어라운드 때문에 무향으로는 닫힌 고리다.
	 *  - Ring 역행은 동일 링을 허용한다. 같은 링 안의 분기·머지가 정상 데이터다.
	 *
	 * 반환값은 발견한 문제 수다(0이면 정상). 테스트가 이 값을 본다.
	 */
	LOSTSIGNAL_API int32 ValidateGraph(const FLSSkillNodeIndex& Index);
#endif
}
