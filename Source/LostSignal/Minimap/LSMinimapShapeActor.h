#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Minimap/LSMinimapTypes.h"
#include "LSMinimapShapeActor.generated.h"

UCLASS()
class LOSTSIGNAL_API ALSMinimapShapeActor : public AActor
{
	GENERATED_BODY()

public:
	ALSMinimapShapeActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FLSMinimapShapeSnapshot BuildSnapshot() const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	ELSMinimapShapeType ShapeType = ELSMinimapShapeType::Box;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true", ClampMin="1.0"))
	FVector2D Extent = FVector2D(300.0f, 300.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	TArray<FVector> PolylinePoints;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	FLinearColor FillColor = FLinearColor(0.42f, 0.1f, 0.85f, 0.45f);
};
