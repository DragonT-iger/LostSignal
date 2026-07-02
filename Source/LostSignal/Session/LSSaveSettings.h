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
		LowestGradeChipsStarterTargetArea = ELSInventorySlotArea::Warehouse;
	}

	UPROPERTY(config, EditAnywhere, Category="LS/Save")
	bool bGrantLowestGradeChipsOnNewGame = true;

	UPROPERTY(config, EditAnywhere, Category="LS/Save", meta=(EditCondition="bGrantLowestGradeChipsOnNewGame"))
	ELSInventorySlotArea LowestGradeChipsStarterTargetArea = ELSInventorySlotArea::Warehouse;

	UPROPERTY(config, EditAnywhere, Category="LS/Save")
	TArray<FLSStarterItemConfig> StarterItems;
};
