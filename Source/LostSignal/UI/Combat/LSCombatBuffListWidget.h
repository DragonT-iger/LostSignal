#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Blueprint/UserWidget.h"
#include "UI/Combat/LSCombatBuffTypes.h"

#include "LSCombatBuffListWidget.generated.h"

class UAbilitySystemComponent;
class ULSCombatBuffIconWidget;
class UPanelWidget;
class UTexture2D;

UCLASS()
class LOSTSIGNAL_API ULSCombatBuffListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI|Combat")
	void InitializeBuffListForPawn(APawn* InPawn);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<UPanelWidget> BuffIconPanel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat")
	TSubclassOf<ULSCombatBuffIconWidget> BuffIconWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat")
	TMap<FGameplayTag, TObjectPtr<UTexture2D>> BuffIconTextures;

private:
	void RefreshBuffList();
	void BuildBuffDisplays(TArray<FLSCombatBuffDisplayData>& OutDisplays) const;
	ULSCombatBuffIconWidget* GetOrCreateBuffIcon(int32 Index);
	bool IsBuffDurationProtocolVisible() const;
	void ResolveBattleProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const;

	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> ObservedPawn;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSCombatBuffIconWidget>> BuffIconPool;

	bool bLoggedMissingIconClass = false;
};
