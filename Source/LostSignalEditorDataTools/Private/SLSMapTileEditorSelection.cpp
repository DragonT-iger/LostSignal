#include "SLSMapTileEditor.h"

#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace
{
	int32 GetLSMapTilePresetMaterialCount(const FLSMapTilePaletteEntry& Preset)
	{
		return Preset.Materials.IsEmpty() && Preset.Mesh != nullptr
			? Preset.Mesh->GetStaticMaterials().Num()
			: Preset.Materials.Num();
	}

	UMaterialInterface* GetLSMapTilePresetMaterial(const FLSMapTilePaletteEntry& Preset, int32 Index)
	{
		return Preset.Materials.IsEmpty() && Preset.Mesh != nullptr
			? Preset.Mesh->GetMaterial(Index)
			: Preset.Materials.IsValidIndex(Index) ? Preset.Materials[Index].Get() : nullptr;
	}

	bool AreLSMapTilePresetsEqual(
		const FLSMapTilePaletteEntry& Left,
		const FLSMapTilePaletteEntry& Right)
	{
		const int32 MaterialCount = GetLSMapTilePresetMaterialCount(Left);
		if (Left.Mesh != Right.Mesh || MaterialCount != GetLSMapTilePresetMaterialCount(Right))
		{
			return false;
		}

		for (int32 Index = 0; Index < MaterialCount; ++Index)
		{
			if (GetLSMapTilePresetMaterial(Left, Index) != GetLSMapTilePresetMaterial(Right, Index))
			{
				return false;
			}
		}
		return true;
	}
}

void SLSMapTileEditor::CollectSelectedTilePresets(TArray<FLSMapTilePaletteEntry>& OutPresets) const
{
	for (FSelectionIterator It(*GEditor->GetSelectedActors()); It; ++It)
	{
		AActor* Actor = Cast<AActor>(*It);
		TArray<UStaticMeshComponent*> Components;
		if (Actor != nullptr)
		{
			Actor->GetComponents(Components);
		}
		for (UStaticMeshComponent* Component : Components)
		{
			if (Component == nullptr || Component->GetStaticMesh() == nullptr)
			{
				continue;
			}

			FLSMapTilePaletteEntry Preset;
			Preset.Mesh = Component->GetStaticMesh();
			for (int32 Index = 0; Index < Component->GetNumMaterials(); ++Index)
			{
				Preset.Materials.Add(Component->GetMaterial(Index));
			}
			UMaterialInterface* FirstMaterial = Preset.Materials.IsEmpty() ? nullptr : Preset.Materials[0].Get();
			Preset.DisplayName = FText::FromString(FirstMaterial != nullptr ? FirstMaterial->GetName() : Preset.Mesh->GetName());
			Preset.PreviewColor = MakePreviewColor(Preset);
			if (!OutPresets.ContainsByPredicate([&Preset](const FLSMapTilePaletteEntry& Existing)
				{ return AreLSMapTilePresetsEqual(Existing, Preset); }))
			{
				OutPresets.Add(MoveTemp(Preset));
			}
		}
	}
}

bool SLSMapTileEditor::AddTilePresetToPalette(
	ULSMapTilePalette& TilePalette,
	const FLSMapTilePaletteEntry& Preset) const
{
	if (Preset.Mesh == nullptr)
	{
		return false;
	}

	FLSMapTilePaletteEntry* Existing = TilePalette.Tiles.FindByPredicate(
		[&Preset](const FLSMapTilePaletteEntry& Entry) { return AreLSMapTilePresetsEqual(Entry, Preset); });
	if (Existing != nullptr)
	{
		if (Existing->Materials.IsEmpty() && !Preset.Materials.IsEmpty())
		{
			*Existing = Preset;
			return true;
		}
		return false;
	}

	TilePalette.Tiles.Add(Preset);
	return true;
}
