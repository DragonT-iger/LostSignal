#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LSAIController.generated.h"

class UStateTree;
class UStateTreeAIComponent;

/** AI controller that hosts the monster StateTree on the server. */
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/AI|StateTree", meta=(RequiredAssetDataTags="Schema=/Script/GameplayStateTreeModule.StateTreeAIComponentSchema"))
	TObjectPtr<UStateTree> DefaultStateTree;
};
