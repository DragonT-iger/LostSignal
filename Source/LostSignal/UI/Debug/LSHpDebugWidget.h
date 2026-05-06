#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "LSHpDebugWidget.generated.h"

class ALSCharacterBase;
class UAbilitySystemComponent;
class ULSCombatAttributeSet;
class UProgressBar;
class UTextBlock;
struct FOnAttributeChangeData;

UCLASS(Abstract)
class LOSTSIGNAL_API ULSHpDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UProgressBar> HealthProgressBar;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> CharacterNameText;

private:
	TWeakObjectPtr<ALSCharacterBase> ObservedCharacter;
	TWeakObjectPtr<UAbilitySystemComponent> ObservedASC;

	FDelegateHandle CurrentHealthChangedHandle;
	FDelegateHandle MaxHealthChangedHandle;

	void RefreshObservedCharacter();
	void BindToObservedASC(UAbilitySystemComponent* NewASC);
	void UnbindFromObservedASC();
	void UpdateHealthDisplay();
	void HandleCurrentHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData);
	const ULSCombatAttributeSet* ResolveCombatAttributeSet() const;
};
