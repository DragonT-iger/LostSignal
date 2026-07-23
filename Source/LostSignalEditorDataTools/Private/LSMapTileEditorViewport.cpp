#include "LSMapTileEditorModel.h"

#include "Editor.h"
#include "Engine/StaticMeshActor.h"
#include "LevelEditorViewport.h"

namespace
{
	FLevelEditorViewportClient* ResolveMapTileLevelViewportClient()
	{
		if (GEditor == nullptr)
		{
			return nullptr;
		}

		FViewport* ActiveViewport = GEditor->GetActiveViewport();
		FViewportClient* ActiveViewportClient = ActiveViewport != nullptr ? ActiveViewport->GetClient() : nullptr;
		for (FLevelEditorViewportClient* ViewportClient : GEditor->GetLevelViewportClients())
		{
			if (ViewportClient != nullptr && ViewportClient == ActiveViewportClient)
			{
				return ViewportClient;
			}
		}

		for (FLevelEditorViewportClient* ViewportClient : GEditor->GetLevelViewportClients())
		{
			if (ViewportClient != nullptr && ViewportClient->IsPerspective())
			{
				return ViewportClient;
			}
		}
		return nullptr;
	}
}

void FLSMapTileEditorModel::FocusViewportOnSelection(
	const FIntPoint& StartCell,
	const FIntPoint& EndCell) const
{
	if (!bLockViewportToSelection)
	{
		return;
	}

	const FLSMapTileEditorCell* Start = Cells.Find(StartCell);
	const FLSMapTileEditorCell* End = Cells.Find(EndCell);
	const AStaticMeshActor* StartActor = Start != nullptr ? Start->Actor.Get() : nullptr;
	const AStaticMeshActor* EndActor = End != nullptr ? End->Actor.Get() : nullptr;
	FLevelEditorViewportClient* ViewportClient = ResolveMapTileLevelViewportClient();
	if (StartActor == nullptr || EndActor == nullptr || ViewportClient == nullptr)
	{
		return;
	}

	const FVector TargetLocation = (StartActor->GetActorLocation() + EndActor->GetActorLocation()) * 0.5;
	const double CurrentHeight = FMath::Abs(ViewportClient->GetViewLocation().Z - TargetLocation.Z);
	const double CameraHeight = FMath::Max(CurrentHeight, static_cast<double>(GridSize) * 4.0);
	ViewportClient->SetViewportType(LVT_Perspective);
	ViewportClient->SetLookAtLocation(TargetLocation, false);
	ViewportClient->SetViewLocation(TargetLocation + FVector(0.0, 0.0, CameraHeight));
	ViewportClient->SetViewRotation(FRotator(-90.0, 0.0, 0.0));
	ViewportClient->Invalidate();
}
