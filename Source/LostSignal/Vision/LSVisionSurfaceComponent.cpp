#include "Vision/LSVisionSurfaceComponent.h"

#include "Components/MeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Vision/LSVisionSubsystem.h"

ULSVisionSurfaceComponent::ULSVisionSurfaceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Creates material instances and registers this surface so it receives vision updates each frame.
void ULSVisionSurfaceComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeVisionMaterials();

	if (UWorld* World = GetWorld())
	{
		if (ULSVisionSubsystem* VisionSubsystem = World->GetSubsystem<ULSVisionSubsystem>())
		{
			VisionSubsystem->RegisterSurface(this);
		}
	}
}

// Unregisters the surface so future vision updates do not touch destroyed materials/components.
void ULSVisionSurfaceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (ULSVisionSubsystem* VisionSubsystem = World->GetSubsystem<ULSVisionSubsystem>())
		{
			VisionSubsystem->UnregisterSurface(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

// Collects the mesh components that should receive mask parameters from this surface wrapper.
void ULSVisionSurfaceComponent::GatherTargetMeshComponents(TArray<UMeshComponent*>& OutMeshComponents) const
{
	OutMeshComponents.Reset();

	if (TargetPrimitives.Num() > 0)
	{
		for (UPrimitiveComponent* Primitive : TargetPrimitives)
		{
			if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(Primitive))
			{
				OutMeshComponents.AddUnique(MeshComponent);
			}
		}

		return;
	}

	if (const AActor* OwnerActor = GetOwner())
	{
		TArray<UMeshComponent*> OwnerMeshComponents;
		OwnerActor->GetComponents<UMeshComponent>(OwnerMeshComponents);
		OutMeshComponents.Append(OwnerMeshComponents);
	}
}

// Replaces the target materials with MIDs so mask texture and world-space parameters can be pushed at runtime.
void ULSVisionSurfaceComponent::InitializeVisionMaterials()
{
	VisionMaterialInstances.Reset();

	TArray<UMeshComponent*> MeshComponents;
	GatherTargetMeshComponents(MeshComponents);

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (MeshComponent == nullptr)
		{
			continue;
		}

		const int32 MaterialCount = MeshComponent->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			UMaterialInterface* SourceMaterial = VisionMaterialOverride;
			if (SourceMaterial == nullptr && bUseExistingMaterials)
			{
				SourceMaterial = MeshComponent->GetMaterial(MaterialIndex);

				// 재호출 시 이미 이 컴포넌트가 교체한 비전 MID를 다시 감싸면 MID-of-MID가 되어 누수/이중 래핑이 생긴다.
				// 그 경우 부모(원본) 머티리얼을 소스로 사용한다.
				if (UMaterialInstanceDynamic* ExistingMID = Cast<UMaterialInstanceDynamic>(SourceMaterial))
				{
					if (ExistingMID->GetOuter() == this && ExistingMID->Parent != nullptr)
					{
						SourceMaterial = ExistingMID->Parent;
					}
				}
			}

			if (SourceMaterial == nullptr)
			{
				continue;
			}

			UMaterialInstanceDynamic* VisionMID = UMaterialInstanceDynamic::Create(SourceMaterial, this);
			if (VisionMID == nullptr)
			{
				continue;
			}

			VisionMaterialInstances.Add(VisionMID);
			MeshComponent->SetMaterial(MaterialIndex, VisionMID);
		}
	}
}

// Pushes the latest mask RT and placement parameters into every vision-enabled material instance.
void ULSVisionSurfaceComponent::ApplyVisionParameters(
	UTextureRenderTarget2D* VisibilityMaskRT,
	const FVector& MaskOriginWS,
	const float MaskExtent,
	const FVector2D& PlayerForward2D)
{
	for (UMaterialInstanceDynamic* VisionMID : VisionMaterialInstances)
	{
		if (VisionMID == nullptr)
		{
			continue;
		}

		if (VisibilityMaskRT != nullptr)
		{
			VisionMID->SetTextureParameterValue(VisibilityMaskTextureParamName, VisibilityMaskRT);
		}

		VisionMID->SetVectorParameterValue(MaskOriginParamName, FLinearColor(MaskOriginWS.X, MaskOriginWS.Y, MaskOriginWS.Z, 0.0f));
		VisionMID->SetScalarParameterValue(MaskExtentParamName, MaskExtent);
		VisionMID->SetVectorParameterValue(Forward2DParamName, FLinearColor(PlayerForward2D.X, PlayerForward2D.Y, 0.0f, 0.0f));
	}
}
