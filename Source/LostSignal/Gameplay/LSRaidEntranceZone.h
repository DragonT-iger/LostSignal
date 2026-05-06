#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LSRaidEntranceZone.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class LOSTSIGNAL_API ALSRaidEntranceZone : public AActor
{
	GENERATED_BODY()

public:
	ALSRaidEntranceZone();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category="LS/Lobby")
	TObjectPtr<UBoxComponent> EntranceBox;

	UPROPERTY(VisibleAnywhere, Category="LS/Lobby")
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

	UPROPERTY(VisibleAnywhere, Category="LS/Lobby")
	TObjectPtr<UTextRenderComponent> MarkerText;

	UPROPERTY(VisibleAnywhere, Category="LS/Lobby")
	TObjectPtr<UPointLightComponent> MarkerLight;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
};
