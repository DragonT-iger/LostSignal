#include "AI/StateTree/LSSTTask_DormantWait.h"

#include "AI/LSAIController.h"
#include "AIController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "GameFramework/Character.h"
#include "StateTreeExecutionContext.h"

namespace
{
	// 휴면 중 애니 정지/재개. 렌더 기반 판정(VisibilityBasedAnimTickOption)은 RT/VSM 환경에서
	// 섀도우·RT 씬 렌더로 LastRenderTime이 계속 갱신돼 무력화되므로 휴면 상태에 직접 묶는다.
	void SetDormantPawnAnimPaused(const AAIController* AIController, bool bPaused)
	{
		const ACharacter* PawnCharacter = AIController ? Cast<ACharacter>(AIController->GetPawn()) : nullptr;
		if (USkeletalMeshComponent* MeshComponent = PawnCharacter ? PawnCharacter->GetMesh() : nullptr)
		{
			MeshComponent->bPauseAnims = bPaused;
		}
	}
}

EStateTreeRunStatus FLSSTTask_DormantWait::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.AIController)
	{
		InstanceData.AIController = Cast<AAIController>(Context.GetOwner());
	}

	if (InstanceData.AIController)
	{
		InstanceData.AIController->StopMovement();
		InstanceData.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	// 휴면 중에는 브레인 틱을 늦춰 evaluator 갱신 비용을 줄인다(웨이크 지연 상한 = DormantTickInterval).
	if (const ALSAIController* LSAIController = Cast<ALSAIController>(InstanceData.AIController))
	{
		if (UStateTreeAIComponent* BrainComponent = LSAIController->GetStateTreeAIComponent())
		{
			InstanceData.PreviousTickInterval = BrainComponent->PrimaryComponentTick.TickInterval;
			BrainComponent->SetComponentTickInterval(FMath::Max(0.0f, InstanceData.DormantTickInterval));
		}
	}

	SetDormantPawnAnimPaused(InstanceData.AIController, true);

	return EStateTreeRunStatus::Running;
}

void FLSSTTask_DormantWait::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (const ALSAIController* LSAIController = Cast<ALSAIController>(InstanceData.AIController))
	{
		if (UStateTreeAIComponent* BrainComponent = LSAIController->GetStateTreeAIComponent())
		{
			BrainComponent->SetComponentTickInterval(InstanceData.PreviousTickInterval);
		}
	}

	SetDormantPawnAnimPaused(InstanceData.AIController, false);
}
