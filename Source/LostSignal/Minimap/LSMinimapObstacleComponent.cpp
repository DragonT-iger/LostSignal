#include "Minimap/LSMinimapObstacleComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Minimap/LSMinimapSubsystem.h"

ULSMinimapObstacleComponent::ULSMinimapObstacleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULSMinimapObstacleComponent::AddTargetPrimitive(UPrimitiveComponent* Primitive)
{
	if (IsValid(Primitive))
	{
		TargetPrimitives.AddUnique(Primitive);
	}
}

void ULSMinimapObstacleComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (ULSMinimapSubsystem* MinimapSubsystem = World->GetSubsystem<ULSMinimapSubsystem>())
		{
			MinimapSubsystem->RegisterObstacle(this);
		}
	}
}

void ULSMinimapObstacleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (ULSMinimapSubsystem* MinimapSubsystem = World->GetSubsystem<ULSMinimapSubsystem>())
		{
			MinimapSubsystem->UnregisterObstacle(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ULSMinimapObstacleComponent::GatherObstacleBounds(TArray<FBox>& OutBounds) const
{
	OutBounds.Reset();
	if (!bVisibleOnMinimap)
	{
		return;
	}

	for (const UPrimitiveComponent* Primitive : TargetPrimitives)
	{
		if (ShouldUsePrimitive(Primitive))
		{
			OutBounds.Add(Primitive->Bounds.GetBox());
		}
	}

	if (OutBounds.Num() > 0 || !bUseOwnerBlockingPrimitives)
	{
		return;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UPrimitiveComponent*> OwnerPrimitives;
	Owner->GetComponents<UPrimitiveComponent>(OwnerPrimitives);
	for (const UPrimitiveComponent* Primitive : OwnerPrimitives)
	{
		if (ShouldUsePrimitive(Primitive))
		{
			OutBounds.Add(Primitive->Bounds.GetBox());
		}
	}
}

bool ULSMinimapObstacleComponent::ShouldUsePrimitive(const UPrimitiveComponent* Primitive) const
{
	if (!IsValid(Primitive))
	{
		return false;
	}

	const ECollisionEnabled::Type CollisionEnabled = Primitive->GetCollisionEnabled();
	if (CollisionEnabled == ECollisionEnabled::NoCollision)
	{
		return false;
	}

	return Primitive->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block;
}
