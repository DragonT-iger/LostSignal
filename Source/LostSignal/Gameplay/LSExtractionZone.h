#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LSExtractionZone.generated.h"

class UBoxComponent;
class ULSMinimapMarkerComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class LOSTSIGNAL_API ALSExtractionZone : public AActor
{
	GENERATED_BODY()

public:
	ALSExtractionZone();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category="LS/Extraction")
	TObjectPtr<UBoxComponent> ExtractionBox;

	UPROPERTY(VisibleAnywhere, Category="LS/Extraction")
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

	UPROPERTY(VisibleAnywhere, Category="LS/Extraction")
	TObjectPtr<UTextRenderComponent> MarkerText;

	UPROPERTY(VisibleAnywhere, Category="LS/Extraction")
	TObjectPtr<UPointLightComponent> MarkerLight;

	UPROPERTY(VisibleAnywhere, Category="LS/Minimap")
	TObjectPtr<ULSMinimapMarkerComponent> MinimapMarkerComponent;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
};
