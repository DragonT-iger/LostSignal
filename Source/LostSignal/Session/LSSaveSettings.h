#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Session/LSSessionSubsystem.h"
#include "LSSaveSettings.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSStarterItemConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Save")
	FName ItemRowName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Save", meta=(ClampMin="1"))
	int32 Amount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Save")
	ELSInventorySlotArea TargetArea = ELSInventorySlotArea::Inventory;
};

UCLASS(config=Game, defaultconfig, meta=(DisplayName="LS Save Settings"))
class LOSTSIGNAL_API ULSSaveSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	ULSSaveSettings()
	{
		bGrantLowestGradeChipsOnNewGame = true;
	}

	// 켜지면 새 게임 시작 시 가장 낮은 등급(Supply) 칩을 하드웨어 장착칸 10·9·8·7번에 기본 장착한다.
	UPROPERTY(config, EditAnywhere, Category="LS/Save")
	bool bGrantLowestGradeChipsOnNewGame = true;

	UPROPERTY(config, EditAnywhere, Category="LS/Save")
	TArray<FLSStarterItemConfig> StarterItems;

	// 새 게임(또는 골드 필드가 없던 기존 세이브 로드) 시 1회 지급하는 기본 골드.
	UPROPERTY(config, EditAnywhere, Category="LS/Save", meta=(ClampMin="0"))
	int32 NewGameGold = 1000;
};
