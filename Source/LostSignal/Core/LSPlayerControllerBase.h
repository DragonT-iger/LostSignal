// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LSPlayerControllerBase.generated.h"

class UInputMappingContext;
class ULSHpDebugWidget;

UCLASS(Abstract)
class LOSTSIGNAL_API ALSPlayerControllerBase : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="LS/UI")
	TSubclassOf<ULSHpDebugWidget> GetDebugHpWidgetClass() const { return DebugHpWidgetClass; }

protected:
	UPROPERTY(EditAnywhere, Category="LS/Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	TSubclassOf<ULSHpDebugWidget> DebugHpWidgetClass;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSHpDebugWidget> DebugHpWidgetInstance;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Input")
	bool bDefaultMappingContextsApplied = false;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
};
