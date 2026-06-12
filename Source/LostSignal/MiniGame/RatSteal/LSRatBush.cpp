#include "MiniGame/RatSteal/LSRatBush.h"

#include "Components/BoxComponent.h"
#include "MiniGame/RatSteal/LSRatPlayer.h"
#include "MiniGame/RatSteal/LSRatYSortComponent.h"
#include "PaperSpriteComponent.h"

ALSRatBush::ALSRatBush()
{
	PrimaryActorTick.bCanEverTick = false;

	// 원작 Bush: 200x200, offset (0, -50)
	HideBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HideBox"));
	HideBox->SetBoxExtent(FVector(100.f, 10.f, 100.f));
	HideBox->SetRelativeLocation(FVector(0.f, 0.f, -50.f));
	HideBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HideBox->SetCollisionObjectType(ECC_WorldStatic);
	HideBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	HideBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HideBox->SetGenerateOverlapEvents(true);
	SetRootComponent(HideBox);

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(HideBox);
	Sprite->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	YSort = CreateDefaultSubobject<ULSRatYSortComponent>(TEXT("YSort"));
	YSort->bStatic = true;

	HideBox->OnComponentBeginOverlap.AddDynamic(this, &ALSRatBush::OnOverlapBegin);
	HideBox->OnComponentEndOverlap.AddDynamic(this, &ALSRatBush::OnOverlapEnd);
}

void ALSRatBush::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ALSRatPlayer* Player = Cast<ALSRatPlayer>(OtherActor))
	{
		Player->EnterBush();
		if (Sprite)
		{
			Sprite->SetSpriteColor(FLinearColor(1.f, 1.f, 1.f, HiddenOpacity));
		}
	}
}

void ALSRatBush::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ALSRatPlayer* Player = Cast<ALSRatPlayer>(OtherActor))
	{
		Player->ExitBush();
		if (Sprite)
		{
			Sprite->SetSpriteColor(FLinearColor::White);
		}
	}
}
