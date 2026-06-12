#include "MiniGame/RatSteal/LSRatYSortComponent.h"

#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"

ULSRatYSortComponent::ULSRatYSortComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULSRatYSortComponent::BeginPlay()
{
	Super::BeginPlay();

	Apply();
	if (bStatic)
	{
		SetComponentTickEnabled(false);
	}
}

void ULSRatYSortComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Apply();
}

void ULSRatYSortComponent::Apply()
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TInlineComponentArray<UMeshComponent*> MeshComponents(Owner);
	for (UMeshComponent* Mesh : MeshComponents)
	{
		if (Mesh)
		{
			const int32 Priority = FMath::RoundToInt(-Mesh->GetComponentLocation().Z * 10.f) + SortOffset;
			Mesh->SetTranslucentSortPriority(Priority);
		}
	}
}
