#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "LSSurvivalStatusWidget.generated.h"

class ALSCharacterBase;
class UAbilitySystemComponent;
class ULSCharacterAttributeSet;
class ULSCombatAttributeSet;
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
	void SetHealthPreview(float TargetHealth, float Duration, bool bIsRecovery);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Survival")
	void ClearHealthPreview();

	UFUNCTION(BlueprintCallable, Category="LS/UI|Survival")
	void StartPreviewSignalCooldown(float Duration);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Survival")
	void SetPreviewSignalProgress(float SignalProgress);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> StaminaText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> HealthPreviewProgressBar;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> StaminaProgressBar;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> SignalProgressBar1;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> SignalProgressBar2;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> SignalProgressBar3;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> SignalProgressBar4;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> SignalProgressBar5;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> SignalProgressBar6;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> SignalProgressBar7;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> SignalProgressBar8;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> SignalProgressBar9;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> SignalProgressBar10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Survival", meta=(ClampMin="0.0"))
	float PreviewSignalCooldownDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Survival")
	bool bStartPreviewSignalCooldownOnConstruct = true;

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
	void InitializeSignalProgressBars();
	void RefreshPreviewSignalCooldown(float InDeltaTime);
	void RefreshSignalProgressFromSave();
	void SetSignalProgress(float Progress);
	void ResolveSurvivalProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const;
	bool IsSurvivalFeatureVisible(FName EnableName) const;
	bool ShouldShowSignalIndicator() const;
	void SetWidgetVisibility(UWidget* Widget, bool bVisible) const;
	void HandleAttributeChanged(const FOnAttributeChangeData& ChangeData);
	const ULSCombatAttributeSet* ResolveCombatAttributeSet() const;
	const ULSCharacterAttributeSet* ResolveCharacterAttributeSet() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProgressBar>> SignalProgressBars;

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

	UPROPERTY(Transient)
	bool bHasHealthPreview = false;

	// 회복량 델타(현재 체력 기준 증감). 표시 시점의 현재 체력에 더해 재계산하므로,
	// 시전 중 데미지로 현재 체력이 바뀌면 프리뷰도 그만큼 따라 조절된다.
	UPROPERTY(Transient)
	float HealthPreviewDelta = 0.0f;

	UPROPERTY(Transient)
	float HealthPreviewDuration = 0.0f;

	UPROPERTY(Transient)
	float HealthPreviewRemaining = 0.0f;

	UPROPERTY(Transient)
	bool bHealthPreviewIsRecovery = true;

	float PreviewSignalCooldownRemaining = 0.0f;
};
