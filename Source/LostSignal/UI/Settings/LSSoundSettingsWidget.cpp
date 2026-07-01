#include "UI/Settings/LSSoundSettingsWidget.h"

#include "Components/Button.h"
#include "LostSignal.h"
#include "UI/Settings/LSSoundRowWidget.h"

void ULSSoundSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &ULSSoundSettingsWidget::HandleBackClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("BackButton is not bound on %s."), *GetNameSafe(this));
	}

	if (MasterRow)
	{
		MasterRow->InitializeRow(ELSSoundBus::Master);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("MasterRow is not bound on %s."), *GetNameSafe(this));
	}

	if (BGMRow)
	{
		BGMRow->InitializeRow(ELSSoundBus::BGM);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("BGMRow is not bound on %s."), *GetNameSafe(this));
	}

	if (SFXRow)
	{
		SFXRow->InitializeRow(ELSSoundBus::SFX);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("SFXRow is not bound on %s."), *GetNameSafe(this));
	}

	// ESC로 스스로 닫을 수 있도록 키보드 포커스를 가져온다. (뒤에 숨은 세팅 화면이 ESC를 가로채지 않게 함)
	SetIsFocusable(true);
	SetKeyboardFocus();
}

void ULSSoundSettingsWidget::NativeDestruct()
{
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &ULSSoundSettingsWidget::HandleBackClicked);
	}

	Super::NativeDestruct();
}

void ULSSoundSettingsWidget::HandleBackClicked()
{
	CloseSound();
}

FReply ULSSoundSettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		CloseSound();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULSSoundSettingsWidget::CloseSound()
{
	// 뒤에 숨겨둔 세팅 화면이 다시 보이도록 알린 뒤 스스로 닫는다.
	OnClosed.Broadcast();
	RemoveFromParent();
}
