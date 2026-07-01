#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Session/LSAudioSettingsSubsystem.h"
#include "LSSoundRowWidget.generated.h"

class UButton;
class UProgressBar;
class USlider;
class UTextBlock;

// 사운드 세팅 한 줄(WBP_SoundRow)의 부모 클래스. Overlay 안의 Slider(0~1)와 ProgressBar가 값을 함께
// 표시하고, -/+ 버튼으로도 조절할 수 있다. ValueText는 0~100 정수로 환산해 보여준다.
// 어떤 버스(Master/BGM/SFX)를 담당하는지는 부모(WBP_Sound)가 InitializeRow()로 지정해 준다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSSoundRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 이 줄이 담당할 버스를 지정하고, 저장된 값으로 슬라이더/프로그레스바/텍스트를 초기화한다.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Settings")
	void InitializeRow(ELSSoundBus InSoundBus);

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<USlider> Slider;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<UProgressBar> ProgressBar;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<UButton> MinusButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<UButton> PlusButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<UTextBlock> ValueText;

private:
	UFUNCTION()
	void HandleSliderValueChanged(float NewValue);

	UFUNCTION()
	void HandleMinusClicked();

	UFUNCTION()
	void HandlePlusClicked();

	// Slider/ProgressBar/ValueText를 NewValue(0~1)로 맞추고, bPersist면 세팅 서브시스템에도 반영한다.
	void ApplyVolume(float NewValue, bool bPersist);

	ULSAudioSettingsSubsystem* GetAudioSettingsSubsystem() const;

	ELSSoundBus SoundBus = ELSSoundBus::Master;

	static constexpr float StepSize = 0.05f;
};
