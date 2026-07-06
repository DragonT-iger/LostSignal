#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "LSSTTask_DormantWait.generated.h"

class AAIController;

USTRUCT()
struct FLSSTTask_DormantWaitInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<AAIController> AIController = nullptr;

	/** 휴면 중 브레인(StateTree 컴포넌트) 틱 간격. 웨이크 반응이 최대 이 간격만큼 지연. */
	UPROPERTY(EditAnywhere, Category="LS/AI", meta=(ClampMin="0.0"))
	float DormantTickInterval = 0.5f;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/AI")
	float PreviousTickInterval = 0.0f;
};

/** 휴면 상태 대기: 이동/포커스 정지 + StateTree 틱을 DormantTickInterval로 스로틀(이탈 시 복원). */
USTRUCT(meta=(DisplayName="LS Dormant Wait", Category="LS|AI|Action"))
struct LOSTSIGNAL_API FLSSTTask_DormantWait : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTTask_DormantWaitInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
