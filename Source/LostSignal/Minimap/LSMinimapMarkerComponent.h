#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Minimap/LSMinimapTypes.h"
#include "LSMinimapMarkerComponent.generated.h"

UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSMinimapMarkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSMinimapMarkerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FLSMinimapMarkerSnapshot BuildSnapshot() const;

	void SetMarkerType(ELSMinimapMarkerType InMarkerType) { MarkerType = InMarkerType; }
	void SetMarkerColor(const FLinearColor& InMarkerColor) { MarkerColor = InMarkerColor; }
	void SetMinimapVisible(bool bInVisible) { bVisibleOnMinimap = bInVisible; }

	ELSMinimapMarkerType GetMarkerType() const { return MarkerType; }
	bool IsMinimapVisible() const { return bVisibleOnMinimap; }
	bool IsAlwaysVisible() const { return bAlwaysVisible; }

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	ELSMinimapMarkerType MarkerType = ELSMinimapMarkerType::Loot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	FLinearColor MarkerColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true", ClampMin="1.0"))
	float DrawRadius = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	bool bAlwaysVisible = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	bool bVisibleOnMinimap = true;
};
