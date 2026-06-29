#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "LSAN_PlaySound.generated.h"

class USoundAttenuation;
class USoundBase;
class USoundConcurrency;

UCLASS()
class LOSTSIGNAL_API ULSAN_PlaySound : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

private:
	UPROPERTY(EditAnywhere, Category="LS/Audio")
	TObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, Category="LS/Audio")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category="LS/Audio")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category="LS/Audio")
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Category="LS/Audio")
	float PitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Category="LS/Audio")
	float StartTime = 0.0f;

	UPROPERTY(EditAnywhere, Category="LS/Audio")
	TObjectPtr<USoundAttenuation> AttenuationSettings;

	UPROPERTY(EditAnywhere, Category="LS/Audio")
	TObjectPtr<USoundConcurrency> ConcurrencySettings;
};
