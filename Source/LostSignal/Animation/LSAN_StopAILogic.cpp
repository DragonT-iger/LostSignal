#include "Animation/LSAN_StopAILogic.h"

#include "AIController.h"
#include "BrainComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "LostSignal.h"

void ULSAN_StopAILogic::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	AAIController* AIController = Cast<AAIController>(OwnerActor->GetInstigatorController());
	if (!AIController)
	{
		AIController = Cast<AAIController>(OwnerActor->GetOwner());
	}

	if (!AIController)
	{
		if (const APawn* OwnerPawn = Cast<APawn>(OwnerActor))
		{
			AIController = Cast<AAIController>(OwnerPawn->GetController());
		}
	}

	if (!AIController || !AIController->BrainComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("%s StopAILogic notify skipped because AIController or BrainComponent is missing."), *GetNameSafe(OwnerActor));
		return;
	}

	UE_LOG(LogLS, Log, TEXT("%s StopLogic from death montage notify. Reason=%s"), *GetNameSafe(AIController), *Reason);
	AIController->BrainComponent->StopLogic(Reason);
}

FString ULSAN_StopAILogic::GetNotifyName_Implementation() const
{
	return TEXT("LS Stop AI Logic");
}
