#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Combat/LSCombatBuffTypes.h"

#include "LSCombatBuffIconWidget.generated.h"

class UProgressBar;
class UImage;
class UTextBlock;
class UTexture2D;

UCLASS()
class LOSTSIGNAL_API ULSCombatBuffIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI|Combat")
	void SetBuffDisplay(const FLSCombatBuffDisplayData& InDisplayData);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<UTextBlock> StackText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<UProgressBar> DurationBar;
};
