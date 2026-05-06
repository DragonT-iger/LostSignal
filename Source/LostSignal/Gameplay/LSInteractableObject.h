#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/LSInteractable.h"
#include "LSInteractableObject.generated.h"

class USphereComponent;

UCLASS(Abstract, BlueprintType)
class LOSTSIGNAL_API ALSInteractableObject : public AActor, public ILSInteractable
{
	GENERATED_BODY()

public:
	ALSInteractableObject();

	virtual bool CanInteract_Implementation(APawn* Interactor) override;
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual FText GetInteractText_Implementation() override;

protected:
	// 에디터에서 상호작용 범위를 시각적으로 확인하기 위한 구체
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Interact")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact")
	FText InteractText;
};
