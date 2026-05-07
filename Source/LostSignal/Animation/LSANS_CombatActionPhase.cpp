#include "Animation/LSANS_CombatActionPhase.h"

#include "Combat/LSCombatStateComponent.h"
#include "Components/SkeletalMeshComponent.h"

void ULSANS_CombatActionPhase::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!OwnerActor)
	{
		return;
	}

	if (ULSCombatStateComponent* CombatStateComponent = OwnerActor->FindComponentByClass<ULSCombatStateComponent>())
	{
		CombatStateComponent->SetActionPhase(Phase);
	}
}

FString ULSANS_CombatActionPhase::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("LS Combat Phase: %s"), *UEnum::GetValueAsString(Phase));
}
