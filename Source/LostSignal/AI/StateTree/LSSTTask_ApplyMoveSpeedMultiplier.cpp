#include "AI/StateTree/LSSTTask_ApplyMoveSpeedMultiplier.h"

#include "AIController.h"
#include "Characters/LSEnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "StateTreeExecutionContext.h"
#include "LostSignal.h"

FLSSTTask_ApplyMoveSpeedMultiplier::FLSSTTask_ApplyMoveSpeedMultiplier()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FLSSTTask_ApplyMoveSpeedMultiplier::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// 에셋 AIController 바인딩 누락 대비 컨텍스트 소유자에서 해석.
	if (!InstanceData.AIController)
	{
		InstanceData.AIController = Cast<AAIController>(Context.GetOwner());
	}

	if (!InstanceData.EnemyCharacter && InstanceData.AIController)
	{
		InstanceData.EnemyCharacter = Cast<ALSEnemyCharacter>(InstanceData.AIController->GetPawn());
	}

	if (!InstanceData.EnemyCharacter)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed To Apply Move Speed Multiplier"));
		return EStateTreeRunStatus::Failed;
	}

	const float BaseMaxWalkSpeed = InstanceData.EnemyCharacter->GetDefaultMaxWalkSpeed();
	if (UCharacterMovementComponent* MovementComponent = InstanceData.EnemyCharacter->GetCharacterMovement())
	{
		if (BaseMaxWalkSpeed > 0.0f)
		{
			MovementComponent->MaxWalkSpeed = BaseMaxWalkSpeed * FMath::Max(0.0f, InstanceData.MoveSpeedMultiplier);
		}
	}

	return EStateTreeRunStatus::Running;
}

void FLSSTTask_ApplyMoveSpeedMultiplier::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.EnemyCharacter)
	{
		const float BaseMaxWalkSpeed = InstanceData.EnemyCharacter->GetDefaultMaxWalkSpeed();
		if (BaseMaxWalkSpeed > 0.0f)
		{
			if (UCharacterMovementComponent* MovementComponent = InstanceData.EnemyCharacter->GetCharacterMovement())
			{
				MovementComponent->MaxWalkSpeed = BaseMaxWalkSpeed;
			}
		}
	}
}
