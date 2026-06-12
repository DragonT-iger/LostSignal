#include "MiniGame/RatSteal/LSRatAttackIndicator.h"

#include "PaperSpriteComponent.h"

ALSRatAttackIndicator::ALSRatAttackIndicator()
{
	PrimaryActorTick.bCanEverTick = false;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 바닥 표시라 항상 캐릭터 뒤에 그려지도록 낮은 우선순위
	Sprite->SetTranslucentSortPriority(-100000);
	SetRootComponent(Sprite);
}

void ALSRatAttackIndicator::BeginPlay()
{
	Super::BeginPlay();

	if (Sprite)
	{
		if (IndicatorSprite)
		{
			Sprite->SetSprite(IndicatorSprite);
		}
		Sprite->SetSpriteColor(FLinearColor(1.f, 0.f, 0.f, Opacity));
	}
}
