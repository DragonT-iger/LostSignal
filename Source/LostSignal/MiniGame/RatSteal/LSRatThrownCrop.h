#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LSRatThrownCrop.generated.h"

class UPaperSprite;
class UPaperSpriteComponent;
class ULSRatYSortComponent;

UCLASS()
class LOSTSIGNAL_API ALSRatThrownCrop : public AActor
{
	GENERATED_BODY()

public:
	ALSRatThrownCrop();

	virtual void Tick(float DeltaSeconds) override;

	void InitThrownCrop(UPaperSprite* SpriteAsset, const FVector& Start, const FVector& Direction);

private:
	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<UPaperSpriteComponent> Sprite;

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<ULSRatYSortComponent> YSort;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Visual")
	float TravelDistance = 520.f;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Visual")
	float ArcHeight = 220.f;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Visual")
	float Duration = 0.65f;

	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;
	float Elapsed = 0.f;
};
