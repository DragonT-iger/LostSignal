#include "Animation/LSAN_MonsterMeleeHit.h"

#include "AI/LSMonsterCombatComponent.h"
#include "Characters/LSEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "LostSignal.h"

void ULSAN_MonsterMeleeHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	ALSEnemyCharacter* EnemyCharacter = MeshComp ? Cast<ALSEnemyCharacter>(MeshComp->GetOwner()) : nullptr;
	if (!EnemyCharacter)
	{
		return;
	}

	if (ULSMonsterCombatComponent* CombatComponent = EnemyCharacter->GetMonsterCombatComponent())
	{
		CombatComponent->PerformMeleeHit();
	}
}

FString ULSAN_MonsterMeleeHit::GetNotifyName_Implementation() const
{
	return TEXT("LS Monster Melee Hit");
}
