#include "Gameplay/LSInteractableObject.h"
#include "Components/SphereComponent.h"

ALSInteractableObject::ALSInteractableObject()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetSphereRadius(200.f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(InteractionSphere);
}

bool ALSInteractableObject::CanInteract_Implementation(APawn* Interactor)
{
	return true;
}

void ALSInteractableObject::Interact_Implementation(APawn* Interactor)
{
}

FText ALSInteractableObject::GetInteractText_Implementation()
{
	return InteractText;
}
