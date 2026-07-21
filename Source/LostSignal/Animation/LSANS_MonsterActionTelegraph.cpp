#include "Animation/LSANS_MonsterActionTelegraph.h"

#include "AI/LSMonsterCombatComponent.h"
#include "Characters/Enemys/LSEnemyCharacter.h"
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
		// TotalDuration(윈드업 윈도우)이 fill 차오름 기준 시간.
		CombatComponent->BeginActionTelegraph(TotalDuration, OriginMode);
	}
}

void ULSANS_MonsterActionTelegraph::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (ULSMonsterCombatComponent* CombatComponent = ResolveMonsterCombat(MeshComp))
	{
		CombatComponent->UpdateActionTelegraphFill(FrameDeltaTime);
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
