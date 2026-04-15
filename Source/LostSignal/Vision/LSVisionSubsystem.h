#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LSVisionSubsystem.generated.h"

class ALSVisionMaskRenderer;
class ULSVisionOccluderComponent;
class ULSVisionSurfaceComponent;
class ULSVisionTargetComponent;
class UTextureRenderTarget2D;

UCLASS()
class LOSTSIGNAL_API ULSVisionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void RegisterOccluder(ULSVisionOccluderComponent* Occluder);
	void UnregisterOccluder(ULSVisionOccluderComponent* Occluder);

	void RegisterSurface(ULSVisionSurfaceComponent* Surface);
	void UnregisterSurface(ULSVisionSurfaceComponent* Surface);

	void RegisterTarget(ULSVisionTargetComponent* Target);
	void UnregisterTarget(ULSVisionTargetComponent* Target);

	const TArray<TObjectPtr<ULSVisionOccluderComponent>>& GetRegisteredOccluders() const
	{
		return RegisteredOccluders;
	}

	const TArray<TObjectPtr<ULSVisionSurfaceComponent>>& GetRegisteredVisionSurfaces() const
	{
		return RegisteredSurfaces;
	}

	const TArray<TObjectPtr<ULSVisionTargetComponent>>& GetRegisteredVisionTargets() const
	{
		return RegisteredTargets;
	}

	ALSVisionMaskRenderer* GetMaskRenderer() const
	{
		return MaskRenderer;
	}

	UTextureRenderTarget2D* GetVisibilityMaskRenderTarget() const
	{
		return RuntimeMaskRenderTarget;
	}

private:
	UTextureRenderTarget2D* ResolveVisibilityMaskRenderTarget();
	UTextureRenderTarget2D* CreateFallbackRenderTarget(int32 Size);

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSVisionOccluderComponent>> RegisteredOccluders;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSVisionSurfaceComponent>> RegisteredSurfaces;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSVisionTargetComponent>> RegisteredTargets;

	UPROPERTY(Transient)
	TObjectPtr<ALSVisionMaskRenderer> MaskRenderer;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RuntimeMaskRenderTarget;
};
