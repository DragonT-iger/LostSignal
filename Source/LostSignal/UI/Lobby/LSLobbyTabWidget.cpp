#include "UI/Lobby/LSLobbyTabWidget.h"

#include "Components/Button.h"
#include "LostSignal.h"

void ULSLobbyTabWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button)
	{
		Button->OnClicked.AddDynamic(this, &ULSLobbyTabWidget::HandleButtonClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("Button is not bound on %s."), *GetNameSafe(this));
	}
}

void ULSLobbyTabWidget::NativeDestruct()
{
	if (Button)
	{
		Button->OnClicked.RemoveDynamic(this, &ULSLobbyTabWidget::HandleButtonClicked);
	}

	Super::NativeDestruct();
}

void ULSLobbyTabWidget::HandleButtonClicked()
{
	OnClicked.Broadcast();
}
