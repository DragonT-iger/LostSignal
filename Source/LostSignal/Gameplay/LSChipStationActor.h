#pragma once

#include "CoreMinimal.h"
#include "Gameplay/LSInteractableObject.h"
#include "LSChipStationActor.generated.h"

class ULSChipStationWidget;
class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(BlueprintType)
class LOSTSIGNAL_API ALSChipStationActor : public ALSInteractableObject
{
	GENERATED_BODY()

public:
	ALSChipStationActor();

	virtual bool CanInteract_Implementation(APawn* Interactor) override;
	virtual void Interact_Implementation(APawn* Interactor) override;

protected:
	virtual void HandleLocalPawnEndOverlap(APawn* Pawn) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Chip")
	TSubclassOf<ULSChipStationWidget> ChipStationWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Chip|Station")
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Chip|Station")
	TObjectPtr<UTextRenderComponent> MarkerText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Chip|Station")
	TObjectPtr<UPointLightComponent> MarkerLight;
};
