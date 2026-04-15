#include "Vision/LSVisionSubsystem.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "LostSignal.h"
#include "Vision/LSVisionMaskRenderer.h"
#include "Vision/LSVisionOccluderComponent.h"
#include "Vision/LSVisionSettings.h"
#include "Vision/LSVisionTargetComponent.h"

// Creates the shared runtime objects that every local vision calculation depends on.
void ULSVisionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const ULSVisionSettings* VisionSettings = GetDefault<ULSVisionSettings>();
	UClass* MaskRendererClass = ALSVisionMaskRenderer::StaticClass();

	if (VisionSettings != nullptr && !VisionSettings->MaskRendererClass.IsNull())
	{
		if (UClass* LoadedClass = VisionSettings->MaskRendererClass.LoadSynchronous())
		{
			MaskRendererClass = LoadedClass;
		}
	}

	RuntimeMaskRenderTarget = ResolveVisibilityMaskRenderTarget();

	if (UWorld* World = GetWorld(); World != nullptr && MaskRendererClass != nullptr)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags |= RF_Transient;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		MaskRenderer = World->SpawnActor<ALSVisionMaskRenderer>(MaskRendererClass, FTransform::Identity, SpawnParameters);
		if (MaskRenderer != nullptr)
		{
			MaskRenderer->SetActorHiddenInGame(true);
			MaskRenderer->SetCanBeDamaged(false);
			MaskRenderer->VisibilityMaskRenderTarget = RuntimeMaskRenderTarget;
		}
	}

	RegisteredOccluders.Reset();
	RegisteredSurfaces.Reset();
	RegisteredTargets.Reset();
}

// Releases the shared vision runtime objects when the world is torn down.
void ULSVisionSubsystem::Deinitialize()
{
	RegisteredOccluders.Reset();
	RegisteredSurfaces.Reset();
	RegisteredTargets.Reset();

	if (IsValid(MaskRenderer))
	{
		MaskRenderer->Destroy();
		MaskRenderer = nullptr;
	}

	RuntimeMaskRenderTarget = nullptr;

	Super::Deinitialize();
}

// Tracks occluders so the solver can collect all blocking segments in one place.
void ULSVisionSubsystem::RegisterOccluder(ULSVisionOccluderComponent* Occluder)
{
	if (Occluder != nullptr)
	{
		RegisteredOccluders.AddUnique(Occluder);
	}
}

// Removes occluders that are no longer valid for this world.
void ULSVisionSubsystem::UnregisterOccluder(ULSVisionOccluderComponent* Occluder)
{
	RegisteredOccluders.Remove(Occluder);
}

// Tracks surfaces that need the latest mask texture and transform parameters.
void ULSVisionSubsystem::RegisterSurface(ULSVisionSurfaceComponent* Surface)
{
	if (Surface != nullptr)
	{
		RegisteredSurfaces.AddUnique(Surface);
	}
}

// Removes surfaces when the owning actor/component leaves the world.
void ULSVisionSubsystem::UnregisterSurface(ULSVisionSurfaceComponent* Surface)
{
	RegisteredSurfaces.Remove(Surface);
}

// Tracks visibility targets that can be shown/hidden by local vision checks.
void ULSVisionSubsystem::RegisterTarget(ULSVisionTargetComponent* Target)
{
	if (Target != nullptr)
	{
		RegisteredTargets.AddUnique(Target);
	}
}

// Removes visibility targets that should no longer receive local visibility updates.
void ULSVisionSubsystem::UnregisterTarget(ULSVisionTargetComponent* Target)
{
	RegisteredTargets.Remove(Target);
}

// Chooses either a configured render target asset or a transient fallback created at runtime.
UTextureRenderTarget2D* ULSVisionSubsystem::ResolveVisibilityMaskRenderTarget()
{
	const ULSVisionSettings* VisionSettings = GetDefault<ULSVisionSettings>();
	if (VisionSettings != nullptr && !VisionSettings->VisibilityMaskRenderTarget.IsNull())
	{
		if (UTextureRenderTarget2D* ConfiguredRenderTarget = VisionSettings->VisibilityMaskRenderTarget.LoadSynchronous())
		{
			return ConfiguredRenderTarget;
		}
	}

	const int32 FallbackSize = VisionSettings != nullptr
		? FMath::Clamp(VisionSettings->FallbackRenderTargetSize, 128, 4096)
		: 1024;

	return CreateFallbackRenderTarget(FallbackSize);
}

// Creates a transient UAV-capable render target so the shader path works without BP setup.
UTextureRenderTarget2D* ULSVisionSubsystem::CreateFallbackRenderTarget(const int32 Size)
{
	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("LSVisionMaskRT_Transient"));
	if (RenderTarget == nullptr)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to allocate fallback vision mask render target."));
		return nullptr;
	}

	RenderTarget->RenderTargetFormat = RTF_RGBA8;
	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->bAutoGenerateMips = false;
	RenderTarget->bCanCreateUAV = true;
	RenderTarget->InitAutoFormat(Size, Size);
	RenderTarget->UpdateResourceImmediate(true);
	return RenderTarget;
}
