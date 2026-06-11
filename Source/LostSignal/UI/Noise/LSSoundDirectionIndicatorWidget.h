#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Gameplay/LSNoiseTypes.h"
#include "LSSoundDirectionIndicatorWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;

UCLASS()
class LOSTSIGNAL_API ULSSoundDirectionIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI|SoundDirection")
	void InitializeSoundDirectionIndicator(APawn* InObservedPawn);

	UFUNCTION(BlueprintCallable, Category="LS/UI|SoundDirection")
	void ShowSoundDirection(FVector SoundWorldLocation, float DurationSeconds = 1.0f, float Strength = 1.0f);

	UFUNCTION(BlueprintCallable, Category="LS/UI|SoundDirection")
	void ShowNoiseEventDirection(const FLSNoiseEvent& NoiseEvent, float DurationSeconds = 1.0f);

	UFUNCTION(BlueprintCallable, Category="LS/UI|SoundDirection")
	void HideSoundDirection();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|SoundDirection")
	TObjectPtr<UImage> IndicatorImage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|SoundDirection")
	TObjectPtr<UMaterialInterface> IndicatorMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|SoundDirection")
	FName CenterUVParameterName = TEXT("CenterUV");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|SoundDirection")
	FName DirectionAngleParameterName = TEXT("DirectionAngle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|SoundDirection")
	FName AspectRatioParameterName = TEXT("AspectRatio");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|SoundDirection")
	FName OpacityParameterName = TEXT("Opacity");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|SoundDirection")
	FName StrengthParameterName = TEXT("Strength");

private:
	void InitializeIndicatorMaterial();
	void RefreshIndicatorMaterial(float Alpha);
	bool ResolveIndicatorParams(FVector2D& OutCenterUV, float& OutDirectionAngle, float& OutAspectRatio) const;
	APawn* ResolveObservedPawn() const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> IndicatorMaterialInstance;

	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> ObservedPawn;

	FVector ActiveSoundWorldLocation = FVector::ZeroVector;
	float ActiveDurationSeconds = 0.0f;
	float ActiveElapsedSeconds = 0.0f;
	float ActiveStrength = 1.0f;
	bool bIndicatorActive = false;
};
