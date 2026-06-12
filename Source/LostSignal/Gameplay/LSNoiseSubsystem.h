#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Gameplay/LSNoiseTypes.h"
#include "LSNoiseSubsystem.generated.h"

class ULSMonsterSenseComponent;

/** World-level broker for gameplay noise events. */
UCLASS()
class LOSTSIGNAL_API ULSNoiseSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterListener(ULSMonsterSenseComponent* Listener);
	void UnregisterListener(ULSMonsterSenseComponent* Listener);
	void EmitNoise(const FLSNoiseEvent& NoiseEvent);

private:
	UPROPERTY(Transient)
	TSet<TObjectPtr<ULSMonsterSenseComponent>> Listeners;
};
