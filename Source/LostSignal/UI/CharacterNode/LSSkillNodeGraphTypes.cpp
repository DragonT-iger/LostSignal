#include "UI/CharacterNode/LSSkillNodeGraphTypes.h"

#include "Data/LSSkillNodeIndex.h"

namespace LSSkillNodeViews
{
void BuildViews(
	const TArray<const FLSSkillNodeRef*>& Nodes,
	const TSet<FName>& ActivatedNodeKeys,
	TArray<FLSSkillNodeView>& OutViews)
{
	OutViews.Reset();
	OutViews.Reserve(Nodes.Num());

	for (const FLSSkillNodeRef* Node : Nodes)
	{
		if (!Node)
		{
			continue;
		}

		FLSSkillNodeView View;
		View.NodeKey = Node->NodeKey;
		View.Kind = Node->Kind;
		View.Ring = Node->Ring;
		View.DisplayName = Node->NodeName;
		View.Prerequisite_1 = Node->Prerequisite_1;
		View.Prerequisite_2 = Node->Prerequisite_2;
		View.ChipGrade = Node->ChipGrade;
		View.RequiredQuantity = Node->RequiredQuantity;
		View.CoinCost = Node->CoinCost;

		if (ActivatedNodeKeys.Contains(Node->NodeKey))
		{
			View.State = ELSSkillNodeState::Activated;
		}
		else
		{
			// ANY 조건. 선행 둘 중 하나만 활성이면 열린다.
			const bool bPrerequisiteMet =
				!Node->HasPrerequisite()
				|| ActivatedNodeKeys.Contains(Node->Prerequisite_1)
				|| ActivatedNodeKeys.Contains(Node->Prerequisite_2);
			View.State = bPrerequisiteMet ? ELSSkillNodeState::Available : ELSSkillNodeState::Locked;
		}

		OutViews.Add(MoveTemp(View));
	}
}

void CollectAutoActivatedNodeKeys(
	const TArray<const FLSSkillNodeRef*>& Nodes,
	TSet<FName>& OutActivatedNodeKeys)
{
	OutActivatedNodeKeys.Reset();
	for (const FLSSkillNodeRef* Node : Nodes)
	{
		if (Node && Node->Kind == ELSSkillNodeKind::Core)
		{
			OutActivatedNodeKeys.Add(Node->NodeKey);
		}
	}
}
} // namespace LSSkillNodeViews
