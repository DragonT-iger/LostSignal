#include "AI/LSAIController.h"

#include "Characters/Enemys/LSEnemyCharacter.h"
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
	if (!HasAuthority() || !GetPawn())
	{
		return;
	}

	if (!StateTreeComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("StateTreeComponent Init Faild"));
		return;
	}

	const ALSEnemyCharacter* EnemyCharacter = Cast<ALSEnemyCharacter>(GetPawn());
	UStateTree* PawnStateTree = EnemyCharacter ? EnemyCharacter->GetDefaultStateTree() : nullptr;
	if (!PawnStateTree)
	{
		UE_LOG(LogLS, Warning, TEXT("[%s] DefaultStateTree 미할당 — 몬스터 BP에서 매핑 필요"), *GetPawn()->GetName());
		return;
	}

	if (StateTreeComponent->IsRunning())
	{
		return;
	}

	StateTreeComponent->SetStateTree(PawnStateTree);
	StateTreeComponent->StartLogic();
}
