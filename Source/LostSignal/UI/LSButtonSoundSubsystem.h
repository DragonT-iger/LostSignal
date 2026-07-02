#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LSButtonSoundSubsystem.generated.h"

class ULocalPlayer;
class UUserWidget;
class UWidget;

// 뷰포트에 추가되는 모든 위젯 트리를 훑어 UButton에 공통 클릭/호버 사운드를 채운다.
// 사운드가 이미 지정된 버튼(WBP 개별 지정)은 건드리지 않는다. 에셋 참조는 ULSAudioSettings가 단일 출처다.
UCLASS()
class LOSTSIGNAL_API ULSButtonSoundSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void HandleWidgetAdded(UWidget* Widget, ULocalPlayer* Player);
	void ApplyButtonSoundsRecursive(UUserWidget& RootWidget) const;

	FDelegateHandle WidgetAddedHandle;
};
