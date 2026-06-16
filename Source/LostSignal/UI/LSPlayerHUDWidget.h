#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "UI/Combat/LSDamageNumberTypes.h"
#include "LSPlayerHUDWidget.generated.h"

class ULSDamageNumberWidget;
class ULSCombatBuffListWidget;
class ULSSkillBarWidget;
class ULSSkillCastGaugeWidget;
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
	void ShowDamageNumber(const FLSDamageNumberPayload& Payload);
	UFUNCTION(BlueprintCallable, Category="LS/UI|Combat")
	void ShowSkillCastGauge(FText Label, float Duration);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Combat")
	void HideSkillCastGauge();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSSkillBarWidget> SkillBar;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSMinimapWidget> Minimap;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSSurvivalStatusWidget> SurvivalStatus;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<ULSCombatBuffListWidget> CombatBuffList;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<ULSSkillCastGaugeWidget> SkillCastGauge;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Noise")
	TObjectPtr<ULSSoundDirectionIndicatorWidget> SoundIndicator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Debug")
	bool bDebugIgnoreSoundIndicatorProtocolLevel = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Noise", meta=(ClampMin="1", ClampMax="5"))
	int32 MaxSoundIndicatorCount = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat")
	TSubclassOf<ULSDamageNumberWidget> DamageNumberWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat", meta=(ClampMin="1", ClampMax="40"))
	int32 MaxDamageNumberCount = 24;

private:
	void InitializeSoundIndicatorPool(APawn* InPawn);
	void HideSoundIndicatorPool();
	ULSSoundDirectionIndicatorWidget* AcquireSoundIndicator();
	ULSSoundDirectionIndicatorWidget* CreatePooledSoundIndicator(UPanelWidget* ParentPanel);
	void ConfigurePooledSoundIndicatorSlot(ULSSoundDirectionIndicatorWidget* IndicatorWidget) const;
	bool IsSoundIndicatorProtocolVisible() const;
	ULSDamageNumberWidget* AcquireDamageNumberWidget();
	ULSDamageNumberWidget* CreateDamageNumberWidget();
	bool IsDamageNumberProtocolVisible() const;
	void ResolveBattleProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSSoundDirectionIndicatorWidget>> SoundIndicatorPool;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSDamageNumberWidget>> DamageNumberPool;

	mutable bool bLoggedMissingSoundIndicatorProtocolData = false;
};
