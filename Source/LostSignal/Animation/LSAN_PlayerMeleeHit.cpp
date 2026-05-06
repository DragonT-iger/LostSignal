#include "Animation/LSAN_PlayerMeleeHit.h"

#include "Combat/LSPlayerCombatComponent.h"
#include "Components/SkeletalMeshComponent.h"

void ULSAN_PlayerMeleeHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	if (ULSPlayerCombatComponent* CombatComponent = OwnerActor->FindComponentByClass<ULSPlayerCombatComponent>())
	{
		CombatComponent->PerformMeleeHit();
	}
}
