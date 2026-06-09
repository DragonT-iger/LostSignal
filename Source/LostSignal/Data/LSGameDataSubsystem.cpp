#include "Data/LSGameDataSubsystem.h"

#include "Data/LSCharacterPassiveSkillRow.h"
#include "Data/LSCharacterSkillRow.h"
#include "Data/LSComboAttackRow.h"
#include "Data/LSGameDataSettings.h"
#include "Data/LSProtocolUnlockRow.h"
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

const FLSCharacterPassiveSkillRow* ULSGameDataSubsystem::FindPassiveSkillRowByID(const int32 PassiveSkillID, const TCHAR* Context) const
{
	if (!CharacterPassiveSkillTable || PassiveSkillID <= 0)
	{
		return nullptr;
	}

	if (const FLSCharacterPassiveSkillRow* Row = FindPassiveSkillRow(FName(*FString::FromInt(PassiveSkillID)), Context))
	{
		return Row;
	}

	for (const TPair<FName, uint8*>& Pair : CharacterPassiveSkillTable->GetRowMap())
	{
		const FLSCharacterPassiveSkillRow* Row = reinterpret_cast<const FLSCharacterPassiveSkillRow*>(Pair.Value);
		if (Row && Row->PassiveSkill_ID == PassiveSkillID)
		{
			return Row;
		}
	}

	return nullptr;
}

const FLSComboAttackRow* ULSGameDataSubsystem::FindComboAttackRow(const FName RowName, const TCHAR* Context) const
{
	if (!ComboAttackTable || RowName.IsNone())
	{
		return nullptr;
	}

	return ComboAttackTable->FindRow<FLSComboAttackRow>(RowName, Context);
}

const FLSComboAttackRow* ULSGameDataSubsystem::FindComboAttackRowByID(const int32 ComboID, const TCHAR* Context) const
{
	if (!ComboAttackTable || ComboID <= 0)
	{
		return nullptr;
	}

	if (const FLSComboAttackRow* Row = FindComboAttackRow(FName(*FString::FromInt(ComboID)), Context))
	{
		return Row;
	}

	for (const TPair<FName, uint8*>& Pair : ComboAttackTable->GetRowMap())
	{
		const FLSComboAttackRow* Row = reinterpret_cast<const FLSComboAttackRow*>(Pair.Value);
		if (Row && Row->Combo_ID == ComboID)
		{
			return Row;
		}
	}

	return nullptr;
}

const FLSComboAttackRow* ULSGameDataSubsystem::FindComboAttackRowByIndex(const int32 CharacterID, const int32 ComboIndex, const int32 ComboTag, const TCHAR* Context) const
{
	(void)Context;

	if (!ComboAttackTable || ComboIndex <= 0)
	{
		return nullptr;
	}

	const FLSComboAttackRow* BestDefaultRow = nullptr;
	const FLSComboAttackRow* BestTaggedRow = nullptr;
	for (const TPair<FName, uint8*>& Pair : ComboAttackTable->GetRowMap())
	{
		const FLSComboAttackRow* Row = reinterpret_cast<const FLSComboAttackRow*>(Pair.Value);
		if (!Row || Row->Combo_Index != ComboIndex || (CharacterID > 0 && Row->Combo_Char != CharacterID))
		{
			continue;
		}

		if (ComboTag > 0 && Row->Combo_Tag == ComboTag)
		{
			if (!BestTaggedRow || Row->Priority > BestTaggedRow->Priority)
			{
				BestTaggedRow = Row;
			}
			continue;
		}

		if (Row->Combo_Tag == 0 && (!BestDefaultRow || Row->Priority > BestDefaultRow->Priority))
		{
			BestDefaultRow = Row;
		}
	}

	return BestTaggedRow ? BestTaggedRow : BestDefaultRow;
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
	if (!StatusEffectTable || StatusID <= 0)
	{
		return nullptr;
	}

	return FindStatusEffectRow(FName(*FString::FromInt(StatusID)), Context);
}

