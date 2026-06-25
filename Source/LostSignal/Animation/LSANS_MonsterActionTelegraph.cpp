#include "Animation/LSANS_MonsterActionTelegraph.h"

#include "AI/LSMonsterCombatComponent.h"
#include "Characters/LSEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	ULSMonsterCombatComponent* ResolveMonsterCombat(USkeletalMeshComponent* MeshComp)
	{
		ALSEnemyCharacter* EnemyCharacter = MeshComp ? Cast<ALSEnemyCharacter>(MeshComp->GetOwner()) : nullptr;
		return EnemyCharacter ? EnemyCharacter->GetMonsterCombatComponent() : nullptr;
	}
}

void ULSANS_MonsterActionTelegraph::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (ULSMonsterCombatComponent* CombatComponent = ResolveMonsterCombat(MeshComp))
	{
		CombatComponent->BeginActionTelegraph();
	}
}

void ULSANS_MonsterActionTelegraph::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (ULSMonsterCombatComponent* CombatComponent = ResolveMonsterCombat(MeshComp))
	{
		CombatComponent->EndActionTelegraph();
	}
}

FString ULSANS_MonsterActionTelegraph::GetNotifyName_Implementation() const
{
	return TEXT("LS Monster Action Telegraph");
}
