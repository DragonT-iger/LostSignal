#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LSMapTilePalette.generated.h"

class UStaticMesh;
class UMaterialInterface;

USTRUCT()
struct LOSTSIGNALEDITORDATATOOLS_API FLSMapTilePaletteEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/MapTileTool")
	FText DisplayName;

	UPROPERTY(EditAnywhere, Category="LS/MapTileTool")
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, Category="LS/MapTileTool")
	TArray<TObjectPtr<UMaterialInterface>> Materials;

	UPROPERTY(EditAnywhere, Category="LS/MapTileTool")
	FLinearColor PreviewColor = FLinearColor(0.18f, 0.32f, 0.55f, 1.0f);
};

UCLASS()
class LOSTSIGNALEDITORDATATOOLS_API ULSMapTilePalette : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="LS/MapTileTool")
	TArray<FLSMapTilePaletteEntry> Tiles;
};
