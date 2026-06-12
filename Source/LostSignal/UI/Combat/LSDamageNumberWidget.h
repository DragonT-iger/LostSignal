#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "UI/Combat/LSDamageNumberTypes.h"
#include "LSDamageNumberWidget.generated.h"

class UTextBlock;

UCLASS(Abstract)
class LOSTSIGNAL_API ULSDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI|Combat")
	void ShowDamageNumber(const FLSDamageNumberPayload& Payload);

	UFUNCTION(BlueprintPure, Category="LS/UI|Combat")
	bool IsDamageNumberActive() const { return bActive; }

	float GetRemainingLifeSeconds() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<UTextBlock> DamageText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat", meta=(ClampMin="0.01"))
	float DurationSeconds = 0.85f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat")
	FVector2D WidgetDesiredSize = FVector2D(96.0f, 32.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat")
	float StartVerticalOffset = -12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat")
	float EndVerticalOffset = -64.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat", meta=(ClampMin="0.0"))
	float RandomHorizontalSpread = 18.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat", meta=(ClampMin="0.0"))
	float ScreenPadding = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat")
	FSlateColor NormalDamageColor = FSlateColor(FLinearColor::White);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat")
	FSlateColor CriticalDamageColor = FSlateColor(FLinearColor(1.0f, 0.85f, 0.2f, 1.0f));

private:
	void HideDamageNumber();
	void RefreshDamageNumber(float LifeAlpha);
	bool ProjectDamageLocation(FVector2D& OutWidgetPosition) const;

	FVector ActiveWorldLocation = FVector::ZeroVector;
	FVector2D RandomScreenOffset = FVector2D::ZeroVector;
	float ActiveDurationSeconds = 0.0f;
	float ActiveElapsedSeconds = 0.0f;
	bool bActive = false;
};
