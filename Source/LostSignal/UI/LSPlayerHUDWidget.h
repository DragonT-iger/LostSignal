#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "LSPlayerHUDWidget.generated.h"

class ULSSkillBarWidget;
class ULSMinimapWidget;
class ULSSoundDirectionIndicatorWidget;
class ULSSurvivalStatusWidget;
class UPanelWidget;

/** Root in-game HUD widget. WBP should place and bind SkillBar and Minimap. */
UCLASS()
class LOSTSIGNAL_API ULSPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void InitializeHUDForPawn(APawn* InPawn);

	void HandleNoiseForSoundIndicator(FVector NoiseLocation, float RadiusCm, FGameplayTag NoiseTag, AActor* NoiseInstigator);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSSkillBarWidget> SkillBar;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSMinimapWidget> Minimap;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSSurvivalStatusWidget> SurvivalStatus;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Noise")
	TObjectPtr<ULSSoundDirectionIndicatorWidget> SoundIndicator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Debug")
	bool bDebugIgnoreSoundIndicatorProtocolLevel = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Noise", meta=(ClampMin="1", ClampMax="5"))
	int32 MaxSoundIndicatorCount = 5;

private:
	void InitializeSoundIndicatorPool(APawn* InPawn);
	ULSSoundDirectionIndicatorWidget* AcquireSoundIndicator();
	ULSSoundDirectionIndicatorWidget* CreatePooledSoundIndicator(UPanelWidget* ParentPanel);
	void ConfigurePooledSoundIndicatorSlot(ULSSoundDirectionIndicatorWidget* IndicatorWidget) const;
	bool IsSoundIndicatorProtocolVisible() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSSoundDirectionIndicatorWidget>> SoundIndicatorPool;
};
