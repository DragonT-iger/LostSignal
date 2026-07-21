#include "Animation/LSAN_MonsterActionChargeStart.h"

#include "AI/LSMonsterCombatComponent.h"
#include "Characters/Enemys/LSEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void ULSAN_MonsterActionChargeStart::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	ALSEnemyCharacter* EnemyCharacter = MeshComp ? Cast<ALSEnemyCharacter>(MeshComp->GetOwner()) : nullptr;
	if (!EnemyCharacter)
	{
		return;
	}

	if (ULSMonsterCombatComponent* CombatComponent = EnemyCharacter->GetMonsterCombatComponent())
	{
		CombatComponent->BeginActionCharge();
	}
}

FString ULSAN_MonsterActionChargeStart::GetNotifyName_Implementation() const
{
	return TEXT("LS Monster Action Charge Start");
}
