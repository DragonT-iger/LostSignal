#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"
#include "LSMonsterPresentationSettings.generated.h"

class UMaterialInterface;
class ULSEnemyHealthBarWidget;

/** 모든 몬스터가 공유하는 전투 텔레그래프·체력바·시야 잔상 에셋 설정. */
UCLASS(config=Game, defaultconfig, meta=(DisplayName="LS Monster Presentation Settings"))
class LOSTSIGNAL_API ULSMonsterPresentationSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category="LS/Monster|Telegraph")
	TSoftObjectPtr<UMaterialInterface> TelegraphCircleMaterial;

	UPROPERTY(config, EditAnywhere, Category="LS/Monster|Telegraph")
	TSoftObjectPtr<UMaterialInterface> TelegraphBoxMaterial;

	UPROPERTY(config, EditAnywhere, Category="LS/Monster|UI")
	TSoftClassPtr<ULSEnemyHealthBarWidget> EnemyHealthBarWidgetClass;

	UPROPERTY(config, EditAnywhere, Category="LS/Monster|Vision")
	TSoftObjectPtr<UMaterialInterface> GhostMaterial;
};
