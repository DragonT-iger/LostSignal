#include "Vision/LSVisionOccluderComponent.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "LostSignal.h"
#include "Vision/LSVisionCollisionGeometry.h"
#include "Vision/LSVisionSettings.h"
#include "Vision/LSVisionSubsystem.h"

ULSVisionOccluderComponent::ULSVisionOccluderComponent()
{
	// 틱은 디버그 세그먼트 드로우 전용이다. 레벨의 모든 벽이 무의미한 틱 비용을 물지 않도록 기본은 꺼두고,
	// bDrawDebugSegments가 켜졌을 때만 활성화한다.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ULSVisionOccluderComponent::OnRegister()
{
	Super::OnRegister();

	RebuildSegments();
	UpdateObservedComponentBinding();
}

void ULSVisionOccluderComponent::BeginPlay()
{
	Super::BeginPlay();
	RebuildSegments();
	UpdateObservedComponentBinding();
	SetComponentTickEnabled(bDrawDebugSegments);

	if (UWorld* World = GetWorld())
	{
		if (ULSVisionSubsystem* VisionSubsystem = World->GetSubsystem<ULSVisionSubsystem>())
		{
			VisionSubsystem->RegisterOccluder(this);
		}
	}
}

void ULSVisionOccluderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (ULSVisionSubsystem* VisionSubsystem = World->GetSubsystem<ULSVisionSubsystem>())
		{
			VisionSubsystem->UnregisterOccluder(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ULSVisionOccluderComponent::OnUnregister()
{
	UpdateObservedComponentBinding();

	Super::OnUnregister();
}

#if WITH_EDITOR
// PIE 중 디테일 패널에서 디버그 체크박스를 토글해도 틱 상태가 즉시 따라오게 한다.
void ULSVisionOccluderComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SetComponentTickEnabled(bDrawDebugSegments);
}
#endif

void ULSVisionOccluderComponent::RebuildSegments()
{
	TArray<FLSVisionSegment2D> RebuiltSegments;
	const float SliceZ = ResolveSliceZ();
	BuildSegmentsForSourceMode(SliceZ, RebuiltSegments);
	Segments = MoveTemp(RebuiltSegments);

	if (Segments.Num() == 0 && GetOwner() != nullptr)
	{
		UE_LOG(
			LogLS,
			Verbose,
			TEXT("LSVisionOccluderComponent on '%s' produced no occluder segments (collision may not cross the vision slice height)."),
			*GetNameSafe(GetOwner()));
	}

	if (GetOwner() != nullptr && GetOwner()->HasActorBegunPlay())
	{
		if (UWorld* World = GetWorld())
		{
			if (ULSVisionSubsystem* VisionSubsystem = World->GetSubsystem<ULSVisionSubsystem>())
			{
				VisionSubsystem->RefreshOccluder(this);
			}
		}
	}
}

void ULSVisionOccluderComponent::BuildSegmentsForSourceMode(
	const float SliceZ,
	TArray<FLSVisionSegment2D>& OutSegments) const
{
	switch (SourceMode)
	{
	case ELSVisionOccluderSourceMode::CollisionGeometry:
		if (UPrimitiveComponent* PrimitiveComponent = ResolveMeshPrimitiveComponent())
		{
			LSVisionCollisionGeometry::AppendCollisionSegments(PrimitiveComponent, SliceZ, OutSegments);
		}
		break;

	case ELSVisionOccluderSourceMode::BoxComponent:
		if (UBoxComponent* BoxComponent = ResolveBoxComponent())
		{
			LSVisionCollisionGeometry::AppendBoxComponentSegments(BoxComponent, OutSegments);
		}
		break;

	case ELSVisionOccluderSourceMode::MeshBounds:
		if (UPrimitiveComponent* PrimitiveComponent = ResolvePrimitiveComponent())
		{
			LSVisionCollisionGeometry::AppendMeshBoundsSegments(PrimitiveComponent, SliceZ, OutSegments);
		}
		break;

	case ELSVisionOccluderSourceMode::PrimitiveBounds:
		if (UPrimitiveComponent* PrimitiveComponent = ResolvePrimitiveComponent())
		{
			LSVisionCollisionGeometry::AppendPrimitiveBoundsSegments(PrimitiveComponent, SliceZ, OutSegments);
		}
		break;

	case ELSVisionOccluderSourceMode::ManualSegments:
		OutSegments = ManualSegments;
		break;

	default:
		break;
	}
}

void ULSVisionOccluderComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDrawDebugSegments)
	{
		DrawDebugSegments();
	}
}

