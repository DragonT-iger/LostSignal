#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "LSEnemyHealthBarWidget.generated.h"

class UProgressBar;

UCLASS(Abstract)
class LOSTSIGNAL_API ULSEnemyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI|Combat")
	void SetHealthPercent(float Percent);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<UProgressBar> HealthProgressBar;
};
