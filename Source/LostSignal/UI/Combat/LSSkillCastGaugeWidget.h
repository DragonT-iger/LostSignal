#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "LSSkillCastGaugeWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS()
class LOSTSIGNAL_API ULSSkillCastGaugeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI|Combat")
	void StartCastGauge(FText Label, float Duration);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Combat")
	void StopCastGauge();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<UTextBlock> CastLabelText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<UTextBlock> CastTimeText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<UProgressBar> CastProgressBar;

private:
	void RefreshCastGauge();
	bool IsCastGaugeProtocolVisible() const;
	void ResolveBattleProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const;

	FText ActiveLabel;
	float CastDuration = 0.0f;
	float CastStartTime = 0.0f;
	bool bCasting = false;
};
