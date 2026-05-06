#pragma once

#include "CoreMinimal.h"
#include "Core/LSGameModeBase.h"
#include "LSResultGameMode.generated.h"

UCLASS()
class LOSTSIGNAL_API ALSResultGameMode : public ALSGameModeBase
{
	GENERATED_BODY()

public:
	// 결과 확인 후 로비로 복귀 — UI 버튼에서 호출
	UFUNCTION(BlueprintCallable, Category="LS/Result")
	void ReturnToLobby();
};
