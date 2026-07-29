#include "Gameplay/LSLootBox.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSDropSettings.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
#include "Minimap/LSMinimapMarkerComponent.h"
#include "Net/UnrealNetwork.h"
#include "Session/LSSaveSubsystem.h"
#include "TimerManager.h"
#include "UI/Interact/LSDistanceMarkerComponent.h"

ALSLootBox::ALSLootBox()
{
	MinimapMarkerComponent = CreateDefaultSubobject<ULSMinimapMarkerComponent>(TEXT("MinimapMarkerComponent"));
	MinimapMarkerComponent->SetMarkerType(ELSMinimapMarkerType::Loot);
	MinimapMarkerComponent->SetMarkerColor(FLinearColor(1.0f, 0.82f, 0.18f, 1.0f));

	// 룻박스는 거리 기반 빌보드 마커를 기본으로 켠다. 위젯 UI는 전역 설정에서 받아 쓴다.
	if (DistanceMarkerComponent)
	{
		DistanceMarkerComponent->SetMarkerEnabled(true);
	}
}

void ALSLootBox::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALSLootBox, bIsOpened);
	DOREPLIFETIME(ALSLootBox, LootResults);
	DOREPLIFETIME(ALSLootBox, TotalLootCount);
}

bool ALSLootBox::CanInteract_Implementation(APawn* Interactor)
{
	return true;
}

void ALSLootBox::Interact_Implementation(APawn* Interactor)
{
	if (!HasAuthority()) return;

	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	if (!bIsOpened)
	{
		ULSDropSubsystem* DropSubsystem = GI->GetSubsystem<ULSDropSubsystem>();
		if (!DropSubsystem)
		{
			UE_LOG(LogLS, Warning, TEXT("ALSLootBox: DropSubsystem 없음"));
			return;
		}

		if (RootingObjectRowName.IsNone())
		{
			UE_LOG(LogLS, Warning, TEXT("ALSLootBox: RootingObjectRowName 미설정"));
			return;
		}

		// 전체 결과는 서버 전용으로 보관하고, 복제되는 LootResults는 비운 채 시작한다.
		// 타이머가 등급 딜레이마다 한 개씩 LootResults로 옮겨 단계 공개한다.
		PendingLootResults = DropSubsystem->OpenRootingObject(RootingObjectRowName);
		LootResults.Reset();
		// 총 개수는 즉시 복제해 클라가 placeholder 슬롯을 그리게 한다(아이템 정체는 점진 공개).
		TotalLootCount = PendingLootResults.Num();
		NextRevealIndex = 0;
		bIsOpened = true;
		if (MinimapMarkerComponent)
		{
			MinimapMarkerComponent->SetMinimapVisible(false);
		}
		if (DistanceMarkerComponent)
		{
			DistanceMarkerComponent->SetMarkerSuppressed(true);
		}
		RefreshWidgetVisibility();
		OnLootResultReceived(PendingLootResults);
		// 리슨서버 호스트는 OnRep_IsOpened가 호출되지 않으므로 오픈 비주얼을 여기서 직접 띄운다.
		OnLootBoxOpenedVisual();
		NotifyLootResultsChanged();
		ForceNetUpdate();
		ScheduleNextReveal();
	}

	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(Interactor ? Interactor->GetController() : nullptr))
	{
		const FText InteractObjectText = GetInteractText_Implementation();
		PlayerController->ShowLootDropWidget(
			InteractObjectText.IsEmpty() ? FText::FromName(RootingObjectRowName) : InteractObjectText,
			LootResults,
			this);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("ALSLootBox: Cannot show loot drop widget because interactor controller is invalid on %s."), *GetNameSafe(this));
	}
}

void ALSLootBox::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RevealTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ALSLootBox::OnRep_IsOpened()
{
	if (MinimapMarkerComponent)
	{
		MinimapMarkerComponent->SetMinimapVisible(!bIsOpened);
	}
	if (DistanceMarkerComponent)
	{
		DistanceMarkerComponent->SetMarkerSuppressed(bIsOpened);
	}
	RefreshWidgetVisibility();
	if (bIsOpened)
	{
		OnLootBoxOpenedVisual();
	}
}

void ALSLootBox::ScheduleNextReveal()
{
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!PendingLootResults.IsValidIndex(NextRevealIndex))
	{
		// 모두 공개됨: 타이머 정리.
		World->GetTimerManager().ClearTimer(RevealTimerHandle);
		return;
	}

	// 딜레이를 0으로 두면 타이머가 발동하지 않아 공개 체인이 멈출 수 있으므로 최소값으로 보정한다.
	const float Delay = FMath::Max(GetItemRevealDelay(PendingLootResults[NextRevealIndex]), 0.01f);
	World->GetTimerManager().SetTimer(RevealTimerHandle, this, &ALSLootBox::RevealNextLootItem, Delay, false);
}

void ALSLootBox::RevealNextLootItem()
{
	if (!HasAuthority())
	{
		return;
	}

	if (PendingLootResults.IsValidIndex(NextRevealIndex))
	{
		LootResults.Add(PendingLootResults[NextRevealIndex]);
		++NextRevealIndex;
		NotifyLootResultsChanged();
		ForceNetUpdate();
	}

	ScheduleNextReveal();
}

