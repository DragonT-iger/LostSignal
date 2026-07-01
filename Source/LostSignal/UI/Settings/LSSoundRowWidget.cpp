#include "UI/Settings/LSSoundRowWidget.h"

#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "LostSignal.h"

void ULSSoundRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Slider)
	{
		Slider->OnValueChanged.AddDynamic(this, &ULSSoundRowWidget::HandleSliderValueChanged);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("Slider is not bound on %s."), *GetNameSafe(this));
	}

	if (MinusButton)
	{
		MinusButton->OnClicked.AddDynamic(this, &ULSSoundRowWidget::HandleMinusClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("MinusButton is not bound on %s."), *GetNameSafe(this));
	}

	if (PlusButton)
	{
		PlusButton->OnClicked.AddDynamic(this, &ULSSoundRowWidget::HandlePlusClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("PlusButton is not bound on %s."), *GetNameSafe(this));
	}

	if (!ProgressBar)
	{
		UE_LOG(LogLS, Warning, TEXT("ProgressBar is not bound on %s."), *GetNameSafe(this));
	}

	if (!ValueText)
	{
		UE_LOG(LogLS, Warning, TEXT("ValueText is not bound on %s."), *GetNameSafe(this));
	}
}

void ULSSoundRowWidget::NativeDestruct()
{
	if (Slider)
	{
		Slider->OnValueChanged.RemoveDynamic(this, &ULSSoundRowWidget::HandleSliderValueChanged);
	}
	if (MinusButton)
	{
		MinusButton->OnClicked.RemoveDynamic(this, &ULSSoundRowWidget::HandleMinusClicked);
	}
	if (PlusButton)
	{
		PlusButton->OnClicked.RemoveDynamic(this, &ULSSoundRowWidget::HandlePlusClicked);
	}

	Super::NativeDestruct();
}

void ULSSoundRowWidget::InitializeRow(ELSSoundBus InSoundBus)
{
	SoundBus = InSoundBus;

	const ULSAudioSettingsSubsystem* Settings = GetAudioSettingsSubsystem();
	const float InitialVolume = Settings ? Settings->GetVolume(SoundBus) : 1.0f;
	ApplyVolume(InitialVolume, /*bPersist=*/false);
}

void ULSSoundRowWidget::HandleSliderValueChanged(float NewValue)
{
	ApplyVolume(NewValue, /*bPersist=*/true);
}

void ULSSoundRowWidget::HandleMinusClicked()
{
	const float CurrentValue = Slider ? Slider->GetValue() : 0.0f;
	ApplyVolume(CurrentValue - StepSize, /*bPersist=*/true);
}

void ULSSoundRowWidget::HandlePlusClicked()
{
	const float CurrentValue = Slider ? Slider->GetValue() : 0.0f;
	ApplyVolume(CurrentValue + StepSize, /*bPersist=*/true);
}

void ULSSoundRowWidget::ApplyVolume(float NewValue, bool bPersist)
{
	const float Clamped = FMath::Clamp(NewValue, 0.0f, 1.0f);

	if (Slider)
	{
		Slider->SetValue(Clamped);
	}
	if (ProgressBar)
	{
		ProgressBar->SetPercent(Clamped);
	}
	if (ValueText)
	{
		ValueText->SetText(FText::AsNumber(FMath::RoundToInt(Clamped * 100.0f)));
	}

	if (bPersist)
	{
		if (ULSAudioSettingsSubsystem* Settings = GetAudioSettingsSubsystem())
		{
			Settings->SetVolume(SoundBus, Clamped);
		}
	}
}

ULSAudioSettingsSubsystem* ULSSoundRowWidget::GetAudioSettingsSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}
	return GameInstance->GetSubsystem<ULSAudioSettingsSubsystem>();
}
