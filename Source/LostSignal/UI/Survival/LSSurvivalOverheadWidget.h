#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "LSSurvivalOverheadWidget.generated.h"

class ALSCharacterBase;
class UAbilitySystemComponent;
class UProgressBar;
class UTextBlock;
class UWidget;
struct FOnAttributeChangeData;

UCLASS(Abstract)
class LOSTSIGNAL_API ULSSurvivalOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="LS/UI|Survival")
	void InitializeSurvivalOverheadForCharacter(ALSCharacterBase* InCharacter);

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> StaminaProgressBar;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> StaminaText;

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
	bool IsSurvivalFeatureVisible(FName EnableName) const;
	void ResolveSurvivalProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const;
	void SetWidgetVisibility(UWidget* Widget, bool bVisible) const;
	void HandleAttributeChanged(const FOnAttributeChangeData& ChangeData);
};
