#include "Animation/LSAN_MonsterActionDash.h"

#include "AI/LSMonsterCombatComponent.h"
#include "Characters/LSEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void ULSAN_MonsterActionDash::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	ALSEnemyCharacter* EnemyCharacter = MeshComp ? Cast<ALSEnemyCharacter>(MeshComp->GetOwner()) : nullptr;
	if (!EnemyCharacter)
	{
		return;
	}

	if (ULSMonsterCombatComponent* CombatComponent = EnemyCharacter->GetMonsterCombatComponent())
	{
		CombatComponent->PerformActionDash();
	}
}

FString ULSAN_MonsterActionDash::GetNotifyName_Implementation() const
{
	return TEXT("LS Monster Action Dash");
}
