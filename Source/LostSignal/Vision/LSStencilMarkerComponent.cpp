#include "Vision/LSStencilMarkerComponent.h"

#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"

ULSStencilMarkerComponent::ULSStencilMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULSStencilMarkerComponent::OnRegister()
{
	Super::OnRegister();

	ApplyStencilSettings();
}

void ULSStencilMarkerComponent::BeginPlay()
{
	Super::BeginPlay();

	ApplyStencilSettings();
}

void ULSStencilMarkerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestorePreviousSettings();

	Super::EndPlay(EndPlayReason);
}

void ULSStencilMarkerComponent::OnUnregister()
{
	RestorePreviousSettings();

	Super::OnUnregister();
}

void ULSStencilMarkerComponent::ApplyStencilSettings()
{
	RestorePreviousSettings();

	TArray<UMeshComponent*> MeshComponents;
	GatherTargetMeshComponents(MeshComponents);

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (MeshComponent == nullptr)
		{
			continue;
		}

		AppliedMeshComponents.Add(MeshComponent);
		PreviousRenderCustomDepthStates.Add(MeshComponent->bRenderCustomDepth);
		PreviousStencilValues.Add(MeshComponent->CustomDepthStencilValue);

		MeshComponent->SetRenderCustomDepth(bEnableCustomDepth);
		MeshComponent->SetCustomDepthStencilValue(StencilValue);
	}
}

void ULSStencilMarkerComponent::GatherTargetMeshComponents(TArray<UMeshComponent*>& OutMeshComponents) const
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

	if (!bAutoFindOwnerMeshes)
	{
		return;
	}

	if (const AActor* OwnerActor = GetOwner())
	{
		TArray<UMeshComponent*> OwnerMeshComponents;
		OwnerActor->GetComponents<UMeshComponent>(OwnerMeshComponents);
		OutMeshComponents.Append(OwnerMeshComponents);
	}
}

void ULSStencilMarkerComponent::RestorePreviousSettings()
{
	for (int32 MeshIndex = 0; MeshIndex < AppliedMeshComponents.Num(); ++MeshIndex)
	{
		UMeshComponent* MeshComponent = AppliedMeshComponents[MeshIndex].Get();
		if (MeshComponent == nullptr)
		{
			continue;
		}

		if (PreviousRenderCustomDepthStates.IsValidIndex(MeshIndex))
		{
			MeshComponent->SetRenderCustomDepth(PreviousRenderCustomDepthStates[MeshIndex]);
		}

		if (PreviousStencilValues.IsValidIndex(MeshIndex))
		{
			MeshComponent->SetCustomDepthStencilValue(PreviousStencilValues[MeshIndex]);
		}
	}

	AppliedMeshComponents.Reset();
	PreviousRenderCustomDepthStates.Reset();
	PreviousStencilValues.Reset();
}
