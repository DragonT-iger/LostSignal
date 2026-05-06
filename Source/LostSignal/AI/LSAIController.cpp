#include "AI/LSAIController.h"

#include "Blueprint/UserWidget.h"
#include "Characters/LSCharacterBase.h"
#include "Components/StateTreeAIComponent.h"
#include "Engine/World.h"
#include "StateTree.h"
#include "LostSignal.h"
#include "UI/Debug/LSHpDebugWidget.h"

namespace
{
	int32 GNextHpDebugWidgetStackIndex = 0;
}

ALSAIController::ALSAIController()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComponent"));
	StateTreeComponent->SetStartLogicAutomatically(false);
}

void ALSAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (GetNetMode() != NM_DedicatedServer && DebugHpWidgetClass && !DebugHpWidgetInstance)
	{
		DebugHpWidgetStackIndex = GNextHpDebugWidgetStackIndex++;
		DebugHpWidgetInstance = CreateWidget<ULSHpDebugWidget>(GetWorld(), DebugHpWidgetClass);
		if (DebugHpWidgetInstance)
		{
			DebugHpWidgetInstance->SetObservedCharacter(Cast<ALSCharacterBase>(InPawn));
			DebugHpWidgetInstance->AddToViewport();
			DebugHpWidgetInstance->SetPositionInViewport(
				DebugHpWidgetBasePosition + FVector2D(0.0f, DebugHpWidgetVerticalSpacing * DebugHpWidgetStackIndex));
		}
	}
	else if (DebugHpWidgetInstance)
	{
		DebugHpWidgetInstance->SetObservedCharacter(Cast<ALSCharacterBase>(InPawn));
	}

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
	if (DebugHpWidgetInstance)
	{
		DebugHpWidgetInstance->RemoveFromParent();
		DebugHpWidgetInstance = nullptr;
		DebugHpWidgetStackIndex = INDEX_NONE;
	}

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
