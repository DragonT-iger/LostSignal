#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LSCombatSettings.generated.h"

class UCurveFloat;

/** 전투 공용 튜닝 값. 넉백/끌어당김 지속시간·감속 커브는 모든 스킬·몬스터 액션이 여기 값을 공유한다(속도만 각 데이터의 CC_Value). */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "LS Combat"))
class LOSTSIGNAL_API ULSCombatSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

	// 모든 넉백/끌어당김 공용 지속시간(초). 0이면 넉백이 적용되지 않는다.
	UPROPERTY(Config, EditAnywhere, Category = "LS/Combat|Knockback", meta = (ClampMin = "0.0"))
	float KnockbackDuration = 0.2f;

	// 모든 넉백/끌어당김 공용 감속 커브(X=정규화 시간 0~1, Y=속도 배율). 미할당이면 등속.
	UPROPERTY(Config, EditAnywhere, Category = "LS/Combat|Knockback")
	TSoftObjectPtr<UCurveFloat> KnockbackStrengthCurve;
};
