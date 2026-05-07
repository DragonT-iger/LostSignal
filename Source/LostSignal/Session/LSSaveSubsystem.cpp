#include "Session/LSSaveSubsystem.h"
#include "Session/LSSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Policies\PrettyJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

const FString ULSSaveSubsystem::SlotName = TEXT("LostSignalSave");
const FString ULSSaveSubsystem::DebugFileName = TEXT("LostSignalSave_Debug.json");

void ULSSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Load();
}

void ULSSaveSubsystem::AddToStash(const TArray<FLSSessionItem>& Items)
{
	if (!SaveData || Items.IsEmpty())
	{
		return;
	}

	for (const FLSSessionItem& NewItem : Items)
	{
		bool bMerged = false;
		for (FLSSessionItem& Existing : SaveData->Stash)
		{
			if (Existing.ItemRowName == NewItem.ItemRowName)
			{
				Existing.Amount += NewItem.Amount;
				bMerged = true;
				break;
			}
		}

		if (!bMerged)
		{
			SaveData->Stash.Add(NewItem);
		}
	}

	UE_LOG(LogLS, Log, TEXT("[Save] Stash updated: added %d entries, total %d"),
		Items.Num(), SaveData->Stash.Num());
	Save();
}

const TArray<FLSSessionItem>& ULSSaveSubsystem::GetStash() const
{
	static TArray<FLSSessionItem> Empty;
	return SaveData ? SaveData->Stash : Empty;
}

void ULSSaveSubsystem::Load()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		SaveData = Cast<ULSSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		UE_LOG(LogLS, Log, TEXT("[Save] Loaded stash entries: %d"),
			SaveData ? SaveData->Stash.Num() : 0);
		return;
	}

	SaveData = Cast<ULSSaveGame>(UGameplayStatics::CreateSaveGameObject(ULSSaveGame::StaticClass()));
	UE_LOG(LogLS, Log, TEXT("[Save] Created new save object"));
}

void ULSSaveSubsystem::Save()
{
	if (!SaveData)
	{
		return;
	}

	UGameplayStatics::SaveGameToSlot(SaveData, SlotName, 0);
#if !UE_BUILD_SHIPPING
	SaveDebugJson();
#endif
	UE_LOG(LogLS, Log, TEXT("[Save] Save completed"));
}

void ULSSaveSubsystem::SaveDebugJson() const
{
	if (!SaveData)
	{
		return;
	}

	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(TEXT("slotName"), SlotName);

	TArray<TSharedPtr<FJsonValue>> StashArray;
	StashArray.Reserve(SaveData->Stash.Num());

	for (const FLSSessionItem& Item : SaveData->Stash)
	{
		TSharedRef<FJsonObject> ItemObject = MakeShared<FJsonObject>();
		ItemObject->SetStringField(TEXT("itemRowName"), Item.ItemRowName.ToString());
		ItemObject->SetNumberField(TEXT("amount"), Item.Amount);
		StashArray.Add(MakeShared<FJsonValueObject>(ItemObject));
	}

	RootObject->SetArrayField(TEXT("stash"), StashArray);

	FString OutputString;
	const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutputString);

	if (!FJsonSerializer::Serialize(RootObject, Writer))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Debug JSON serialize failed"));
		return;
	}

	const FString DebugFilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), DebugFileName);
	if (!FFileHelper::SaveStringToFile(OutputString, *DebugFilePath))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Debug JSON write failed: %s"), *DebugFilePath);
		return;
	}

	UE_LOG(LogLS, Log, TEXT("[Save] Debug JSON written: %s"), *DebugFilePath);
}
