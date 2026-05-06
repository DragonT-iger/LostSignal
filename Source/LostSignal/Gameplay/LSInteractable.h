#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LSInteractable.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class ULSInteractable : public UInterface
{
	GENERATED_BODY()
};

class LOSTSIGNAL_API ILSInteractable
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, Category="LS/Interact")
	bool CanInteract(APawn* Interactor);

	UFUNCTION(BlueprintNativeEvent, Category="LS/Interact")
	void Interact(APawn* Interactor);

	UFUNCTION(BlueprintNativeEvent, Category="LS/Interact")
	FText GetInteractText();
};
