#include "Gameplay/LSLootBox.h"
#include "LostSignal.h"
#include "Net/UnrealNetwork.h"

void ALSLootBox::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALSLootBox, bIsOpened);
}

bool ALSLootBox::CanInteract_Implementation(APawn* Interactor)
{
	return !bIsOpened;
}

void ALSLootBox::Interact_Implementation(APawn* Interactor)
{
	if (!HasAuthority() || bIsOpened) return;

	bIsOpened = true;

	ULSDropSubsystem* DropSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULSDropSubsystem>()
		: nullptr;

	if (!DropSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("ALSLootBox: DropSubsystem 없음"));
		return;
	}

	if (RootingObjectRowName.IsNone())
	{
		UE_LOG(LogLS, Warning, TEXT("ALSLootBox: RootingObjectRowName 미설정"));
		return;
	}

	const TArray<FLSDropResult> Results = DropSubsystem->OpenRootingObject(RootingObjectRowName);
	OnLootResultReceived(Results);
}