const FLSProtocolUnlockRow* ULSGameDataSubsystem::FindProtocolUnlockRow(const FName RowName, const TCHAR* Context) const
{
	if (!ProtocolUnlockTable || RowName.IsNone())
	{
		return nullptr;
	}

	return ProtocolUnlockTable->FindRow<FLSProtocolUnlockRow>(RowName, Context);
}

const FLSProtocolUnlockRow* ULSGameDataSubsystem::FindProtocolUnlockRowByEnableName(const ELSProtocolType ProtocolType, const FName EnableName, const TCHAR* Context) const
{
	if (!ProtocolUnlockTable || EnableName.IsNone())
	{
		return nullptr;
	}

	const FName NormalizedEnableName = LSProtocol::NormalizeProtocolEnableName(EnableName);
	TArray<const FLSProtocolUnlockRow*> Rows;
	GetProtocolUnlockRows(ProtocolType, Rows, Context);
	for (const FLSProtocolUnlockRow* Row : Rows)
	{
		if (Row && LSProtocol::NormalizeProtocolEnableName(Row->Protocol_Enable_Name) == NormalizedEnableName)
		{
			return Row;
		}
	}

	return nullptr;
}

void ULSGameDataSubsystem::GetProtocolUnlockRows(const ELSProtocolType ProtocolType, TArray<const FLSProtocolUnlockRow*>& OutRows, const TCHAR* Context) const
{
	(void)Context;

	OutRows.Reset();
	if (!ProtocolUnlockTable)
	{
		return;
	}

	for (const TPair<FName, uint8*>& Pair : ProtocolUnlockTable->GetRowMap())
	{
		ELSProtocolType RowProtocolType = ELSProtocolType::Survival;
		if (!LSProtocol::ResolveProtocolTypeFromRowName(Pair.Key, RowProtocolType) || RowProtocolType != ProtocolType)
		{
			continue;
		}

		const FLSProtocolUnlockRow* Row = reinterpret_cast<const FLSProtocolUnlockRow*>(Pair.Value);
		if (Row)
		{
			OutRows.Add(Row);
		}
	}

	OutRows.Sort([](const FLSProtocolUnlockRow& Left, const FLSProtocolUnlockRow& Right)
	{
		if (Left.Protocol_Required_Level != Right.Protocol_Required_Level)
		{
			return Left.Protocol_Required_Level < Right.Protocol_Required_Level;
		}

		return Left.Protocol_Enable_Name.LexicalLess(Right.Protocol_Enable_Name);
	});
}

void ULSGameDataSubsystem::GetProtocolRequiredLevels(const ELSProtocolType ProtocolType, TArray<int32>& OutRequiredLevels, const TCHAR* Context) const
{
	OutRequiredLevels.Reset();

	TArray<const FLSProtocolUnlockRow*> Rows;
	GetProtocolUnlockRows(ProtocolType, Rows, Context);
	for (const FLSProtocolUnlockRow* Row : Rows)
	{
		if (Row && Row->Protocol_Required_Level > 0)
		{
			OutRequiredLevels.AddUnique(Row->Protocol_Required_Level);
		}
	}

	OutRequiredLevels.Sort();
}

int32 ULSGameDataSubsystem::CountProtocolUnlockRows(const ELSProtocolType ProtocolType, const TCHAR* Context) const
{
	TArray<const FLSProtocolUnlockRow*> Rows;
	GetProtocolUnlockRows(ProtocolType, Rows, Context);
	return Rows.Num();
}

int32 ULSGameDataSubsystem::GetMaxProtocolRequiredLevel(const ELSProtocolType ProtocolType, const TCHAR* Context) const
{
	TArray<const FLSProtocolUnlockRow*> Rows;
	GetProtocolUnlockRows(ProtocolType, Rows, Context);

	int32 MaxRequiredLevel = 0;
	for (const FLSProtocolUnlockRow* Row : Rows)
	{
		if (Row)
		{
			MaxRequiredLevel = FMath::Max(MaxRequiredLevel, Row->Protocol_Required_Level);
		}
	}

	return MaxRequiredLevel;
}

