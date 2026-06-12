#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LSRatBush.generated.h"

class UBoxComponent;
class UPaperSpriteComponent;
class ULSRatYSortComponent;

/**
 * 은신용 부쉬 (15_Mechanic_Stealth).
 * 플레이어 진입 시 Hide(Farmer 추적 해제), 부쉬는 반투명(0.7) 표시.
 */
UCLASS()
class LOSTSIGNAL_API ALSRatBush : public AActor
{
	GENERATED_BODY()

public:
	ALSRatBush();

protected:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<UBoxComponent> HideBox;

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<UPaperSpriteComponent> Sprite;

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<ULSRatYSortComponent> YSort;

	/** 플레이어가 안에 있을 때 부쉬 투명도 (원작 0.7) */
	UPROPERTY(EditAnywhere, Category = "LS/RatSteal|Balance")
	float HiddenOpacity = 0.7f;
};
