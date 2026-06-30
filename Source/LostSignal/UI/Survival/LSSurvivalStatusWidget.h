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
class UTexture2D;
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
	void StartPreviewRingCooldown(float Duration);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Survival")
	void SetPreviewSignalChip(FName ChipItemRowName, float DisappearProgress);

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
	TObjectPtr<UImage> SurvivalCooldownRingImage;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UImage> ChipImage;

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
	void RefreshSignalChipFromSave();
	void SetRingCooldownProgress(float Progress);
	void SetChipImageTexture(UTexture2D* Texture);
	void SetSignalChipIcon(FName ChipItemRowName);
	void ClearPreviewSignalChip();
	void ResolveSurvivalProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const;
	bool IsSurvivalFeatureVisible(FName EnableName) const;
	bool ShouldShowSignalIndicator() const;
	void SetWidgetVisibility(UWidget* Widget, bool bVisible) const;
	void HandleAttributeChanged(const FOnAttributeChangeData& ChangeData);
	const ULSCombatAttributeSet* ResolveCombatAttributeSet() const;
	const ULSCharacterAttributeSet* ResolveCharacterAttributeSet() const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SurvivalCooldownRingMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ChipImageMaterial;

	UPROPERTY(Transient)
	FName PreviewSignalChipRowName;

	// 레이드 HUD에서 현재 카운트다운 중인 "다음에 사라질" 슬롯 인덱스. 구간 변화 감지용.
	UPROPERTY(Transient)
	int32 ActiveSignalSlotIndex = INDEX_NONE;

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

	UPROPERTY(Transient)
	float HealthPreviewTarget = 0.0f;

	UPROPERTY(Transient)
	float HealthPreviewDuration = 0.0f;

	UPROPERTY(Transient)
	float HealthPreviewRemaining = 0.0f;

	UPROPERTY(Transient)
	bool bHealthPreviewIsRecovery = true;

	float PreviewRingCooldownRemaining = 0.0f;
};
