#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSNoiseEmitterComponent.generated.h"

class UDataTable;
struct FLSNoiseProfileRow;

/** Emits character gameplay noise events for monster sensing. */
UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSNoiseEmitterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSNoiseEmitterComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="LS/Noise")
	void EmitNoiseByRow(FName NoiseRowName);

	UFUNCTION(BlueprintCallable, Category="LS/Noise")
	void EmitInteractNoise();

private:
	void UpdateMovementNoise(float DeltaTime);
	void EmitNoiseFromProfile(const FLSNoiseProfileRow& Profile);
	const FLSNoiseProfileRow* FindNoiseProfile(FName NoiseRowName) const;
	bool IsOwnerMoving() const;
	bool IsOwnerRunning() const;
	void LogMissingNoiseProfileTableOnce();

	UPROPERTY(EditDefaultsOnly, Category="LS/Noise")
	TObjectPtr<UDataTable> NoiseProfileTable;

	UPROPERTY(EditDefaultsOnly, Category="LS/Noise")
	FName WalkNoiseRowName = TEXT("Walk");

	UPROPERTY(EditDefaultsOnly, Category="LS/Noise")
	FName RunNoiseRowName = TEXT("Run");

	UPROPERTY(EditDefaultsOnly, Category="LS/Noise")
	FName InteractNoiseRowName = TEXT("Interact");

	UPROPERTY(EditDefaultsOnly, Category="LS/Noise", meta=(ClampMin="0.0"))
	float MinimumMovementSpeed = 10.0f;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Noise")
	float MovementNoiseElapsedSeconds = 0.0f;

	bool bLoggedMissingNoiseProfileTable = false;
};
