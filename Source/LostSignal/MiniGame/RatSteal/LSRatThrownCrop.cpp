#include "MiniGame/RatSteal/LSRatThrownCrop.h"

#include "MiniGame/RatSteal/LSRatYSortComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"

ALSRatThrownCrop::ALSRatThrownCrop()
{
	PrimaryActorTick.bCanEverTick = true;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sprite->SetRelativeScale3D(FVector(0.22f, 1.f, 0.22f));
	SetRootComponent(Sprite);

	YSort = CreateDefaultSubobject<ULSRatYSortComponent>(TEXT("YSort"));
	YSort->SortOffset = 60;
}

void ALSRatThrownCrop::InitThrownCrop(UPaperSprite* SpriteAsset, const FVector& Start, const FVector& Direction)
{
	StartLocation = Start;
	const FVector SafeDirection = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction.GetSafeNormal();
	EndLocation = StartLocation + SafeDirection * TravelDistance;
	SetActorLocation(StartLocation);

	if (SpriteAsset)
	{
		Sprite->SetSprite(SpriteAsset);
	}
}

void ALSRatThrownCrop::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Elapsed += DeltaSeconds;
	const float Alpha = Duration > 0.f ? FMath::Clamp(Elapsed / Duration, 0.f, 1.f) : 1.f;
	FVector Location = FMath::Lerp(StartLocation, EndLocation, Alpha);
	Location.Z += FMath::Sin(Alpha * PI) * ArcHeight;
	SetActorLocation(Location);

	if (Alpha >= 1.f)
	{
		Destroy();
	}
}
