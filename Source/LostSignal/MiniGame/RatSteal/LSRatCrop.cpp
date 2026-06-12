#include "MiniGame/RatSteal/LSRatCrop.h"

#include "Components/BoxComponent.h"
#include "LostSignal.h"
#include "MiniGame/RatSteal/LSRatSpawnManager.h"
#include "MiniGame/RatSteal/LSRatYSortComponent.h"
#include "PaperFlipbookComponent.h"
#include "PaperSpriteComponent.h"

ALSRatCrop::ALSRatCrop()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(FVector(75.f, 10.f, 75.f));
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision); // S 단계부터 활성
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionBox->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionBox);

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(CollisionBox);
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 원작 작물 스케일 0.2 (512px 원본 → 약 102px)
	Sprite->SetRelativeScale3D(FVector(0.2f, 1.f, 0.2f));

	SparkleEffect = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("SparkleEffect"));
	SparkleEffect->SetupAttachment(CollisionBox);
	SparkleEffect->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SparkleEffect->SetRelativeScale3D(FVector(0.2f, 1.f, 0.2f));
	SparkleEffect->SetVisibility(false);

	YSort = CreateDefaultSubobject<ULSRatYSortComponent>(TEXT("YSort"));
	YSort->bStatic = true;
}

void ALSRatCrop::InitCrop(ALSRatSpawnManager* InSpawnManager, ELSRatFarmRank InRank, ELSRatCropType InType,
	const FVector& GrowSeconds, ELSRatCropSize InMaxSize, const TArray<TObjectPtr<UPaperSprite>>& StageSprites)
{
	SpawnManager = InSpawnManager;
	Rank = InRank;
	Type = InType;
	GrowTime = GrowSeconds;
	MaxSize = InMaxSize;
	Stages = StageSprites;

	SetStage(ELSRatCropSize::Born);
	bInitialized = true;
}

void ALSRatCrop::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bInitialized || Size >= MaxSize)
	{
		return;
	}

	Elapsed += DeltaSeconds;

	float Threshold = 0.f;
	switch (Size)
	{
	case ELSRatCropSize::Born: Threshold = GrowTime.X; break;
	case ELSRatCropSize::S:    Threshold = GrowTime.Y; break;
	case ELSRatCropSize::M:    Threshold = GrowTime.Z; break;
	default: return;
	}

	if (Elapsed >= Threshold)
	{
		Elapsed = 0.f;
		SetStage(static_cast<ELSRatCropSize>(static_cast<uint8>(Size) + 1));
	}
}

void ALSRatCrop::SetStage(ELSRatCropSize NewSize)
{
	Size = NewSize;

	const int32 StageIndex = static_cast<int32>(Size);
	if (Sprite && Stages.IsValidIndex(StageIndex) && Stages[StageIndex])
	{
		Sprite->SetSprite(Stages[StageIndex]);
	}
	else
	{
		UE_LOG(LogLS, Verbose, TEXT("[RatSteal] 작물 %s 단계 %d 스프라이트 없음 (Stages=%d)"),
			*GetName(), StageIndex, Stages.Num());
	}

	if (Size == ELSRatCropSize::S && CollisionBox)
	{
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	if (Size == ELSRatCropSize::L && SparkleEffect)
	{
		SparkleEffect->SetVisibility(true);
		SparkleEffect->Play();
	}
}

void ALSRatCrop::NotifyStolen()
{
	if (SpawnManager)
	{
		SpawnManager->NotifyCropRemoved(this);
	}
	Destroy();
}
