#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MiniGame/RatSteal/LSRatTypes.h"
#include "LSRatCrop.generated.h"

class UBoxComponent;
class UPaperFlipbookComponent;
class UPaperSprite;
class UPaperSpriteComponent;
class ULSRatYSortComponent;
class ALSRatSpawnManager;

/**
 * 농작물 (12_Entity_Crop).
 * Born→S→M→L 시간 성장. S부터 콜라이더 활성(훔치기 가능), L 도달 시 반짝임 이펙트.
 * C랭크 밭은 M까지만 성장.
 */
UCLASS()
class LOSTSIGNAL_API ALSRatCrop : public AActor
{
	GENERATED_BODY()

public:
	ALSRatCrop();

	virtual void Tick(float DeltaSeconds) override;

	/** 스폰 매니저가 주입 (구 SetCropData). StageSprites는 Born/S/M/L 4장 */
	void InitCrop(ALSRatSpawnManager* InSpawnManager, ELSRatFarmRank InRank, ELSRatCropType InType,
		const FVector& GrowSeconds, ELSRatCropSize InMaxSize, const TArray<TObjectPtr<UPaperSprite>>& StageSprites);

	/** 플레이어가 훔쳤을 때. 스폰 매니저 목록에서 제거 후 파괴 */
	void NotifyStolen();

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	ELSRatCropType GetCropType() const { return Type; }

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	ELSRatCropSize GetCropSize() const { return Size; }

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	ELSRatFarmRank GetFarmRank() const { return Rank; }

	/** 다 자란(S 이상) 작물만 훔칠 수 있다 */
	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	bool IsStealable() const { return Size >= ELSRatCropSize::S; }

protected:
	void SetStage(ELSRatCropSize NewSize);

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<UPaperSpriteComponent> Sprite;

	/** L 도달 시 활성화되는 반짝임 (crop_sparkle) */
	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<UPaperFlipbookComponent> SparkleEffect;

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<ULSRatYSortComponent> YSort;

private:
	UPROPERTY()
	TObjectPtr<ALSRatSpawnManager> SpawnManager;

	UPROPERTY()
	TArray<TObjectPtr<UPaperSprite>> Stages;

	ELSRatCropType Type = ELSRatCropType::None;
	ELSRatCropSize Size = ELSRatCropSize::Born;
	ELSRatCropSize MaxSize = ELSRatCropSize::L;
	ELSRatFarmRank Rank = ELSRatFarmRank::RankA;

	// 단계별 성장 시간 (X=Born→S, Y=S→M, Z=M→L)
	FVector GrowTime = FVector::ZeroVector;
	float Elapsed = 0.f;
	bool bInitialized = false;
};
