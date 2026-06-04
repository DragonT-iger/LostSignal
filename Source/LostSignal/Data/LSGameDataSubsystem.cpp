#include "Data/LSGameDataSubsystem.h"

#include "Data/LSCharacterPassiveSkillRow.h"
#include "Data/LSCharacterSkillRow.h"
#include "Data/LSGameDataSettings.h"
#include "Data/LSStatusEffectRow.h"
#include "Engine/DataTable.h"
#include "LostSignal.h"

void ULSGameDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadTables();
}

void ULSGameDataSubsystem::ReloadTables()
{
	LoadTables();
}

const FLSCharacterSkillRow* ULSGameDataSubsystem::FindActiveSkillRow(const FName RowName, const TCHAR* Context) const
{
	if (!CharacterActiveSkillTable || RowName.IsNone())
	{
		return nullptr;
	}

	return CharacterActiveSkillTable->FindRow<FLSCharacterSkillRow>(RowName, Context);
}

const FLSCharacterSkillRow* ULSGameDataSubsystem::FindActiveSkillRowByID(const int32 SkillID, const TCHAR* Context) const
{
	if (!CharacterActiveSkillTable || SkillID <= 0)
	{
		return nullptr;
	}

	if (const FLSCharacterSkillRow* Row = FindActiveSkillRow(FName(*FString::FromInt(SkillID)), Context))
	{
		return Row;
	}

	for (const TPair<FName, uint8*>& Pair : CharacterActiveSkillTable->GetRowMap())
	{
		const FLSCharacterSkillRow* Row = reinterpret_cast<const FLSCharacterSkillRow*>(Pair.Value);
		if (Row && Row->Skill_ID == SkillID)
		{
			return Row;
		}
	}

	return nullptr;
}

const FLSCharacterPassiveSkillRow* ULSGameDataSubsystem::FindPassiveSkillRow(const FName RowName, const TCHAR* Context) const
{
	if (!CharacterPassiveSkillTable || RowName.IsNone())
	{
		return nullptr;
	}

	return CharacterPassiveSkillTable->FindRow<FLSCharacterPassiveSkillRow>(RowName, Context);
}

const FLSStatusEffectRow* ULSGameDataSubsystem::FindStatusEffectRow(const FName RowName, const TCHAR* Context) const
{
	if (!StatusEffectTable || RowName.IsNone())
	{
		return nullptr;
	}

	return StatusEffectTable->FindRow<FLSStatusEffectRow>(RowName, Context);
}

const FLSStatusEffectRow* ULSGameDataSubsystem::FindStatusEffectRowByID(const int32 StatusID, const TCHAR* Context) const
{
	return FindStatusEffectRow(FName(*FString::FromInt(StatusID)), Context);
}

void ULSGameDataSubsystem::LoadTables()
{
	const ULSGameDataSettings* Settings = GetDefault<ULSGameDataSettings>();
	if (!Settings)
	{
		return;
	}

	CharacterActiveSkillTable = Settings->CharacterActiveSkillTable.LoadSynchronous();
	CharacterPassiveSkillTable = Settings->CharacterPassiveSkillTable.LoadSynchronous();
	StatusEffectTable = Settings->StatusEffectTable.LoadSynchronous();

	NormalizeActiveSkillRows();

	if (!CharacterActiveSkillTable)
	{
		UE_LOG(LogLS, Warning, TEXT("[GameData] CharacterActiveSkillTable 미설정 - 프로젝트 설정 > LS Game Data Settings 확인"));
	}

	if (!CharacterPassiveSkillTable)
	{
		UE_LOG(LogLS, Warning, TEXT("[GameData] CharacterPassiveSkillTable 미설정 - 프로젝트 설정 > LS Game Data Settings 확인"));
	}

	if (!StatusEffectTable)
	{
		UE_LOG(LogLS, Warning, TEXT("[GameData] StatusEffectTable 미설정 - 프로젝트 설정 > LS Game Data Settings 확인"));
	}
}

void ULSGameDataSubsystem::NormalizeActiveSkillRows() const
{
	if (!CharacterActiveSkillTable)
	{
		return;
	}

	for (const TPair<FName, uint8*>& Pair : CharacterActiveSkillTable->GetRowMap())
	{
		FLSCharacterSkillRow* Row = reinterpret_cast<FLSCharacterSkillRow*>(Pair.Value);
		if (Row)
		{
			Row->NormalizeSkillIDFromRowName(Pair.Key);
		}
	}
}
