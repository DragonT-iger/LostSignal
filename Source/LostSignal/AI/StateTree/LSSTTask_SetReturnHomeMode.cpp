#include "AI/StateTree/LSSTTask_SetReturnHomeMode.h"

#include "AI/LSMonsterSenseComponent.h"
#include "AIController.h"
#include "Characters/LSEnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "StateTreeExecutionContext.h"
#include "LostSignal.h"

FLSSTTask_SetReturnHomeMode::FLSSTTask_SetReturnHomeMode()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FLSSTTask_SetReturnHomeMode::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.EnemyCharacter && InstanceData.AIController)
	{
		InstanceData.EnemyCharacter = Cast<ALSEnemyCharacter>(InstanceData.AIController->GetPawn());
	}

	if (!InstanceData.SenseComponent && InstanceData.EnemyCharacter)
	{
		InstanceData.SenseComponent = InstanceData.EnemyCharacter->FindComponentByClass<ULSMonsterSenseComponent>();
	}

	if (!InstanceData.EnemyCharacter || !InstanceData.SenseComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed To Enter ReturnHome State"))
		return EStateTreeRunStatus::Failed;
	}

	if (InstanceData.bClearFocusOnEnter && InstanceData.AIController)
	{
		InstanceData.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	if (InstanceData.bClearStaleInterestOnEnter)
	{
		InstanceData.SenseComponent->ClearInterest();
	}

	InstanceData.SenseComponent->SetReturnHomeMode(true);
	InstanceData.SenseComponent->SetForceMaxSightRadius(true);

	if (UCharacterMovementComponent* MovementComponent = InstanceData.EnemyCharacter->GetCharacterMovement())
	{
		InstanceData.PreviousMaxWalkSpeed = MovementComponent->MaxWalkSpeed;
		MovementComponent->MaxWalkSpeed = InstanceData.PreviousMaxWalkSpeed * FMath::Max(0.0f, InstanceData.AlertMoveSpeedMultiplier);
	}

	return EStateTreeRunStatus::Running;
}

void FLSSTTask_SetReturnHomeMode::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UE_LOG(LogLS, Warning, TEXT("Exit ReturnHome"))

	if (InstanceData.SenseComponent)
	{
		InstanceData.SenseComponent->SetReturnHomeMode(false);
		InstanceData.SenseComponent->SetForceMaxSightRadius(false);
	}

	if (InstanceData.EnemyCharacter && InstanceData.PreviousMaxWalkSpeed > 0.0f)
	{
		if (UCharacterMovementComponent* MovementComponent = InstanceData.EnemyCharacter->GetCharacterMovement())
		{
			MovementComponent->MaxWalkSpeed = InstanceData.PreviousMaxWalkSpeed;
		}
	}
}
