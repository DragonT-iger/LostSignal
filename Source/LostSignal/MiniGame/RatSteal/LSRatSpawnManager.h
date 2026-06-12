#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MiniGame/RatSteal/LSRatTypes.h"
#include "LSRatSpawnManager.generated.h"

class UPaperFlipbook;
class UPaperSprite;
class ALSRatCrop;

/** 작물 종류별 단계 스프라이트 (Born/S/M/L 4장, 원작 plant→S→M→L) */
USTRUCT(BlueprintType)
struct FLSRatCropVisualSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "LS/RatSteal")
	TArray<TObjectPtr<UPaperSprite>> StageSprites;
};

/** 밭 1개 스폰 설정 (원작 FarmData + 구역 RECT) */
USTRUCT(BlueprintType)
struct FLSRatFarmConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "LS/RatSteal")
	ELSRatFarmRank Rank = ELSRatFarmRank::RankA;

	/** 밭 영역 절반 크기 (중심 = 스폰 매니저 위치, 원작 farm RECT) */
	UPROPERTY(EditAnywhere, Category = "LS/RatSteal")
	FVector2D HalfExtent = FVector2D(1070.f, 720.f);

	/** 최대 작물 수 (원작 maxRate: A20 / B15 / C10) */
	UPROPERTY(EditAnywhere, Category = "LS/RatSteal")
	int32 MaxRate = 20;

	UPROPERTY(EditAnywhere, Category = "LS/RatSteal")
	int32 InitialSpawnCount = 8;

	/** 스폰 간격 초 (원작 spawnTime: A3 / B4 / C5) */
	UPROPERTY(EditAnywhere, Category = "LS/RatSteal")
	float SpawnInterval = 3.f;

	/** 단계별 성장 시간 X=Born→S, Y=S→M, Z=M→L (원작 GrowSpeed) */
	UPROPERTY(EditAnywhere, Category = "LS/RatSteal")
	FVector GrowSeconds = FVector(5.f, 7.f, 7.f);

	/** 종류 확률 % (가지/감자/호박 합 100, 원작 RandomCrop 34/33/33) */
	UPROPERTY(EditAnywhere, Category = "LS/RatSteal")
	FIntVector CropProbability = FIntVector(34, 33, 33);

	float ElapsedTime = 0.f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ALSRatCrop>> Crops;
};

/**
 * 농작물 스폰 매니저 (20_System_Spawn, 구 SpawnManager).
 * 액터 위치를 원점으로 A/B/C 동심 링 구조의 밭에 작물을 생성한다.
 * (B는 A 영역 제외, C는 B 영역 제외, A는 Home 제외 — 원작 도넛 스폰)
 */
UCLASS()
class LOSTSIGNAL_API ALSRatSpawnManager : public AActor
{
	GENERATED_BODY()

public:
	ALSRatSpawnManager();

	virtual void Tick(float DeltaSeconds) override;

	/** 작물이 훔쳐졌을 때 목록 정리 */
	void NotifyCropRemoved(ALSRatCrop* Crop);

protected:
	virtual void BeginPlay() override;

	ALSRatCrop* SpawnCrop(FLSRatFarmConfig& Farm, const FVector2D& InnerHalfExtent);
	void SpawnInitialCrops();
	bool FindSpawnPoint(const FLSRatFarmConfig& Farm, const FVector2D& InnerHalfExtent, FVector2D& OutPoint) const;
	bool HasRequiredStageSprites(const TArray<TObjectPtr<UPaperSprite>>& Stages, ELSRatCropSize MaxSize, ELSRatCropType Type) const;
	ELSRatCropType PickCropType(const FIntVector& Probability) const;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal")
	TSubclassOf<ALSRatCrop> CropClass;

	/** 밭 A/B/C (원작 farm_A ±1070x720 / farm_B ±2130x1330 / farm_C ±3210x2110) */
	UPROPERTY(EditAnywhere, Category = "LS/RatSteal|Balance")
	TArray<FLSRatFarmConfig> Farms;

	/** 중앙 Home 제외 영역 절반 크기 (원작 ±50) */
	UPROPERTY(EditAnywhere, Category = "LS/RatSteal|Balance")
	FVector2D HomeHalfExtent = FVector2D(50.f, 50.f);

	/** 기존 작물과의 최소 간격 (원작 spawnRange 200) */
	UPROPERTY(EditAnywhere, Category = "LS/RatSteal|Balance")
	float SpawnRange = 200.f;

	/** 내부 제외 영역 경계 여유 (원작 IsInnerRect ±50) */
	UPROPERTY(EditAnywhere, Category = "LS/RatSteal|Balance")
	float InnerMargin = 50.f;

	/** 종류별 단계 스프라이트 (에셋 임포트 후 할당) */
	UPROPERTY(EditAnywhere, Category = "LS/RatSteal|Visual")
	TMap<ELSRatCropType, FLSRatCropVisualSet> CropVisuals;
};
