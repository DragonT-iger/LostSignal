#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSMPCVisionSourceComponent.generated.h"

class UMaterialParameterCollection;

UCLASS(ClassGroup = (Vision), meta = (BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSMPCVisionSourceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSMPCVisionSourceComponent();

protected:
	// Starts pushing MPC data as soon as the owning actor is active in the world.
	virtual void BeginPlay() override;

	// Stops the global effect when this source goes away.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Writes the latest player-centered vision values into the shared MPC every frame.
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LS/Vision|MPC")
	TObjectPtr<UMaterialParameterCollection> VisionParameterCollection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|MPC")
	FName CenterParameterName = TEXT("LS_Vision_CenterWS");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|MPC")
	FName RadiusParameterName = TEXT("LS_Vision_Radius");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|MPC")
	FName FeatherParameterName = TEXT("LS_Vision_Feather");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|MPC")
	FName EnabledParameterName = TEXT("LS_Vision_Enabled");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|MPC", meta = (ClampMin = "0.0"))
	float Radius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|MPC", meta = (ClampMin = "0.0"))
	float FeatherWidth = 64.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|MPC")
	FVector WorldOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|MPC")
	bool bOnlyLocallyControlled = true;

private:
	// Filters updates so only the intended local viewpoint drives the shared MPC values.
	bool ShouldUpdateForLocalView() const;

	// Resolves the world-space center that materials should use for their radial mask test.
	FVector GetVisionCenterWorld() const;

	// Pushes the current center/radius/feather values into the configured material parameter collection.
	void PushParametersToMPC(float EnabledValue) const;

	// Clears the effect by writing disabled/default values back into the MPC.
	void ResetParameters() const;
};
