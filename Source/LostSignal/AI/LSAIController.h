#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LSAIController.generated.h"

class UStateTree;
class UStateTreeAIComponent;
class ULSHpDebugWidget;

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

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	TSubclassOf<ULSHpDebugWidget> DebugHpWidgetClass;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSHpDebugWidget> DebugHpWidgetInstance;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	FVector2D DebugHpWidgetBasePosition = FVector2D(40.0f, 120.0f);

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	float DebugHpWidgetVerticalSpacing = 60.0f;

	int32 DebugHpWidgetStackIndex = INDEX_NONE;
};
