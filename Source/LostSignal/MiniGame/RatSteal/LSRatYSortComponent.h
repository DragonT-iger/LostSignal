#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSRatYSortComponent.generated.h"

/**
 * 2D 깊이 정렬 (23_System_Camera).
 * 원작 YSort: OrderInLayer = -WorldY. UE에선 X-Z 평면을 쓰므로
 * TranslucencySortPriority = -WorldZ 로 화면 아래쪽이 앞에 그려진다.
 */
UCLASS(ClassGroup = (LS), meta = (BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSRatYSortComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSRatYSortComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 고정 오브젝트(작물/부쉬)는 true로 두고 첫 프레임만 정렬 */
	UPROPERTY(EditAnywhere, Category = "LS/RatSteal")
	bool bStatic = false;

	/** 정렬 보정값 (원작 YSort offset) */
	UPROPERTY(EditAnywhere, Category = "LS/RatSteal")
	int32 SortOffset = 0;

private:
	void Apply();
};
