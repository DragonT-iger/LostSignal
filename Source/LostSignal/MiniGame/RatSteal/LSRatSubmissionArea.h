#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LSRatSubmissionArea.generated.h"

class UBoxComponent;

/**
 * 제출존 (21_System_Score, 구 SubMissionArea).
 * 플레이어 진입 시 인벤토리 전량 정산 → 점수 → 베이비 포만 회복.
 * 원작은 맵 좌우 끝 x=±3580에 2개 배치, 박스 521x4320.
 */
UCLASS()
class LOSTSIGNAL_API ALSRatSubmissionArea : public AActor
{
	GENERATED_BODY()

public:
	ALSRatSubmissionArea();

protected:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<UBoxComponent> SubmitBox;
};
