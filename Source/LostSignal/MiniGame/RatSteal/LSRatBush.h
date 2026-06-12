#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LSRatBush.generated.h"

class UBoxComponent;
class UPaperSpriteComponent;
class ULSRatYSortComponent;

/**
 * 은신용 부쉬(15_Mechanic_Stealth).
 * 플레이어 진입 시 Hide 상태가 되며, 시각 피드백은 플레이어 알파로만 처리한다.
 */
UCLASS()
class LOSTSIGNAL_API ALSRatBush : public AActor
{
	GENERATED_BODY()

public:
	ALSRatBush();

protected:
	virtual void BeginPlay() override;

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
};
