#pragma once

#include "CoreMinimal.h"
#include "MiniGame/RatSteal/LSRatGameMode.h"
#include "LSRatTutorialGameMode.generated.h"

/**
 * 튜토리얼 씬용 GameMode (32_Tutorial).
 * 학습 방해 방지를 위해 3분 타이머와 포만 감소를 끈다. 그 외 규칙은 동일.
 * 단계 진행/안내 문구는 WBP_RatStealTutorial 쪽에서 제어한다.
 */
UCLASS()
class LOSTSIGNAL_API ALSRatTutorialGameMode : public ALSRatGameMode
{
	GENERATED_BODY()

public:
	ALSRatTutorialGameMode();
};
