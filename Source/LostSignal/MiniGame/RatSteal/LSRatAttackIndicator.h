#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LSRatAttackIndicator.generated.h"

class UPaperSprite;
class UPaperSpriteComponent;

/**
 * 농부 공격 지시자 (구 AttackPattern indicator, redCircle.png 불투명도 0.5).
 * 표시 후 attackDelay(1.5s) 뒤 농부가 Execute로 타격 판정.
 */
UCLASS()
class LOSTSIGNAL_API ALSRatAttackIndicator : public AActor
{
	GENERATED_BODY()

public:
	ALSRatAttackIndicator();

protected:
	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<UPaperSpriteComponent> Sprite;

	/** redCircle 스프라이트 (에셋 임포트 후 할당) */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Visual")
	TObjectPtr<UPaperSprite> IndicatorSprite;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Visual")
	float Opacity = 0.5f;

	virtual void BeginPlay() override;
};
