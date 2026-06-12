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
	HideBox->SetBoxExtent(FVector(80.f, 10.f, 80.f));
	HideBox->SetRelativeLocation(FVector(0.f, 0.f, -40.f));
	HideBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HideBox->SetCollisionObjectType(ECC_WorldStatic);
	HideBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	HideBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HideBox->SetGenerateOverlapEvents(true);
	SetRootComponent(HideBox);

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(HideBox);
	Sprite->SetRelativeLocation(FVector(0.f, -2.f, 40.f));
	Sprite->SetRelativeScale3D(FVector(0.45f, 1.f, 0.45f));
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	YSort = CreateDefaultSubobject<ULSRatYSortComponent>(TEXT("YSort"));
	YSort->bStatic = true;
	YSort->SortOffset = 10;

	HideBox->OnComponentBeginOverlap.AddDynamic(this, &ALSRatBush::OnOverlapBegin);
	HideBox->OnComponentEndOverlap.AddDynamic(this, &ALSRatBush::OnOverlapEnd);
}

void ALSRatBush::BeginPlay()
{
	Super::BeginPlay();

	if (YSort)
	{
		YSort->SortOffset = 10;
	}

	if (HideBox)
	{
		HideBox->SetBoxExtent(FVector(80.f, 10.f, 80.f));
		HideBox->SetRelativeLocation(FVector(0.f, 0.f, -40.f));
	}

	if (Sprite)
	{
		Sprite->SetRelativeLocation(FVector(0.f, -2.f, 40.f));
		Sprite->SetRelativeScale3D(FVector(0.45f, 1.f, 0.45f));
		Sprite->SetSpriteColor(FLinearColor::White);
	}
}

void ALSRatBush::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ALSRatPlayer* Player = Cast<ALSRatPlayer>(OtherActor))
	{
		Player->EnterBush();
	}
}

void ALSRatBush::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (ALSRatPlayer* Player = Cast<ALSRatPlayer>(OtherActor))
	{
		Player->ExitBush();
	}
}
