#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LSAIController.generated.h"

class UStateTreeAIComponent;

/** AI controller that hosts the monster StateTree on the server. 실행할 StateTree 에셋은 빙의한 ALSEnemyCharacter의 DefaultStateTree에서 읽는다. */
UCLASS()
class LOSTSIGNAL_API ALSAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALSAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	void TryStartStateTreeLogic();

	UFUNCTION(BlueprintPure, Category="LS/AI|StateTree")
	UStateTreeAIComponent* GetStateTreeAIComponent() const { return StateTreeComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/AI|StateTree")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;
};
