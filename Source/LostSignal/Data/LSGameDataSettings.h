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

	// 캐릭터 강화 노드. 기획 시트가 종류별로 나뉘어 있어 테이블도 5개다.
	// 런타임은 이 5개를 통합 인덱스 하나로 합쳐서만 쓴다(LSSkillNodeIndex).
	// 시트 전체를 내보낸 CSV 를 그대로 쓰므로 각 에셋에서 Ignore Extra Fields 를 켜야 한다.
	UPROPERTY(config, EditAnywhere, Category="LS/DataTables/SkillNode")
	TSoftObjectPtr<UDataTable> SkillNodeCoreTable;

	UPROPERTY(config, EditAnywhere, Category="LS/DataTables/SkillNode")
	TSoftObjectPtr<UDataTable> SkillNodeMainStatTable;

	UPROPERTY(config, EditAnywhere, Category="LS/DataTables/SkillNode")
	TSoftObjectPtr<UDataTable> SkillNodeSubStatTable;

	UPROPERTY(config, EditAnywhere, Category="LS/DataTables/SkillNode")
	TSoftObjectPtr<UDataTable> SkillNodeEnhanceTable;

	UPROPERTY(config, EditAnywhere, Category="LS/DataTables/SkillNode")
	TSoftObjectPtr<UDataTable> SkillNodeEvolveTable;
};
