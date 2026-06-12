#include "MiniGame/RatSteal/LSRatSubmissionArea.h"

#include "Components/BoxComponent.h"
#include "MiniGame/RatSteal/LSRatPlayer.h"

ALSRatSubmissionArea::ALSRatSubmissionArea()
{
	PrimaryActorTick.bCanEverTick = false;

	// 원작 521 x 4320 (세로로 긴 벽 전체)
	SubmitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SubmitBox"));
	SubmitBox->SetBoxExtent(FVector(260.5f, 10.f, 2160.f));
	SubmitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SubmitBox->SetCollisionObjectType(ECC_WorldStatic);
	SubmitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	SubmitBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SubmitBox->SetGenerateOverlapEvents(true);
	SetRootComponent(SubmitBox);

	SubmitBox->OnComponentBeginOverlap.AddDynamic(this, &ALSRatSubmissionArea::OnOverlapBegin);
}

void ALSRatSubmissionArea::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ALSRatPlayer* Player = Cast<ALSRatPlayer>(OtherActor))
	{
		Player->SubmitAndFeed();
	}
}
