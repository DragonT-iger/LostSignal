#include "Animation/LSAN_MonsterActionHit.h"

#include "AI/LSMonsterCombatComponent.h"
#include "Characters/Enemys/LSEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void ULSAN_MonsterActionHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	ALSEnemyCharacter* EnemyCharacter = MeshComp ? Cast<ALSEnemyCharacter>(MeshComp->GetOwner()) : nullptr;
	if (!EnemyCharacter)
	{
		return;
	}

	if (ULSMonsterCombatComponent* CombatComponent = EnemyCharacter->GetMonsterCombatComponent())
	{
		CombatComponent->PerformActionHit();
	}
}

FString ULSAN_MonsterActionHit::GetNotifyName_Implementation() const
{
	return TEXT("LS Monster Action Hit");
}
