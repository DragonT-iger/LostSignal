#pragma once

#include "CoreMinimal.h"
#include "Gameplay/LSInteractableObject.h"
#include "LSLobbyStorageActor.generated.h"

class ULSLobbyStorageWidget;
class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(BlueprintType)
class LOSTSIGNAL_API ALSLobbyStorageActor : public ALSInteractableObject
{
	GENERATED_BODY()

public:
	ALSLobbyStorageActor();

	virtual bool CanInteract_Implementation(APawn* Interactor) override;
	virtual void Interact_Implementation(APawn* Interactor) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Storage")
	TSubclassOf<ULSLobbyStorageWidget> LobbyStorageWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Lobby|Storage")
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Lobby|Storage")
	TObjectPtr<UTextRenderComponent> MarkerText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Lobby|Storage")
	TObjectPtr<UPointLightComponent> MarkerLight;
};
