#include "MiniGame/RatSteal/LSRatSpawnManager.h"

#include "LostSignal.h"
#include "MiniGame/RatSteal/LSRatCrop.h"
#include "MiniGame/RatSteal/LSRatGameMode.h"

ALSRatSpawnManager::ALSRatSpawnManager()
{
	PrimaryActorTick.bCanEverTick = true;

	CropClass = ALSRatCrop::StaticClass();

	// 원작 SpawnManager::Awake 기본값
	FLSRatFarmConfig FarmA;
	FarmA.Rank = ELSRatFarmRank::RankA;
	FarmA.HalfExtent = FVector2D(1070.f, 720.f);
	FarmA.MaxRate = 20;
	FarmA.SpawnInterval = 3.f;
	FarmA.GrowSeconds = FVector(5.f, 7.f, 7.f);

	FLSRatFarmConfig FarmB;
	FarmB.Rank = ELSRatFarmRank::RankB;
	FarmB.HalfExtent = FVector2D(2130.f, 1330.f);
	FarmB.MaxRate = 15;
	FarmB.SpawnInterval = 4.f;
	FarmB.GrowSeconds = FVector(5.f, 7.f, 12.f);

	FLSRatFarmConfig FarmC;
	FarmC.Rank = ELSRatFarmRank::RankC;
	FarmC.HalfExtent = FVector2D(3210.f, 2110.f);
	FarmC.MaxRate = 10;
	FarmC.SpawnInterval = 5.f;
	FarmC.GrowSeconds = FVector(5.f, 12.f, 0.f); // C랭크는 M 최대 → M→L 미사용

	Farms = { FarmA, FarmB, FarmC };
}

void ALSRatSpawnManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const ALSRatGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSRatGameMode>();
	if (GameMode && !GameMode->IsPlaying())
	{
		return;
	}

	for (int32 Index = 0; Index < Farms.Num(); ++Index)
	{
		FLSRatFarmConfig& Farm = Farms[Index];
		if (Farm.Crops.Num() >= Farm.MaxRate)
		{
			continue;
		}

		Farm.ElapsedTime += DeltaSeconds;
		if (Farm.ElapsedTime < Farm.SpawnInterval)
		{
			continue;
		}
		Farm.ElapsedTime = 0.f;

		// 안쪽 제외 영역: A는 Home, B는 A 영역, C는 B 영역 (원작 도넛 스폰)
		const FVector2D InnerHalfExtent = (Index == 0) ? HomeHalfExtent : Farms[Index - 1].HalfExtent;

		if (ALSRatCrop* NewCrop = SpawnCrop(Farm, InnerHalfExtent))
		{
			Farm.Crops.Add(NewCrop);
		}
	}
}

ALSRatCrop* ALSRatSpawnManager::SpawnCrop(FLSRatFarmConfig& Farm, const FVector2D& InnerHalfExtent)
{
	if (!CropClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[RatSteal] SpawnManager: CropClass 미할당"));
		return nullptr;
	}

	FVector2D Point;
	if (!FindSpawnPoint(Farm, InnerHalfExtent, Point))
	{
		return nullptr; // 자리가 없으면 이번 주기는 건너뜀 (원작 do-while 무한루프 방지)
	}

	const ELSRatCropType Type = PickCropType(Farm.CropProbability);
	if (Type == ELSRatCropType::None)
	{
		UE_LOG(LogLS, Warning, TEXT("[RatSteal] SpawnManager: 작물 확률 합이 100이 아님"));
		return nullptr;
	}

	const FVector Origin = GetActorLocation();
	const FVector SpawnLocation(Origin.X + Point.X, Origin.Y, Origin.Z + Point.Y);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ALSRatCrop* Crop = GetWorld()->SpawnActor<ALSRatCrop>(CropClass, SpawnLocation, FRotator::ZeroRotator, Params);
	if (!Crop)
	{
		return nullptr;
	}

	// C랭크는 최대 M (12_Entity_Crop)
	const ELSRatCropSize MaxSize = (Farm.Rank == ELSRatFarmRank::RankC) ? ELSRatCropSize::M : ELSRatCropSize::L;

	TArray<TObjectPtr<UPaperSprite>> Stages;
	if (const FLSRatCropVisualSet* Visuals = CropVisuals.Find(Type))
	{
		Stages = Visuals->StageSprites;
	}

	Crop->InitCrop(this, Farm.Rank, Type, Farm.GrowSeconds, MaxSize, Stages);
	UE_LOG(LogLS, Verbose, TEXT("[RatSteal] 작물 스폰 %s rank=%d at (%.0f, %.0f) sprites=%d"),
		*UEnum::GetValueAsString(Type), static_cast<int32>(Farm.Rank),
		SpawnLocation.X, SpawnLocation.Z, Stages.Num());
	return Crop;
}

bool ALSRatSpawnManager::FindSpawnPoint(const FLSRatFarmConfig& Farm, const FVector2D& InnerHalfExtent, FVector2D& OutPoint) const
{
	constexpr int32 MaxAttempts = 64;

	for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
	{
		const FVector2D Candidate(
			FMath::FRandRange(-Farm.HalfExtent.X, Farm.HalfExtent.X),
			FMath::FRandRange(-Farm.HalfExtent.Y, Farm.HalfExtent.Y));

		// 안쪽 제외 영역 + 여유(원작 IsInnerRect의 ±50)
		const bool bInsideInner =
			FMath::Abs(Candidate.X) <= InnerHalfExtent.X + InnerMargin &&
			FMath::Abs(Candidate.Y) <= InnerHalfExtent.Y + InnerMargin;
		if (bInsideInner)
		{
			continue;
		}

		// 같은 랭크 기존 작물과 spawnRange(200) 이상 간격 (원작 CheckRange)
		bool bTooClose = false;
		const FVector Origin = GetActorLocation();
		const FVector CandidateWorld(Origin.X + Candidate.X, Origin.Y, Origin.Z + Candidate.Y);
		for (const ALSRatCrop* Crop : Farm.Crops)
		{
			if (Crop && FVector::Dist(Crop->GetActorLocation(), CandidateWorld) <= SpawnRange)
			{
				bTooClose = true;
				break;
			}
		}
		if (bTooClose)
		{
			continue;
		}

		OutPoint = Candidate;
		return true;
	}

	return false;
}

ELSRatCropType ALSRatSpawnManager::PickCropType(const FIntVector& Probability) const
{
	// 원작 RandomCrop(가지, 감자, 호박) — 매개변수 합이 100이어야 함
	const int32 EggplantCut = Probability.X;
	const int32 PotatoCut = EggplantCut + Probability.Y;
	const int32 PumpkinCut = PotatoCut + Probability.Z;

	const int32 Random = FMath::RandRange(0, 99);

	if (Random <= EggplantCut)
	{
		return ELSRatCropType::Eggplant;
	}
	if (Random <= PotatoCut)
	{
		return ELSRatCropType::Potato;
	}
	if (Random <= PumpkinCut)
	{
		return ELSRatCropType::Pumpkin;
	}
	return ELSRatCropType::None;
}

void ALSRatSpawnManager::NotifyCropRemoved(ALSRatCrop* Crop)
{
	for (FLSRatFarmConfig& Farm : Farms)
	{
		if (Farm.Crops.Remove(Crop) > 0)
		{
			return;
		}
	}
}
