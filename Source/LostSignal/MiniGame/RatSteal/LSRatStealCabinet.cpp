#include "MiniGame/RatSteal/LSRatStealCabinet.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MiniGame/RatSteal/LSRatStealSubsystem.h"

#define LOCTEXT_NAMESPACE "RatSteal"

ALSRatStealCabinet::ALSRatStealCabinet()
{
	PrimaryActorTick.bCanEverTick = false;

	CabinetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CabinetMesh"));
	SetRootComponent(CabinetMesh);

	InteractBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractBox"));
	InteractBox->SetupAttachment(CabinetMesh);
	InteractBox->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	InteractBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	InteractText = LOCTEXT("CabinetInteract", "몰래몰래팜 플레이");
}

bool ALSRatStealCabinet::CanInteract_Implementation(APawn* Interactor)
{
	return !MiniGameLevel.IsNull();
}

void ALSRatStealCabinet::Interact_Implementation(APawn* Interactor)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULSRatStealSubsystem* Subsystem = GI->GetSubsystem<ULSRatStealSubsystem>())
		{
			Subsystem->EnterMiniGame(MiniGameLevel, Interactor);
		}
	}
}

FText ALSRatStealCabinet::GetInteractText_Implementation()
{
	return InteractText;
}

#undef LOCTEXT_NAMESPACE