float ALSLootBox::GetItemRevealDelay(const FLSDropResult& Item) const
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	const float DefaultDelay = Settings ? Settings->DefaultRevealDelaySeconds : 0.5f;

	const FString Grade = LSInventorySlotUtils::ResolveItemGradeFromRowName(Item.ItemRowName);
	if (Settings && !Grade.IsEmpty())
	{
		if (const float* Found = Settings->GradeRevealDelaySeconds.Find(Grade))
		{
			if (*Found > 0.0f)
			{
				return *Found;
			}
		}
	}

	return FMath::Max(DefaultDelay, 0.0f);
}

void ALSLootBox::OnRep_LootResults()
{
	NotifyLootResultsChanged();
}

bool ALSLootBox::DropLootSlot(const int32 FromLootSlotIndex, const int32 ToLootSlotIndex)
{
	if (!LootResults.IsValidIndex(FromLootSlotIndex) || !LSInventorySlotUtils::IsFilled(LootResults[FromLootSlotIndex]) || !LootResults.IsValidIndex(ToLootSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop loot slot because slot data is invalid. From=%d To=%d"), FromLootSlotIndex, ToLootSlotIndex);
		return false;
	}

	const bool bChanged = LSInventorySlotUtils::DropResultSlot(LootResults, FromLootSlotIndex, ToLootSlotIndex);
	if (bChanged)
	{
		NotifyLootResultsChanged();
		ForceNetUpdate();
	}
	return bChanged;
}

bool ALSLootBox::TransferLootSlotToSession(const int32 LootSlotIndex, ULSRaidInventoryComponent* RaidInventory, FLSSessionItem& OutRemainingLootItem)
{
	OutRemainingLootItem = FLSSessionItem();
	if (!LootResults.IsValidIndex(LootSlotIndex) || LootResults[LootSlotIndex].ItemRowName.IsNone() || LootResults[LootSlotIndex].Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot because slot is invalid. Index=%d"), LootSlotIndex);
		return false;
	}

	if (!RaidInventory || !RaidInventory->IsRaidActive())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot because raid session is not active."));
		return false;
	}

	if (!RaidInventory->TryAddSessionItem(LootResults[LootSlotIndex].ItemRowName, LootResults[LootSlotIndex].Amount, LootResults[LootSlotIndex].ChipStats, OutRemainingLootItem))
	{
		return false;
	}

	if (OutRemainingLootItem.ItemRowName.IsNone() || OutRemainingLootItem.Amount <= 0)
	{
		ClearLootSlot(LootSlotIndex);
	}
	else
	{
		LSInventorySlotUtils::SetDropResultFromSessionItem(LootResults[LootSlotIndex], OutRemainingLootItem);
	}
	NotifyLootResultsChanged();
	ForceNetUpdate();
	return true;
}

bool ALSLootBox::TransferLootSlotToSessionSlot(const int32 LootSlotIndex, ULSRaidInventoryComponent* RaidInventory, const ELSInventorySlotArea ToSlotArea, const int32 ToSlotIndex, FLSSessionItem& OutRemainingLootItem)
{
	OutRemainingLootItem = FLSSessionItem();
	if (!LootResults.IsValidIndex(LootSlotIndex) || LootResults[LootSlotIndex].ItemRowName.IsNone() || LootResults[LootSlotIndex].Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to inventory slot because slot is invalid. Index=%d"), LootSlotIndex);
		return false;
	}

	if (!RaidInventory || !RaidInventory->IsRaidActive())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to inventory slot because raid session is not active."));
		return false;
	}

	FLSSessionItem ExternalItem = LSInventorySlotUtils::ToSessionItem(LootResults[LootSlotIndex]);
	if (!RaidInventory->DropExternalItemToSessionSlot(ExternalItem, ToSlotArea, ToSlotIndex))
	{
		return false;
	}

	OutRemainingLootItem = ExternalItem;
	if (ExternalItem.ItemRowName.IsNone() || ExternalItem.Amount <= 0)
	{
		ClearLootSlot(LootSlotIndex);
	}
	else
	{
		LSInventorySlotUtils::SetDropResultFromSessionItem(LootResults[LootSlotIndex], ExternalItem);
	}

	NotifyLootResultsChanged();
	ForceNetUpdate();
	return true;
}

bool ALSLootBox::TransferSessionSlotToLootSlot(const int32 LootSlotIndex, ULSRaidInventoryComponent* RaidInventory, const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex, FLSSessionItem& OutLootItem)
{
	OutLootItem = FLSSessionItem();
	if (!LootResults.IsValidIndex(LootSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer session slot to loot slot because loot index is invalid. Index=%d"), LootSlotIndex);
		return false;
	}

	if (!RaidInventory || !RaidInventory->IsRaidActive())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer session slot to loot slot because raid session is not active."));
		return false;
	}

	FLSSessionItem CurrentLootItem = LSInventorySlotUtils::ToSessionItem(LootResults[LootSlotIndex]);

	FLSSessionItem SourceItem;
	if (!RaidInventory->GetSessionSlotItem(FromSlotArea, FromSlotIndex, SourceItem))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer session slot to loot slot because source slot is empty. Area=%d Index=%d"),
			static_cast<int32>(FromSlotArea), FromSlotIndex);
		return false;
	}

	if (!RaidInventory->ReplaceSessionSlotItem(FromSlotArea, FromSlotIndex, CurrentLootItem, OutLootItem))
	{
		return false;
	}

	LSInventorySlotUtils::SetDropResultFromSessionItem(LootResults[LootSlotIndex], OutLootItem);
	NotifyLootResultsChanged();
	ForceNetUpdate();
	return true;
}

