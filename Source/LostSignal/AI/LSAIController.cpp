#include "AI/LSAIController.h"

#include "Components/StateTreeAIComponent.h"
#include "Engine/World.h"
#include "StateTree.h"
#include "LostSignal.h"

ALSAIController::ALSAIController()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComponent"));
	StateTreeComponent->SetStartLogicAutomatically(false);
}

void ALSAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	TryStartStateTreeLogic();

#if WITH_GAMEPLAY_DEBUGGER
	const TArray<FName> ActiveStates = StateTreeComponent->GetActiveStateNames();
	for (const FName& DebugStateName : ActiveStates)
	{
		UE_LOG(LogLS, Warning, TEXT("Active State: %s"), *DebugStateName.ToString());
	}
#endif
}

void ALSAIController::OnUnPossess()
{
	if (StateTreeComponent && StateTreeComponent->IsRunning())
	{
		StateTreeComponent->StopLogic(TEXT("Pawn unpossessed"));
	}

	Super::OnUnPossess();
}

void ALSAIController::TryStartStateTreeLogic()
{
	if (!HasAuthority() || !StateTreeComponent || !DefaultStateTree || !GetPawn())
	{
		if(!StateTreeComponent)
			UE_LOG(LogLS, Warning, TEXT("StateTreeComponent Init Faild"));

		if (!DefaultStateTree)
			UE_LOG(LogLS, Warning, TEXT("DefaultStateTree Init Faild"));

		return;
	}

	if (StateTreeComponent->IsRunning())
	{
		return;
	}

	StateTreeComponent->SetStateTree(DefaultStateTree);
	StateTreeComponent->StartLogic();
}
