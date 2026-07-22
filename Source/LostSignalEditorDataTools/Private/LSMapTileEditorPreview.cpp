#include "LSMapTileEditorModel.h"

#include "Engine/StaticMeshActor.h"
#include "LSMapTilePalette.h"
#include "UObject/Package.h"

void FLSMapTileEditorModel::RefreshHoverPreview()
{
	ClearHoverPreview();
	const FLSMapTileEditorCell* Cell = HoveredCell.IsSet() ? Cells.Find(HoveredCell.GetValue()) : nullptr;
	AStaticMeshActor* Actor = Cell != nullptr ? Cell->Actor.Get() : nullptr;
	UStaticMeshComponent* SourceComponent = Actor != nullptr ? Actor->GetStaticMeshComponent() : nullptr;
	UWorld* World = GetEditorWorld();
	if (SourceComponent == nullptr || World == nullptr)
	{
		return;
	}

	const FBox SourceBox = SourceComponent->CalcBounds(SourceComponent->GetComponentTransform()).GetBox();
	HoverHighlightComponent.Reset(NewObject<UBoxComponent>(GetTransientPackage(), NAME_None, RF_Transient));
	HoverHighlightComponent->SetBoxExtent(SourceBox.GetExtent() + FVector(8.0));
	HoverHighlightComponent->SetWorldLocation(SourceBox.GetCenter());
	HoverHighlightComponent->ShapeColor = FColor::Yellow;
	HoverHighlightComponent->bDrawOnlyIfSelected = false;
	HoverHighlightComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HoverHighlightComponent->RegisterComponentWithWorld(World);

	const FLSMapTilePaletteEntry* ActiveTile = GetActiveTile();
	if (ActiveTile == nullptr || ActiveTile->Mesh == nullptr)
	{
		return;
	}

	HoverPreviewComponent.Reset(NewObject<UStaticMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient));
	HoverPreviewComponent->SetMobility(EComponentMobility::Movable);
	HoverPreviewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HoverPreviewComponent->SetCastShadow(false);
	HoverPreviewComponent->SetStaticMesh(ActiveTile->Mesh.Get());
	for (int32 Index = 0; Index < ActiveTile->Materials.Num(); ++Index)
	{
		HoverPreviewComponent->SetMaterial(Index, ActiveTile->Materials[Index].Get());
	}

	FTransform PreviewTransform = Actor->GetActorTransform();
	FRotator PreviewRotation = Actor->GetActorRotation();
	PreviewRotation.Yaw = PaintYawDegrees;
	PreviewTransform.SetRotation(PreviewRotation.Quaternion());
	const double PreviewTopZ = HoverPreviewComponent->CalcBounds(PreviewTransform).GetBox().Max.Z;
	FVector PreviewLocation = PreviewTransform.GetLocation();
	PreviewLocation.Z += bAlignTopSurface ? SourceBox.Max.Z - PreviewTopZ : 0.0;
	PreviewLocation.Z += 1.0;
	PreviewTransform.SetLocation(PreviewLocation);
	HoverPreviewComponent->SetWorldTransform(PreviewTransform);
	HoverPreviewComponent->RegisterComponentWithWorld(World);
}

void FLSMapTileEditorModel::ClearHoverPreview()
{
	if (HoverPreviewComponent.IsValid())
	{
		HoverPreviewComponent->UnregisterComponent();
		HoverPreviewComponent.Reset();
	}
	if (HoverHighlightComponent.IsValid())
	{
		HoverHighlightComponent->UnregisterComponent();
		HoverHighlightComponent.Reset();
	}
}
