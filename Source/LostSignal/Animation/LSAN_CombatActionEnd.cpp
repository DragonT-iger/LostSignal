#include "Animation/LSAN_CombatActionEnd.h"

#include "Combat/LSCombatStateComponent.h"
#include "Combat/LSPlayerCombatComponent.h"
#include "Components/SkeletalMeshComponent.h"

void ULSAN_CombatActionEnd::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!OwnerActor)
	{
		return;
	}

	if (ULSPlayerCombatComponent* PlayerCombatComponent = OwnerActor->FindComponentByClass<ULSPlayerCombatComponent>())
	{
		PlayerCombatComponent->HandleCombatActionEnd(ExpectedState);
		return;
	}

	if (ULSCombatStateComponent* CombatStateComponent = OwnerActor->FindComponentByClass<ULSCombatStateComponent>())
	{
		CombatStateComponent->EndActionIfCurrent(ExpectedState);
	}
}

FString ULSAN_CombatActionEnd::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("LS Combat End: %s"), *UEnum::GetValueAsString(ExpectedState));
}
