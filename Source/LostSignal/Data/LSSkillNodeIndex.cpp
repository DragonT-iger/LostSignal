#include "Data/LSSkillNodeIndex.h"

#include "Engine/DataTable.h"
#include "LostSignal.h"

namespace
{
// 이 파일 로컬 헬퍼는 전부 SkillNode 접두사를 붙인다.
// 유니티 빌드에서 다른 .cpp 의 같은 이름 헬퍼와 중복 정의로 터지는 것을 막기 위한 것이다.

// 코어 row 에는 선행 컬럼이 없다. 코어는 "선행이 없는 노드"라는 것이 정의다.
void SkillNodeReadPrerequisites(const FLSSkillNodeCoreRow& /*Row*/, FLSSkillNodeRef& /*OutRef*/)
{
}

template <typename RowType>
void SkillNodeReadPrerequisites(const RowType& Row, FLSSkillNodeRef& OutRef)
{
	OutRef.Prerequisite_1 = Row.Prerequisite_1;
	OutRef.Prerequisite_2 = Row.Prerequisite_2;
}

// Kind 별 필수 컬럼 검사. 통과하지 못한 row 는 인덱스에 넣지 않는다.
bool SkillNodeValidateKindColumns(const FLSSkillNodeRef& Ref)
{
	switch (Ref.Kind)
	{
	case ELSSkillNodeKind::MainStat:
	case ELSSkillNodeKind::SubStat:
	{
		const FLSSkillNodeStatRow* Row = Ref.AsStat();
		if (!Row)
		{
			return false;
		}
		if (Row->Stat_Field.IsNone() || !LSSkillNodes::GetKnownStatFields().Contains(Row->Stat_Field))
		{
			UE_LOG(LogLS, Warning, TEXT("[SkillNode] %s: Stat_Field '%s'는 알려진 스탯 토큰이 아니다 - 노드를 건너뛴다"),
				*Ref.NodeKey.ToString(), *Row->Stat_Field.ToString());
			return false;
		}
		if (Row->Operation == ELSSkillNodeOperation::None)
		{
			UE_LOG(LogLS, Warning, TEXT("[SkillNode] %s: Operation이 없다 - 노드를 건너뛴다"), *Ref.NodeKey.ToString());
			return false;
		}
		return true;
	}
	case ELSSkillNodeKind::SkillEnhance:
	{
		const FLSSkillNodeEnhanceRow* Row = Ref.AsEnhance();
		if (!Row)
		{
			return false;
		}
		if (Row->Skill_ID == 0)
		{
			UE_LOG(LogLS, Warning, TEXT("[SkillNode] %s: Skill_ID가 0이다 - 노드를 건너뛴다"), *Ref.NodeKey.ToString());
			return false;
		}
		if (!LSSkillNodes::GetKnownParameterFields().Contains(Row->Parameter_Field))
		{
			UE_LOG(LogLS, Warning, TEXT("[SkillNode] %s: Parameter_Field '%s'는 강화 가능한 필드가 아니다 - 노드를 건너뛴다"),
				*Ref.NodeKey.ToString(), *Row->Parameter_Field.ToString());
			return false;
		}
		return true;
	}
	case ELSSkillNodeKind::SkillEvolve:
	{
		const FLSSkillNodeEvolveRow* Row = Ref.AsEvolve();
		if (!Row)
		{
			return false;
		}
		if (Row->Base_Skill_ID == 0 || Row->Evolution_Skill_ID == 0)
		{
			UE_LOG(LogLS, Warning, TEXT("[SkillNode] %s: Base/Evolution Skill_ID가 비어 있다 - 노드를 건너뛴다"),
				*Ref.NodeKey.ToString());
			return false;
		}
		return true;
	}
	default:
		return true;
	}
}

template <typename RowType>
void SkillNodeCollectRows(FLSSkillNodeIndex& OutIndex, const UDataTable* Table, const ELSSkillNodeKind Kind)
{
	if (!Table)
	{
		return;
	}

	// 에셋의 Row 구조체가 다르면 row 메모리 해석이 전부 어긋난다. 캐스팅 전에 막는다.
	if (Table->GetRowStruct() != RowType::StaticStruct())
	{
		UE_LOG(LogLS, Warning, TEXT("[SkillNode] %s의 Row 구조체가 %s가 아니다 - 테이블 전체를 건너뛴다"),
			*Table->GetName(), *RowType::StaticStruct()->GetName());
		return;
	}

	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		const RowType* Row = reinterpret_cast<const RowType*>(Pair.Value);
		if (!Row)
		{
			continue;
		}

		if (const FLSSkillNodeRef* Existing = OutIndex.Nodes.Find(Pair.Key))
		{
			UE_LOG(LogLS, Warning, TEXT("[SkillNode] 노드 키 '%s'가 중복이다(기존 Kind=%d) - 뒤에 온 row를 무시한다"),
				*Pair.Key.ToString(), static_cast<int32>(Existing->Kind));
			continue;
		}

		if (Row->Character_ID == 0)
		{
			UE_LOG(LogLS, Warning, TEXT("[SkillNode] %s: Character_ID가 0이다 - 노드를 건너뛴다"), *Pair.Key.ToString());
			continue;
		}

		FLSSkillNodeRef Ref;
		Ref.NodeKey = Pair.Key;
		Ref.Kind = Kind;
		Ref.CharacterID = Row->Character_ID;
		Ref.Ring = Row->Ring;
		Ref.Slot = Row->Slot;
		Ref.NodeName = Row->Node_Name;
		Ref.ChipGrade = Row->Chip_Grade;
		Ref.RequiredQuantity = Row->Required_Quantity;
		Ref.CoinCost = Row->Coin_Cost;
		Ref.Payload = Row;
		SkillNodeReadPrerequisites(*Row, Ref);

		if (!SkillNodeValidateKindColumns(Ref))
		{
			continue;
		}

		// 키를 먼저 꺼낸다. Add(Ref.NodeKey, MoveTemp(Ref))로 쓰면 인자 평가 순서가 보장되지 않는다.
		const FName NodeKey = Ref.NodeKey;
		const int32 CharacterID = Ref.CharacterID;
		OutIndex.NodeKeysByCharacter.FindOrAdd(CharacterID).Add(NodeKey);
		OutIndex.Nodes.Add(NodeKey, MoveTemp(Ref));
	}
}
} // namespace