int32 ULSGameDataSubsystem::CountVisibleProtocolUnlockRows(const ELSProtocolType ProtocolType, const int32 CurrentLevel, const int32 PreviousLevel, const TCHAR* Context) const
{
	TArray<const FLSProtocolUnlockRow*> Rows;
	GetProtocolUnlockRows(ProtocolType, Rows, Context);

	int32 VisibleCount = 0;
	for (const FLSProtocolUnlockRow* Row : Rows)
	{
		if (Row && IsProtocolUnlockVisible(*Row, CurrentLevel, PreviousLevel))
		{
			++VisibleCount;
		}
	}

	return VisibleCount;
}

int32 ULSGameDataSubsystem::GetVisibleProtocolEnableValueSum(const ELSProtocolType ProtocolType, const FName EnableName, const int32 CurrentLevel, const int32 PreviousLevel, const TCHAR* Context) const
{
	if (EnableName.IsNone())
	{
		return 0;
	}

	const FName NormalizedEnableName = LSProtocol::NormalizeProtocolEnableName(EnableName);
	TArray<const FLSProtocolUnlockRow*> Rows;
	GetProtocolUnlockRows(ProtocolType, Rows, Context);

	int32 ValueSum = 0;
	for (const FLSProtocolUnlockRow* Row : Rows)
	{
		if (!Row || LSProtocol::NormalizeProtocolEnableName(Row->Protocol_Enable_Name) != NormalizedEnableName)
		{
			continue;
		}

		if (IsProtocolUnlockVisible(*Row, CurrentLevel, PreviousLevel))
		{
			ValueSum += Row->Protocol_Enable_Value;
		}
	}

	return ValueSum;
}

bool ULSGameDataSubsystem::IsProtocolUnlockVisible(const FLSProtocolUnlockRow& Row, const int32 CurrentLevel, const int32 PreviousLevel, bool* bOutProtected) const
{
	if (bOutProtected)
	{
		*bOutProtected = false;
	}

	if (CurrentLevel >= Row.Protocol_Required_Level)
	{
		return true;
	}

	const bool bProtected =
		PreviousLevel >= Row.Protocol_Required_Level &&
		Row.Protocol_Protected_Level > 0 &&
		CurrentLevel >= Row.Protocol_Protected_Level;
	if (bOutProtected)
	{
		*bOutProtected = bProtected;
	}

	return bProtected;
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
	ComboAttackTable = Settings->ComboAttackTable.LoadSynchronous();
	StatusEffectTable = Settings->StatusEffectTable.LoadSynchronous();
	ProtocolUnlockTable = Settings->ProtocolUnlockTable.LoadSynchronous();

	NormalizeActiveSkillRows();
	NormalizePassiveSkillRows();
	NormalizeComboAttackRows();

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

	if (!ProtocolUnlockTable)
	{
		UE_LOG(LogLS, Warning, TEXT("[GameData] ProtocolUnlockTable 미설정 - 프로젝트 설정 > LS Game Data Settings 확인"));
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

void ULSGameDataSubsystem::NormalizePassiveSkillRows() const
{
	if (!CharacterPassiveSkillTable)
	{
		return;
	}

	for (const TPair<FName, uint8*>& Pair : CharacterPassiveSkillTable->GetRowMap())
	{
		FLSCharacterPassiveSkillRow* Row = reinterpret_cast<FLSCharacterPassiveSkillRow*>(Pair.Value);
		if (Row)
		{
			Row->NormalizePassiveSkillIDFromRowName(Pair.Key);
		}
	}
}

void ULSGameDataSubsystem::NormalizeComboAttackRows() const
{
	if (!ComboAttackTable)
	{
		return;
	}

	for (const TPair<FName, uint8*>& Pair : ComboAttackTable->GetRowMap())
	{
		FLSComboAttackRow* Row = reinterpret_cast<FLSComboAttackRow*>(Pair.Value);
		if (Row)
		{
			Row->NormalizeComboIDFromRowName(Pair.Key);
		}
	}
}
