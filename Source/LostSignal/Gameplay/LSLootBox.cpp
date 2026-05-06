#include "Gameplay/LSLootBox.h"
#include "LostSignal.h"
#include "Net/UnrealNetwork.h"
#include "Session/LSSessionSubsystem.h"

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
	RefreshWidgetVisibility();

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	ULSDropSubsystem* DropSubsystem = GI->GetSubsystem<ULSDropSubsystem>();
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

	// 드랍 결과를 세션 인벤토리에 등록 (탈출 시 보관 처리됨)
	ULSSessionSubsystem* Session = GI->GetSubsystem<ULSSessionSubsystem>();
	if (Session)
	{
		for (const FLSDropResult& Result : Results)
		{
			Session->AddSessionItem(Result.ItemRowName, Result.Amount);
		}
	}

	OnLootResultReceived(Results);
}

void ALSLootBox::OnRep_IsOpened()
{
	RefreshWidgetVisibility();
}
