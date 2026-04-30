#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/DeveloperSettings.h"
#include "LSDropSettings.generated.h"

UCLASS(config=Game, defaultconfig, meta=(DisplayName="LS Drop Settings"))
class LOSTSIGNAL_API ULSDropSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> RootingObjectTable;

	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> DropTable;

	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> GroupTable;

	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> ChipTable;

	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> WeaponTable;

	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> ArmorTable;

	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> ItemTable;

	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> ChipStatTable;
};
