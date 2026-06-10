#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "LSSurvivalStatusWidget.generated.h"

class ALSCharacterBase;
class UAbilitySystemComponent;
class ULSCharacterAttributeSet;
class ULSCombatAttributeSet;
class UImage;
class UMaterialInstanceDynamic;
class UProgressBar;
class UTextBlock;
class UWidget;
struct FOnAttributeChangeData;

UCLASS(Abstract)
class LOSTSIGNAL_API ULSSurvivalStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="LS/UI|Survival")
	void InitializeSurvivalStatusForPawn(APawn* InPawn);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Survival")
	void SetPreviewSurvivalStatus(int32 CurrentSurvivalProtocol, int32 PreviousSurvivalProtocol, float CurrentHealth, float MaxHealth, float CurrentStamina, float MaxStamina);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Survival")
	void StartPreviewRingCooldown(float Duration);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> StaminaText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> StaminaProgressBar;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UImage> SurvivalCooldownRingImage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Survival", meta=(ClampMin="0.0"))
	float PreviewRingCooldownDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Survival")
	bool bStartPreviewRingCooldownOnConstruct = true;

private:
	TWeakObjectPtr<ALSCharacterBase> ObservedCharacter;
	TWeakObjectPtr<UAbilitySystemComponent> ObservedASC;

	FDelegateHandle CurrentHealthChangedHandle;
	FDelegateHandle MaxHealthChangedHandle;
	FDelegateHandle CurrentStaminaChangedHandle;
	FDelegateHandle MaxStaminaChangedHandle;

	void BindToObservedASC(UAbilitySystemComponent* NewASC);
	void UnbindFromObservedASC();
	void RefreshDisplay();
	void RefreshVisibility();
	void RefreshPreviewRingCooldown(float InDeltaTime);
	void SetRingCooldownProgress(float Progress);
	void ResolveSurvivalProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const;
	bool IsSurvivalFeatureVisible(FName EnableName) const;
	void SetWidgetVisibility(UWidget* Widget, bool bVisible) const;
	void HandleAttributeChanged(const FOnAttributeChangeData& ChangeData);
	const ULSCombatAttributeSet* ResolveCombatAttributeSet() const;
	const ULSCharacterAttributeSet* ResolveCharacterAttributeSet() const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SurvivalCooldownRingMaterial;

	UPROPERTY(Transient)
	bool bUsePreviewSurvivalStatus = false;

	UPROPERTY(Transient)
	int32 PreviewCurrentSurvivalProtocol = 0;

	UPROPERTY(Transient)
	int32 PreviewPreviousSurvivalProtocol = 0;

	UPROPERTY(Transient)
	float PreviewCurrentHealth = 0.0f;

	UPROPERTY(Transient)
	float PreviewMaxHealth = 0.0f;

	UPROPERTY(Transient)
	float PreviewCurrentStamina = 0.0f;

	UPROPERTY(Transient)
	float PreviewMaxStamina = 0.0f;

	float PreviewRingCooldownRemaining = 0.0f;
};
