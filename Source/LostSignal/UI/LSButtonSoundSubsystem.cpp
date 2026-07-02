#include "UI/LSButtonSoundSubsystem.h"

#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Data/LSAudioSettings.h"
#include "Sound/SoundBase.h"

void ULSButtonSoundSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get())
	{
		WidgetAddedHandle = ViewportSubsystem->OnWidgetAdded.AddUObject(this, &ULSButtonSoundSubsystem::HandleWidgetAdded);
	}
}

void ULSButtonSoundSubsystem::Deinitialize()
{
	if (UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get())
	{
		ViewportSubsystem->OnWidgetAdded.Remove(WidgetAddedHandle);
	}
	Super::Deinitialize();
}

void ULSButtonSoundSubsystem::HandleWidgetAdded(UWidget* Widget, ULocalPlayer* Player)
{
	// PIE 다중 인스턴스 등 다른 GameInstance의 위젯에는 반응하지 않는다.
	UUserWidget* UserWidget = Cast<UUserWidget>(Widget);
	if (!UserWidget || UserWidget->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	ApplyButtonSoundsRecursive(*UserWidget);
}

void ULSButtonSoundSubsystem::ApplyButtonSoundsRecursive(UUserWidget& RootWidget) const
{
	if (!RootWidget.WidgetTree)
	{
		return;
	}

	const ULSAudioSettings* AudioSettings = GetDefault<ULSAudioSettings>();
	USoundBase* PressedSound = AudioSettings ? AudioSettings->ButtonPressedSound.LoadSynchronous() : nullptr;
	USoundBase* HoveredSound = AudioSettings ? AudioSettings->ButtonHoveredSound.LoadSynchronous() : nullptr;
	if (!PressedSound && !HoveredSound)
	{
		return;
	}

	RootWidget.WidgetTree->ForEachWidget([this, PressedSound, HoveredSound](UWidget* Widget)
	{
		if (UUserWidget* ChildUserWidget = Cast<UUserWidget>(Widget))
		{
			ApplyButtonSoundsRecursive(*ChildUserWidget);
			return;
		}

		UButton* Button = Cast<UButton>(Widget);
		if (!Button)
		{
			return;
		}

		FButtonStyle Style = Button->GetStyle();
		bool bChanged = false;
		if (PressedSound && !Style.PressedSlateSound.GetResourceObject())
		{
			Style.PressedSlateSound.SetResourceObject(PressedSound);
			bChanged = true;
		}
		if (HoveredSound && !Style.HoveredSlateSound.GetResourceObject())
		{
			Style.HoveredSlateSound.SetResourceObject(HoveredSound);
			bChanged = true;
		}
		if (bChanged)
		{
			Button->SetStyle(Style);
		}
	});
}
