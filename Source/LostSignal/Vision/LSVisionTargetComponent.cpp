#include "Vision/LSVisionTargetComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Vision/LSVisionSubsystem.h"
#include "Vision/LSVisionTypes.h"

ULSVisionTargetComponent::ULSVisionTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Seeds default sample points and registers this actor as a visibility target for local vision checks.
void ULSVisionTargetComponent::BeginPlay()
{
	Super::BeginPlay();

	if (VisibilitySampleOffsets.Num() == 0)
	{
		VisibilitySampleOffsets.Add(FVector::ZeroVector);
		VisibilitySampleOffsets.Add(FVector(0.0f, 0.0f, 50.0f));
		VisibilitySampleOffsets.Add(FVector(0.0f, 0.0f, 100.0f));
	}

	if (UWorld* World = GetWorld())
	{
		if (ULSVisionSubsystem* VisionSubsystem = World->GetSubsystem<ULSVisionSubsystem>())
		{
			VisionSubsystem->RegisterTarget(this);
		}
	}
}

// Removes the target from the subsystem so hidden/visible state is no longer driven after teardown.
void ULSVisionTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (ULSVisionSubsystem* VisionSubsystem = World->GetSubsystem<ULSVisionSubsystem>())
		{
			VisionSubsystem->UnregisterTarget(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

// Applies the local visibility result to the configured primitive set.
void ULSVisionTargetComponent::SetLocallyVisible(const bool bVisible)
{
	// 시야 갱신이 변화 감지 없이 매 주기 호출되므로, 값이 실제로 바뀔 때만 알린다.
	const bool bChanged = (bIsLocallyVisible != bVisible);
	bIsLocallyVisible = bVisible;

	if (bChanged)
	{
		OnLocalVisibilityChanged.Broadcast(bVisible);
	}

	if (!bHideWhenNotVisible)
	{
		return;
	}

	TArray<UPrimitiveComponent*> TargetComponents;
	GatherRenderPrimitives(TargetComponents);

	for (UPrimitiveComponent* PrimitiveComponent : TargetComponents)
	{
		if (PrimitiveComponent != nullptr)
		{
			PrimitiveComponent->SetVisibility(bVisible, true);
		}
	}
}

// Builds the world-space probe points that the vision system tests against the current polygon.
void ULSVisionTargetComponent::GatherVisibilitySamplePoints(TArray<FVector>& OutSamplePoints) const
{
	OutSamplePoints.Reset();

	const AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	for (const FVector& Offset : VisibilitySampleOffsets)
	{
		OutSamplePoints.Add(OwnerLocation + Offset);
	}
}

// Resolves which primitive components should be shown/hidden when local visibility changes.
void ULSVisionTargetComponent::GatherRenderPrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const
{
	OutPrimitives.Reset();

	for (UPrimitiveComponent* PrimitiveComponent : RenderPrimitives)
	{
		if (PrimitiveComponent != nullptr && !PrimitiveComponent->ComponentHasTag(LSVisionTags::HideExempt))
		{
			OutPrimitives.AddUnique(PrimitiveComponent);
		}
	}

	if (OutPrimitives.Num() > 0 || !bUseOwnerPrimitiveComponents)
	{
		return;
	}

	if (const AActor* OwnerActor = GetOwner())
	{
		TArray<UPrimitiveComponent*> OwnerPrimitiveComponents;
		OwnerActor->GetComponents<UPrimitiveComponent>(OwnerPrimitiveComponents);

		for (UPrimitiveComponent* PrimitiveComponent : OwnerPrimitiveComponents)
		{
			if (PrimitiveComponent != nullptr && !PrimitiveComponent->ComponentHasTag(LSVisionTags::HideExempt))
			{
				OutPrimitives.AddUnique(PrimitiveComponent);
			}
		}
	}
}
