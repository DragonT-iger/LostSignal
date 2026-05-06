#include "Session/LSSaveSubsystem.h"
#include "Session/LSSaveGame.h"
#include "LostSignal.h"
#include "Kismet/GameplayStatics.h"

const FString ULSSaveSubsystem::SlotName = TEXT("LostSignalSave");

void ULSSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Load();
}

void ULSSaveSubsystem::AddToStash(const TArray<FLSSessionItem>& Items)
{
	if (!SaveData || Items.IsEmpty()) return;

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

	UE_LOG(LogLS, Log, TEXT("[Save] 스태시 추가 %d종 → 총 %d종"), Items.Num(), SaveData->Stash.Num());
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
		UE_LOG(LogLS, Log, TEXT("[Save] 로드 완료 - 스태시 %d종"), SaveData->Stash.Num());
	}
	else
	{
		SaveData = Cast<ULSSaveGame>(UGameplayStatics::CreateSaveGameObject(ULSSaveGame::StaticClass()));
		UE_LOG(LogLS, Log, TEXT("[Save] 새 세이브 생성"));
	}
}

void ULSSaveSubsystem::Save()
{
	if (!SaveData) return;

	UGameplayStatics::SaveGameToSlot(SaveData, SlotName, 0);
	UE_LOG(LogLS, Log, TEXT("[Save] 저장 완료"));
}
