#include "LostSignalEditorDataToolsModule.h"

#include "Engine/DataTable.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "LostSignalEditorDataTools"

namespace
{
	struct FLSDataTableImportTarget
	{
		const TCHAR* AssetName;
		const TCHAR* CsvName;
	};

	const FLSDataTableImportTarget GDataTableImportTargets[] =
	{
		{ TEXT("DT_Armor"), TEXT("DT_Armor.csv") },
		{ TEXT("DT_CharacterStat"), TEXT("DT_CharacterStat.csv") },
		{ TEXT("DT_ChipRow"), TEXT("DT_Chip.csv") },
		{ TEXT("DT_ChipStat"), TEXT("DT_ChipStat.csv") },
		{ TEXT("DT_DropTable"), TEXT("DT_DropTable.csv") },
		{ TEXT("DT_GroupTable"), TEXT("DT_GroupTable.csv") },
		{ TEXT("DT_Item"), TEXT("DT_Item.csv") },
		{ TEXT("DT_RootingObject"), TEXT("DT_RootingObject.csv") },
		{ TEXT("DT_Weapon"), TEXT("DT_Weapon.csv") },
	};
}

void FLostSignalEditorDataToolsModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FLostSignalEditorDataToolsModule::RegisterMenus));
}

void FLostSignalEditorDataToolsModule::ShutdownModule()
{
	if (UToolMenus* ToolMenus = UToolMenus::TryGet())
	{
		UToolMenus::UnRegisterStartupCallback(this);
		ToolMenus->UnregisterOwner(this);
	}
}

void FLostSignalEditorDataToolsModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& ToolsSection = ToolsMenu->FindOrAddSection("LostSignal");
	ToolsSection.Label = LOCTEXT("LostSignalSection", "LostSignal");

	ToolsSection.AddMenuEntry(
		"LSReimportDataTables",
		LOCTEXT("ReimportDataTablesLabel", "Reimport DataTables"),
		LOCTEXT("ReimportDataTablesTooltip", "Reimport LostSignal CSV files into DataTable assets and save them."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLostSignalEditorDataToolsModule::ReimportDataTables))
	);

}

void FLostSignalEditorDataToolsModule::ReimportDataTables()
{
	const FString CsvDirectory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Content/LostSignal/Sandbox/DT"));
	const FString DataTableAssetDirectory = TEXT("/Game/LostSignal/Data/DataTables");

	int32 ImportedCount = 0;
	TArray<FString> Failures;
	TArray<UPackage*> PackagesToSave;

	for (const FLSDataTableImportTarget& Target : GDataTableImportTargets)
	{
		const FString CsvPath = FPaths::Combine(CsvDirectory, Target.CsvName);
		if (!IFileManager::Get().FileExists(*CsvPath))
		{
			Failures.Add(FString::Printf(TEXT("%s: CSV not found (%s)"), Target.AssetName, *CsvPath));
			continue;
		}

		const FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), *DataTableAssetDirectory, Target.AssetName, Target.AssetName);
		UDataTable* DataTable = LoadObject<UDataTable>(nullptr, *AssetPath);
		if (DataTable == nullptr)
		{
			Failures.Add(FString::Printf(TEXT("%s: DataTable asset not found (%s)"), Target.AssetName, *AssetPath));
			continue;
		}

		FString CsvContent;
		if (!FFileHelper::LoadFileToString(CsvContent, *CsvPath))
		{
			Failures.Add(FString::Printf(TEXT("%s: failed to read CSV (%s)"), Target.AssetName, *CsvPath));
			continue;
		}

		const TArray<FString> ImportProblems = DataTable->CreateTableFromCSVString(CsvContent);
		if (ImportProblems.Num() > 0)
		{
			Failures.Add(FString::Printf(TEXT("%s: %s"), Target.AssetName, *FString::Join(ImportProblems, TEXT("; "))));
			continue;
		}

		DataTable->MarkPackageDirty();
		if (UPackage* Package = DataTable->GetOutermost())
		{
			PackagesToSave.AddUnique(Package);
		}

		++ImportedCount;
		UE_LOG(LogTemp, Log, TEXT("Reimported DataTable %s from %s"), Target.AssetName, *CsvPath);
	}

	if (PackagesToSave.Num() > 0)
	{
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, false);
	}

	const FString Message = FString::Printf(
		TEXT("Imported %d DataTables.\nFailed %d."),
		ImportedCount,
		Failures.Num());

	if (Failures.Num() > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("DataTable reimport failed:\n%s"), *FString::Join(Failures, TEXT("\n")));
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message + TEXT("\n\nCheck Output Log for details.")));
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
	}
}

IMPLEMENT_MODULE(FLostSignalEditorDataToolsModule, LostSignalEditorDataTools)

#undef LOCTEXT_NAMESPACE
