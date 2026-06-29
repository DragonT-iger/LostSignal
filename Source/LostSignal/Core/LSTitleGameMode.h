#pragma once

#include "CoreMinimal.h"
#include "Core/LSGameModeBase.h"
#include "LSTitleGameMode.generated.h"

class ULSTitleMenuWidget;

// 타이틀 레벨 전용 GameMode. WBP_TitleMenu를 뷰포트에 올리고 UI 입력 모드로 전환한다.
UCLASS()
class LOSTSIGNAL_API ALSTitleGameMode : public ALSGameModeBase
{
	GENERATED_BODY()

public:
	ALSTitleGameMode();

	virtual void BeginPlay() override;

protected:
	// BP(BP_TitleGameMode)에서 WBP_TitleMenu를 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Title")
	TSubclassOf<ULSTitleMenuWidget> TitleMenuWidgetClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<ULSTitleMenuWidget> TitleMenuWidgetInstance;

	void CreateTitleMenuWidget();
};
