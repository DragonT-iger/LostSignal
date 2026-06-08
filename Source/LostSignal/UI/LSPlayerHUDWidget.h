#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSPlayerHUDWidget.generated.h"

class ULSSkillBarWidget;
class ULSMinimapWidget;

/** Root in-game HUD widget. WBP should place and bind SkillBar and Minimap. */
UCLASS()
class LOSTSIGNAL_API ULSPlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void InitializeHUDForPawn(APawn* InPawn);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSSkillBarWidget> SkillBar;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSMinimapWidget> Minimap;
};