void ULSVisionOccluderComponent::UpdateObservedComponentBinding()
{
	if (ObservedSceneComponent.IsValid())
	{
		ObservedSceneComponent->TransformUpdated.RemoveAll(this);
		ObservedSceneComponent.Reset();
	}

	if (!bRebuildOnTransformChanged || SourceMode == ELSVisionOccluderSourceMode::ManualSegments)
	{
		return;
	}

	if (USceneComponent* SceneComponent = ResolveObservedSceneComponent())
	{
		SceneComponent->TransformUpdated.AddUObject(this, &ULSVisionOccluderComponent::HandleObservedComponentTransformUpdated);
		ObservedSceneComponent = SceneComponent;
	}
}

void ULSVisionOccluderComponent::HandleObservedComponentTransformUpdated(
	USceneComponent* UpdatedComponent,
	EUpdateTransformFlags UpdateTransformFlags,
	ETeleportType Teleport)
{
	RebuildSegments();
}

UBoxComponent* ULSVisionOccluderComponent::ResolveBoxComponent() const
{
	if (SourceBoxComponent != nullptr)
	{
		return SourceBoxComponent;
	}

	if (bAutoFindOwnerComponents && GetOwner() != nullptr)
	{
		return GetOwner()->FindComponentByClass<UBoxComponent>();
	}

	return nullptr;
}

UPrimitiveComponent* ULSVisionOccluderComponent::ResolveMeshPrimitiveComponent() const
{
	if (SourcePrimitiveComponent != nullptr)
	{
		return SourcePrimitiveComponent;
	}

	if (bAutoFindOwnerComponents && GetOwner() != nullptr)
	{
		if (UStaticMeshComponent* StaticMeshComponent = GetOwner()->FindComponentByClass<UStaticMeshComponent>())
		{
			return StaticMeshComponent;
		}

		return GetOwner()->FindComponentByClass<UPrimitiveComponent>();
	}

	return nullptr;
}

UPrimitiveComponent* ULSVisionOccluderComponent::ResolvePrimitiveComponent() const
{
	if (SourcePrimitiveComponent != nullptr)
	{
		return SourcePrimitiveComponent;
	}

	if (bAutoFindOwnerComponents && GetOwner() != nullptr)
	{
		return GetOwner()->FindComponentByClass<UPrimitiveComponent>();
	}

	return nullptr;
}

void ULSVisionOccluderComponent::DrawDebugSegments() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	for (const FLSVisionSegment2D& Segment : Segments)
	{
		DrawDebugLine(
			World,
			FVector(Segment.Start.X, Segment.Start.Y, DebugDrawZOffset),
			FVector(Segment.End.X, Segment.End.Y, DebugDrawZOffset),
			DebugSegmentColor,
			false,
			0.0f,
			0,
			DebugDrawThickness);
	}
}

USceneComponent* ULSVisionOccluderComponent::ResolveObservedSceneComponent() const
{
	switch (SourceMode)
	{
	case ELSVisionOccluderSourceMode::BoxComponent:
		return ResolveBoxComponent();

	case ELSVisionOccluderSourceMode::CollisionGeometry:
	case ELSVisionOccluderSourceMode::MeshBounds:
		return ResolveMeshPrimitiveComponent();

	case ELSVisionOccluderSourceMode::PrimitiveBounds:
		return ResolvePrimitiveComponent();

	case ELSVisionOccluderSourceMode::ManualSegments:
	default:
		return nullptr;
	}
}

float ULSVisionOccluderComponent::ResolveSliceZ() const
{
	const ULSVisionSettings* VisionSettings = GetDefault<ULSVisionSettings>();
	if (VisionSettings != nullptr && VisionSettings->bSliceHeightFromPlayer)
	{
		if (const UWorld* World = GetWorld())
		{
			if (const ULSVisionSubsystem* VisionSubsystem = World->GetSubsystem<ULSVisionSubsystem>())
			{
				return VisionSubsystem->GetRuntimeSliceZ();
			}
		}
	}

	return VisionSettings != nullptr ? VisionSettings->OccluderSliceHeight : 0.0f;
}