bool ALSLootBox::TransferLootSlotToSave(const int32 LootSlotIndex, ULSSaveSubsystem* SaveSubsystem, FLSSessionItem& OutRemainingLootItem)
{
	OutRemainingLootItem = FLSSessionItem();
	if (!LootResults.IsValidIndex(LootSlotIndex) || LootResults[LootSlotIndex].ItemRowName.IsNone() || LootResults[LootSlotIndex].Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to save because slot is invalid. Index=%d"), LootSlotIndex);
		return false;
	}

	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to save because SaveSubsystem is missing."));
		return false;
	}

	if (!SaveSubsystem->TryAddToInventory(LootResults[LootSlotIndex].ItemRowName, LootResults[LootSlotIndex].Amount, LootResults[LootSlotIndex].ChipStats, OutRemainingLootItem))
	{
		return false;
	}

	if (OutRemainingLootItem.ItemRowName.IsNone() || OutRemainingLootItem.Amount <= 0)
	{
		ClearLootSlot(LootSlotIndex);
	}
	else
	{
		LSInventorySlotUtils::SetDropResultFromSessionItem(LootResults[LootSlotIndex], OutRemainingLootItem);
	}
	NotifyLootResultsChanged();
	ForceNetUpdate();
	return true;
}

bool ALSLootBox::TransferLootSlotToSaveSlot(const int32 LootSlotIndex, ULSSaveSubsystem* SaveSubsystem, const ELSInventorySlotArea ToSlotArea, const int32 ToSlotIndex, FLSSessionItem& OutRemainingLootItem)
{
	OutRemainingLootItem = FLSSessionItem();
	if (!LootResults.IsValidIndex(LootSlotIndex) || LootResults[LootSlotIndex].ItemRowName.IsNone() || LootResults[LootSlotIndex].Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to save slot because slot is invalid. Index=%d"), LootSlotIndex);
		return false;
	}

	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to save slot because SaveSubsystem is missing."));
		return false;
	}

	FLSSessionItem ExternalItem = LSInventorySlotUtils::ToSessionItem(LootResults[LootSlotIndex]);
	if (!SaveSubsystem->DropExternalItemToStoredSlot(ExternalItem, ToSlotArea, ToSlotIndex))
	{
		return false;
	}

	OutRemainingLootItem = ExternalItem;
	if (ExternalItem.ItemRowName.IsNone() || ExternalItem.Amount <= 0)
	{
		ClearLootSlot(LootSlotIndex);
	}
	else
	{
		LSInventorySlotUtils::SetDropResultFromSessionItem(LootResults[LootSlotIndex], ExternalItem);
	}

	NotifyLootResultsChanged();
	ForceNetUpdate();
	return true;
}

bool ALSLootBox::TransferSaveSlotToLootSlot(const int32 LootSlotIndex, ULSSaveSubsystem* SaveSubsystem, const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex, FLSSessionItem& OutLootItem)
{
	OutLootItem = FLSSessionItem();
	if (!LootResults.IsValidIndex(LootSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer save slot to loot slot because loot index is invalid. Index=%d"), LootSlotIndex);
		return false;
	}

	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer save slot to loot slot because SaveSubsystem is missing."));
		return false;
	}

	FLSSessionItem CurrentLootItem = LSInventorySlotUtils::ToSessionItem(LootResults[LootSlotIndex]);

	FLSSessionItem SourceItem;
	if (!SaveSubsystem->GetStoredSlotItem(FromSlotArea, FromSlotIndex, SourceItem))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer save slot to loot slot because source slot is empty. Area=%d Index=%d"),
			static_cast<int32>(FromSlotArea), FromSlotIndex);
		return false;
	}

	if (!SaveSubsystem->ReplaceStoredSlotItem(FromSlotArea, FromSlotIndex, CurrentLootItem, OutLootItem))
	{
		return false;
	}

	LSInventorySlotUtils::SetDropResultFromSessionItem(LootResults[LootSlotIndex], OutLootItem);
	NotifyLootResultsChanged();
	ForceNetUpdate();
	return true;
}

void ALSLootBox::ClearLootSlot(const int32 LootSlotIndex)
{
	if (!LootResults.IsValidIndex(LootSlotIndex))
	{
		return;
	}

	LSInventorySlotUtils::ClearDropResult(LootResults[LootSlotIndex]);
}

void ALSLootBox::NotifyLootResultsChanged()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(It->Get()))
		{
			PlayerController->RefreshLootDropWidgetForSource(this, LootResults);
		}
	}
}
