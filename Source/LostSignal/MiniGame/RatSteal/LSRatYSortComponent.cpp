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

	const int32 Priority = static_cast<int32>(-Owner->GetActorLocation().Z) + SortOffset;

	TInlineComponentArray<UMeshComponent*> MeshComponents(Owner);
	for (UMeshComponent* Mesh : MeshComponents)
	{
		if (Mesh)
		{
			Mesh->SetTranslucentSortPriority(Priority);
		}
	}
}
