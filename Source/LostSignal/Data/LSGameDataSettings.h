#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/DeveloperSettings.h"
#include "LSGameDataSettings.generated.h"

UCLASS(config=Game, defaultconfig, meta=(DisplayName="LS Game Data Settings"))
class LOSTSIGNAL_API ULSGameDataSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category="LS/DataTables")
	TSoftObjectPtr<UDataTable> CharacterActiveSkillTable;

	UPROPERTY(config, EditAnywhere, Category="LS/DataTables")
	TSoftObjectPtr<UDataTable> CharacterPassiveSkillTable;

	UPROPERTY(config, EditAnywhere, Category="LS/DataTables")
	TSoftObjectPtr<UDataTable> ComboAttackTable;

	UPROPERTY(config, EditAnywhere, Category="LS/DataTables")
	TSoftObjectPtr<UDataTable> StatusEffectTable;

	UPROPERTY(config, EditAnywhere, Category="LS/DataTables")
	TSoftObjectPtr<UDataTable> ConsumableTable;

	UPROPERTY(config, EditAnywhere, Category="LS/DataTables")
	TSoftObjectPtr<UDataTable> ConsumableEffectTable;

	UPROPERTY(config, EditAnywhere, Category="LS/DataTables")
	TSoftObjectPtr<UDataTable> ProtocolUnlockTable;

	UPROPERTY(config, EditAnywhere, Category="LS/DataTables")
	TSoftObjectPtr<UDataTable> MonsterArchetypeTable;

	UPROPERTY(config, EditAnywhere, Category="LS/DataTables")
	TSoftObjectPtr<UDataTable> MonsterActionTable;

	UPROPERTY(config, EditAnywhere, Category="LS/DataTables")
	TSoftObjectPtr<UDataTable> NoiseProfileTable;
};
