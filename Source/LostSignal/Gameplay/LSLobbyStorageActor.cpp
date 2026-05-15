#include "Gameplay/LSLobbyStorageActor.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/LSPlayerControllerBase.h"
#include "Engine/StaticMesh.h"
#include "LostSignal.h"
#include "UI/Storage/LSLobbyStorageWidget.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LSLobbyStorageActor"

ALSLobbyStorageActor::ALSLobbyStorageActor()
{
	InteractText = LOCTEXT("LobbyStorageInteractText", "창고");

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	MarkerMesh->SetupAttachment(RootComponent);
	MarkerMesh->SetRelativeLocation(FVector(0.f, 0.f, -190.f));
	MarkerMesh->SetRelativeScale3D(FVector(2.8f, 2.8f, 0.12f));
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMesh->SetGenerateOverlapEvents(false);
	MarkerMesh->SetCustomDepthStencilValue(253);
	MarkerMesh->SetRenderCustomDepth(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		MarkerMesh->SetStaticMesh(CylinderMesh.Object);
	}

	MarkerText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MarkerText"));
	MarkerText->SetupAttachment(RootComponent);
	MarkerText->SetRelativeLocation(FVector(0.f, 0.f, 260.f));
	MarkerText->SetRelativeRotation(FRotator(60.f, 180.f, 0.f));
	MarkerText->SetRelativeScale3D(FVector(3.f, 3.f, 3.f));
	MarkerText->SetHorizontalAlignment(EHTA_Center);
	MarkerText->SetVerticalAlignment(EVRTA_TextCenter);
	MarkerText->SetText(LOCTEXT("LobbyStorageMarkerText", "STORAGE"));
	MarkerText->SetTextRenderColor(FColor(255, 205, 80));
	MarkerText->SetWorldSize(56.f);
	MarkerText->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MarkerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MarkerLight"));
	MarkerLight->SetupAttachment(RootComponent);
	MarkerLight->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
	MarkerLight->SetLightColor(FLinearColor(1.f, 0.62f, 0.12f));
	MarkerLight->SetIntensity(1200.f);
	MarkerLight->SetAttenuationRadius(450.f);
}

bool ALSLobbyStorageActor::CanInteract_Implementation(APawn* Interactor)
{
	return Interactor != nullptr;
}

void ALSLobbyStorageActor::Interact_Implementation(APawn* Interactor)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!LobbyStorageWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("LobbyStorageWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(Interactor ? Interactor->GetController() : nullptr);
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot show lobby storage because interactor controller is invalid on %s."), *GetNameSafe(this));
		return;
	}

	PlayerController->ShowLobbyStorageWidget(LobbyStorageWidgetClass);
}

#undef LOCTEXT_NAMESPACE