namespace LSSkillNodes
{
const TArray<FName>& GetKnownStatFields()
{
	// 기획 시트가 쓰는 토큰 그대로다. Char_HP_Recovery 는 코드 필드명(Char_Recovery)과 다르지만,
	// 토큰을 어트리뷰트로 번역하는 지점이 한 곳이면 문제가 되지 않는다.
	static const TArray<FName> KnownStatFields = {
		TEXT("Char_Attack"),
		TEXT("Char_Atkspeed"),
		TEXT("Char_Cal"),
		TEXT("Char_Crit"),
		TEXT("Char_CritDmg"),
		TEXT("Char_ArmorPen"),
		TEXT("Char_Health"),
		TEXT("Char_Defence"),
		TEXT("Char_HP_Recovery"),
		TEXT("Char_Poise"),
		TEXT("Char_Speed")
	};
	return KnownStatFields;
}

const TArray<FName>& GetKnownParameterFields()
{
	// FLSCharacterSkillRow 의 필드명이다. 이 세 개만 강화 대상이다.
	static const TArray<FName> KnownParameterFields = {
		TEXT("Skill_Multiplier"),
		TEXT("Range_X"),
		TEXT("Skill_Cooldown")
	};
	return KnownParameterFields;
}

void BuildIndex(
	FLSSkillNodeIndex& OutIndex,
	const UDataTable* CoreTable,
	const UDataTable* MainStatTable,
	const UDataTable* SubStatTable,
	const UDataTable* EnhanceTable,
	const UDataTable* EvolveTable)
{
	OutIndex.Reset();

	SkillNodeCollectRows<FLSSkillNodeCoreRow>(OutIndex, CoreTable, ELSSkillNodeKind::Core);
	SkillNodeCollectRows<FLSSkillNodeStatRow>(OutIndex, MainStatTable, ELSSkillNodeKind::MainStat);
	SkillNodeCollectRows<FLSSkillNodeStatRow>(OutIndex, SubStatTable, ELSSkillNodeKind::SubStat);
	SkillNodeCollectRows<FLSSkillNodeEnhanceRow>(OutIndex, EnhanceTable, ELSSkillNodeKind::SkillEnhance);
	SkillNodeCollectRows<FLSSkillNodeEvolveRow>(OutIndex, EvolveTable, ELSSkillNodeKind::SkillEvolve);

	// 표시 순서를 안정화한다. GetRowMap 순회 순서는 보장되지 않는다.
	for (TPair<int32, TArray<FName>>& Pair : OutIndex.NodeKeysByCharacter)
	{
		Pair.Value.Sort([&OutIndex](const FName Left, const FName Right)
		{
			const FLSSkillNodeRef* LeftRef = OutIndex.Nodes.Find(Left);
			const FLSSkillNodeRef* RightRef = OutIndex.Nodes.Find(Right);
			if (LeftRef && RightRef && LeftRef->Ring != RightRef->Ring)
			{
				return LeftRef->Ring < RightRef->Ring;
			}
			return Left.LexicalLess(Right);
		});
	}
}

#if WITH_EDITOR
int32 ValidateGraph(const FLSSkillNodeIndex& Index)
{
	int32 ProblemCount = 0;

	// 선행 관계 검사 3종을 한 번에 돈다.
	for (const TPair<FName, FLSSkillNodeRef>& Pair : Index.Nodes)
	{
		const FLSSkillNodeRef& Node = Pair.Value;
		for (const FName Prereq : { Node.Prerequisite_1, Node.Prerequisite_2 })
		{
			if (Prereq.IsNone())
			{
				continue;
			}

			const FLSSkillNodeRef* PrereqNode = Index.Find(Prereq);
			if (!PrereqNode)
			{
				UE_LOG(LogLS, Warning, TEXT("[SkillNode] %s의 선행 '%s'가 인덱스에 없다"),
					*Node.NodeKey.ToString(), *Prereq.ToString());
				++ProblemCount;
				continue;
			}

			if (PrereqNode->CharacterID != Node.CharacterID)
			{
				UE_LOG(LogLS, Warning, TEXT("[SkillNode] %s(캐릭터 %d)가 다른 캐릭터의 노드 %s(캐릭터 %d)를 선행으로 갖는다"),
					*Node.NodeKey.ToString(), Node.CharacterID, *Prereq.ToString(), PrereqNode->CharacterID);
				++ProblemCount;
			}

			// 동일 링은 정상이다. 1차 링의 메인<->서브 분기와 2차 링의 서브->메인 머지가 같은 링 안에서 일어난다.
			// 여기를 >= 로 쓰면 정상 데이터 120건이 경고로 뜬다.
			if (PrereqNode->Ring > Node.Ring)
			{
				UE_LOG(LogLS, Warning, TEXT("[SkillNode] %s(링 %d)의 선행 %s가 더 바깥 링(%d)이다"),
					*Node.NodeKey.ToString(), Node.Ring, *Prereq.ToString(), PrereqNode->Ring);
				++ProblemCount;
			}
		}
	}

	// 사이클 검사 — 선행 방향(노드 -> 선행)으로만 따라간다.
	// 무향으로 돌면 1차 링이 랩어라운드 때문에 닫힌 고리로 보여 정상 데이터가 사이클로 잡힌다.
	enum class EVisitState : uint8 { Unvisited, InStack, Done };
	TMap<FName, EVisitState> VisitStates;
	VisitStates.Reserve(Index.Nodes.Num());

	for (const TPair<FName, FLSSkillNodeRef>& Pair : Index.Nodes)
	{
		if (VisitStates.FindRef(Pair.Key) == EVisitState::Done)
		{
			continue;
		}

		// 반복 DFS. Value 가 false 면 진입, true 면 후처리(스택에서 뺀다).
		TArray<TPair<FName, bool>> Stack;
		Stack.Push({ Pair.Key, false });
		while (Stack.Num() > 0)
		{
			const TPair<FName, bool> Current = Stack.Pop();
			if (Current.Value)
			{
				VisitStates.Add(Current.Key, EVisitState::Done);
				continue;
			}

			const EVisitState State = VisitStates.FindRef(Current.Key);
			if (State == EVisitState::Done)
			{
				continue;
			}
			if (State == EVisitState::InStack)
			{
				UE_LOG(LogLS, Warning, TEXT("[SkillNode] 선행 관계에 순환이 있다 - %s"), *Current.Key.ToString());
				++ProblemCount;
				continue;
			}

			VisitStates.Add(Current.Key, EVisitState::InStack);
			Stack.Push({ Current.Key, true });

			if (const FLSSkillNodeRef* Node = Index.Find(Current.Key))
			{
				for (const FName Prereq : { Node->Prerequisite_1, Node->Prerequisite_2 })
				{
					if (!Prereq.IsNone() && Index.Find(Prereq))
					{
						Stack.Push({ Prereq, false });
					}
				}
			}
		}
	}

	// 섬 검사 — 코어에서 선행 역방향으로 내려간다. 여기서도 무향 순회를 쓰지 않는다.
	TMultiMap<FName, FName> ChildrenByPrereq;
	for (const TPair<FName, FLSSkillNodeRef>& Pair : Index.Nodes)
	{
		for (const FName Prereq : { Pair.Value.Prerequisite_1, Pair.Value.Prerequisite_2 })
		{
			if (!Prereq.IsNone())
			{
				ChildrenByPrereq.Add(Prereq, Pair.Key);
			}
		}
	}

	TSet<FName> Reachable;
	TArray<FName> Frontier;
	for (const TPair<FName, FLSSkillNodeRef>& Pair : Index.Nodes)
	{
		if (Pair.Value.Kind == ELSSkillNodeKind::Core)
		{
			Reachable.Add(Pair.Key);
			Frontier.Add(Pair.Key);
		}
	}

	TArray<FName> Children;
	while (Frontier.Num() > 0)
	{
		const FName Current = Frontier.Pop();
		Children.Reset();
		ChildrenByPrereq.MultiFind(Current, Children);
		for (const FName Child : Children)
		{
			if (!Reachable.Contains(Child))
			{
				Reachable.Add(Child);
				Frontier.Add(Child);
			}
		}
	}

	for (const TPair<FName, FLSSkillNodeRef>& Pair : Index.Nodes)
	{
		if (!Reachable.Contains(Pair.Key))
		{
			UE_LOG(LogLS, Warning, TEXT("[SkillNode] %s는 코어에서 도달할 수 없다(섬)"), *Pair.Key.ToString());
			++ProblemCount;
		}
	}

	return ProblemCount;
}
#endif // WITH_EDITOR
} // namespace LSSkillNodes
