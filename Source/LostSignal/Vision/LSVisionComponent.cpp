#include "Vision/LSVisionComponent.h"

#include "Camera/CameraComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Vision/LSVisionMaskRenderer.h"
#include "Vision/LSVisionOccluderComponent.h"
#include "Vision/LSVisionSolver.h"
#include "Vision/LSVisionSubsystem.h"
#include "Vision/LSVisionSurfaceComponent.h"
#include "Vision/LSVisionTargetComponent.h"

ULSVisionComponent::ULSVisionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Starts periodic vision updates and optionally injects the vision post-process material into the local camera.
void ULSVisionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			VisionUpdateTimerHandle,
			this,
			&ULSVisionComponent::UpdateVisionPolygon,
			UpdateInterval,
			true);
	}

	if (PostProcessMaterial != nullptr)
	{
		PostProcessMID = UMaterialInstanceDynamic::Create(PostProcessMaterial, this);

		if (PostProcessMID != nullptr)
		{
			if (UCameraComponent* Camera = GetOwner() ? GetOwner()->FindComponentByClass<UCameraComponent>() : nullptr)
			{
				Camera->PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, PostProcessMID));
			}
		}
	}
}

// Stops the periodic vision update loop when the owning actor leaves the world.
void ULSVisionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(VisionUpdateTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

// Recomputes the current visibility polygon and propagates the result to mask, surfaces, and targets.
void ULSVisionComponent::UpdateVisionPolygon()
{
	if (!IsLocalVisionController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr || GetOwner() == nullptr)
	{
		return;
	}

	ULSVisionSubsystem* VisionSubsystem = World->GetSubsystem<ULSVisionSubsystem>();
	if (VisionSubsystem == nullptr || VisionSubsystem->GetMaskRenderer() == nullptr)
	{
		return;
	}

	const FVector ActorLocation = GetOwner()->GetActorLocation();
	const FVector2D ActorLocation2D(ActorLocation.X, ActorLocation.Y);

	const FVector ActorForward = GetOwner()->GetActorForwardVector();
	const FVector2D ActorForward2D(ActorForward.X, ActorForward.Y);

	FLSVisionSolverInfo SolverInfo;
	SolverInfo.OriginPos = ActorLocation2D;
	SolverInfo.OriginForward = ActorForward2D.GetSafeNormal();
	SolverInfo.HalfFovDegrees = HalfFOVDegrees;
	SolverInfo.VisionRadius = VisionRadius;
	SolverInfo.AngleEpsilon = 0.01f;
	SolverInfo.DivideAngleDegree = DivideAngleDegree;
	SolverInfo.MaxRayDistance = MaxRayDistance;
	SolverInfo.World = World;

	for (ULSVisionOccluderComponent* Occluder : VisionSubsystem->GetRegisteredOccluders())
	{
		if (Occluder == nullptr)
		{
			continue;
		}

		for (FLSVisionSegment2D& Segment : Occluder->Segments)
		{
			SolverInfo.Segments.Add(&Segment);
		}
	}

	CurrentPolygon = FLSVisionSolver::Solve(SolverInfo);

	if (PostProcessMID != nullptr)
	{
		PostProcessMID->SetVectorParameterValue(MaskOriginParamName, FLinearColor(CurrentPolygon.Origin.X, CurrentPolygon.Origin.Y, 0.0f, 0.0f));
		PostProcessMID->SetScalarParameterValue(MaskExtentParamName, CurrentPolygon.Extent);

		if (UTextureRenderTarget2D* VisibilityMaskRT = VisionSubsystem->GetVisibilityMaskRenderTarget())
		{
			PostProcessMID->SetTextureParameterValue(VisibilityMaskTextureParamName, VisibilityMaskRT);
		}
	}

	VisionSubsystem->GetMaskRenderer()->RequestMaskUpdate(CurrentPolygon);

	for (ULSVisionSurfaceComponent* SurfaceComponent : VisionSubsystem->GetRegisteredVisionSurfaces())
	{
		if (SurfaceComponent != nullptr)
		{
			SurfaceComponent->ApplyVisionParameters(
				VisionSubsystem->GetVisibilityMaskRenderTarget(),
				FVector(CurrentPolygon.Origin.X, CurrentPolygon.Origin.Y, 0.0f),
				CurrentPolygon.Extent,
				SolverInfo.OriginForward);
		}
	}

	UpdateVisionTargets(SolverInfo.OriginPos);
}

// Limits vision simulation to the locally controlled pawn so remote pawns do not drive local rendering.
bool ULSVisionComponent::IsLocalVisionController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr || !OwnerPawn->IsPlayerControlled())
	{
		return false;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(OwnerPawn->GetController());
	return PlayerController != nullptr && PlayerController->IsLocalPlayerController();
}

// Checks whether a 2D point lies inside the latest solved visibility polygon.
bool ULSVisionComponent::IsPointVisibleInCurrentVision(const FVector2D& Point2D) const
{
	if (CurrentPolygon.Points.Num() < 3)
	{
		return false;
	}

	bool bInside = false;
	for (int32 CurrentIndex = 0, PreviousIndex = CurrentPolygon.Points.Num() - 1; CurrentIndex < CurrentPolygon.Points.Num(); PreviousIndex = CurrentIndex++)
	{
		const FVector2D& CurrentPoint = CurrentPolygon.Points[CurrentIndex];
		const FVector2D& PreviousPoint = CurrentPolygon.Points[PreviousIndex];

		const bool bCrossesHorizontalRay = ((CurrentPoint.Y > Point2D.Y) != (PreviousPoint.Y > Point2D.Y));
		if (!bCrossesHorizontalRay)
		{
			continue;
		}

		const float Denominator = PreviousPoint.Y - CurrentPoint.Y;
		if (FMath::IsNearlyZero(Denominator))
		{
			continue;
		}

		const float XIntersection = ((PreviousPoint.X - CurrentPoint.X) * (Point2D.Y - CurrentPoint.Y) / Denominator) + CurrentPoint.X;
		if (Point2D.X < XIntersection)
		{
			bInside = !bInside;
		}
	}

	return bInside;
}

// Updates registered targets so locally hidden actors can be culled outside the visible area.
void ULSVisionComponent::UpdateVisionTargets(const FVector2D& VisionOrigin2D)
{
	ULSVisionSubsystem* VisionSubsystem = GetWorld() ? GetWorld()->GetSubsystem<ULSVisionSubsystem>() : nullptr;
	if (VisionSubsystem == nullptr)
	{
		return;
	}

	for (ULSVisionTargetComponent* VisionTarget : VisionSubsystem->GetRegisteredVisionTargets())
	{
		if (VisionTarget == nullptr || VisionTarget->GetOwner() == GetOwner())
		{
			continue;
		}

		bool bVisible = false;
		TArray<FVector> SamplePoints;
		VisionTarget->GatherVisibilitySamplePoints(SamplePoints);

		for (const FVector& SamplePoint : SamplePoints)
		{
			const FVector2D SamplePoint2D(SamplePoint.X, SamplePoint.Y);
			const float Distance = FVector2D::Distance(VisionOrigin2D, SamplePoint2D);

			if (Distance > MaxRayDistance)
			{
				continue;
			}

			if (Distance < VisionRadius || IsPointVisibleInCurrentVision(SamplePoint2D))
			{
				bVisible = true;
				break;
			}
		}

		VisionTarget->SetLocallyVisible(bVisible);
	}
}
