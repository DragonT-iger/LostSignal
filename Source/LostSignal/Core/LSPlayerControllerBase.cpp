// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/LSPlayerControllerBase.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Characters/LSPlayerCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Gameplay/LSLootBox.h"
#include "Gameplay/LSWorldDroppedItem.h"
#include "InputMappingContext.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
#include "UI/Debug/LSHpDebugWidget.h"
#include "UI/LootDrop/LSLootDropWidget.h"

ALSPlayerControllerBase::ALSPlayerControllerBase()
{
	RaidInventoryComponent = CreateDefaultSubobject<ULSRaidInventoryComponent>(TEXT("RaidInventoryComponent"));
}

void ALSPlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	InitializeRaidInventoryFromSessionSubsystem();

	if (!IsLocalPlayerController())
	{
		return;
	}

	bShowMouseCursor = true;
	bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	UWidgetBlueprintLibrary::SetFocusToGameViewport();

	if (DebugHpWidgetClass && !DebugHpWidgetInstance)
	{
		DebugHpWidgetInstance = CreateWidget<ULSHpDebugWidget>(this, DebugHpWidgetClass);
		if (DebugHpWidgetInstance)
		{
			DebugHpWidgetInstance->SetObservedCharacter(Cast<ALSCharacterBase>(GetPawn()));
			DebugHpWidgetInstance->AddToViewport();
		}
	}
}

void ALSPlayerControllerBase::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController() || bDefaultMappingContextsApplied)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* IMC : DefaultMappingContexts)
		{
			if (IMC)
			{
				Subsystem->AddMappingContext(IMC, 0);
			}
		}

		bDefaultMappingContextsApplied = true;
	}
}

void ALSPlayerControllerBase::InitializeRaidInventoryFromSessionSubsystem()
{
	if (!RaidInventoryComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot initialize raid inventory because RaidInventoryComponent is missing on %s."), *GetNameSafe(this));
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULSSessionSubsystem* SessionSub = GameInstance ? GameInstance->GetSubsystem<ULSSessionSubsystem>() : nullptr;
	if (!SessionSub || !SessionSub->IsRaidActive())
	{
		return;
	}

	RaidInventoryComponent->MirrorRaidInventoryState(SessionSub->GetSessionInventory(), SessionSub->GetSessionSafeInventory());
	if (HasAuthority())
	{
		SyncRaidInventoryToClient();
	}
}

void ALSPlayerControllerBase::ClientStartRaidSession_Implementation(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems)
{
	if (!RaidInventoryComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot mirror raid inventory because RaidInventoryComponent is missing on %s."), *GetNameSafe(this));
		return;
	}

	RaidInventoryComponent->StartRaidInventory(Loadout, SafeItems);
}

void ALSPlayerControllerBase::ShowLootDropWidget(const FText& LootSourceName, const TArray<FLSDropResult>& Results, ALSLootBox* SourceLootBox)
{
	if (IsLocalPlayerController())
	{
		ShowLootDropWidgetLocal(LootSourceName, Results, SourceLootBox);
		return;
	}

	ClientShowLootDropWidget(LootSourceName, Results, SourceLootBox);
}

void ALSPlayerControllerBase::ClientShowLootDropWidget_Implementation(const FText& LootSourceName, const TArray<FLSDropResult>& Results, ALSLootBox* SourceLootBox)
{
	ShowLootDropWidgetLocal(LootSourceName, Results, SourceLootBox);
}

void ALSPlayerControllerBase::HideLootDropWidget()
{
	if (IsLocalPlayerController())
	{
		HideLootDropWidgetLocal();
		return;
	}

	ClientHideLootDropWidget();
}

void ALSPlayerControllerBase::ClientHideLootDropWidget_Implementation()
{
	HideLootDropWidgetLocal();
}

void ALSPlayerControllerBase::ClientSyncRaidSessionAndLoot_Implementation(ALSLootBox* SourceLootBox, const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSDropResult>& LootResults)
{
	if (!RaidInventoryComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot sync raid inventory because RaidInventoryComponent is missing on %s."), *GetNameSafe(this));
		return;
	}

	RaidInventoryComponent->MirrorRaidInventoryState(InventoryItems, SafeItems);

	if (ALSPlayerCharacter* PlayerCharacter = Cast<ALSPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->RebuildInventoryWidgetSlots();
	}

	RefreshLootDropWidgetForSource(SourceLootBox, LootResults);
}

void ALSPlayerControllerBase::RefreshLootDropWidgetForSource(ALSLootBox* SourceLootBox, const TArray<FLSDropResult>& Results)
{
	if (!IsLocalPlayerController() || !LootDropWidgetInstance || !LootDropWidgetInstance->IsVisible())
	{
		return;
	}

	LootDropWidgetInstance->RefreshLootItemsFromSource(SourceLootBox, Results);
}

void ALSPlayerControllerBase::SyncRaidInventoryToClient()
{
	if (!HasAuthority() || !RaidInventoryComponent)
	{
		return;
	}

	ClientSyncRaidSessionAndLoot(
		nullptr,
		RaidInventoryComponent->GetSessionInventory(),
		RaidInventoryComponent->GetSessionSafeInventory(),
		TArray<FLSDropResult>());
}

void ALSPlayerControllerBase::ShowLootDropWidgetLocal(const FText& LootSourceName, const TArray<FLSDropResult>& Results, ALSLootBox* SourceLootBox)
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (!LootDropWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("LootDropWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	if (!LootDropWidgetInstance)
	{
		LootDropWidgetInstance = CreateWidget<ULSLootDropWidget>(this, LootDropWidgetClass);
		if (!LootDropWidgetInstance)
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create loot drop widget on %s."), *GetNameSafe(this));
			return;
		}
	}

	if (!LootDropWidgetInstance->IsInViewport())
	{
		LootDropWidgetInstance->AddToViewport();
	}

	LootDropWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	LootDropWidgetInstance->SetLootSourceName(LootSourceName);
	LootDropWidgetInstance->SetSourceLootBox(SourceLootBox);
	LootDropWidgetInstance->SetLootItems(Results);
}

void ALSPlayerControllerBase::HideLootDropWidgetLocal()
{
	if (LootDropWidgetInstance)
	{
		LootDropWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
		LootDropWidgetInstance->ClearLootItems();
	}
}

bool ALSPlayerControllerBase::TransferLootDropSlotToSession(ALSLootBox* SourceLootBox, const int32 LootSlotIndex, const FName ItemRowName, const int32 Amount, FLSSessionItem& OutLootItem)
{
	OutLootItem = FLSSessionItem();
	if (!SourceLootBox || LootSlotIndex == INDEX_NONE || ItemRowName.IsNone() || Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot request loot transfer because request data is invalid. Slot=%d Row=%s Amount=%d"),
			LootSlotIndex,
			*ItemRowName.ToString(),
			Amount);
		return false;
	}

	if (HasAuthority())
	{
		const bool bTransferred = TransferLootDropSlotToSessionInternal(SourceLootBox, LootSlotIndex, OutLootItem);
		if (bTransferred)
		{
			SyncRaidSessionAndLootFromServer(SourceLootBox);
		}
		return bTransferred;
	}

	ServerTransferLootDropSlotToSession(SourceLootBox, LootSlotIndex);
	return true;
}

void ALSPlayerControllerBase::ServerTransferLootDropSlotToSession_Implementation(ALSLootBox* SourceLootBox, const int32 LootSlotIndex)
{
	FLSSessionItem IgnoredLootItem;
	const bool bTransferred = TransferLootDropSlotToSessionInternal(SourceLootBox, LootSlotIndex, IgnoredLootItem);
	if (SourceLootBox || bTransferred)
	{
		SyncRaidSessionAndLootFromServer(SourceLootBox);
	}
}

bool ALSPlayerControllerBase::TransferLootDropSlotToSessionSlot(ALSLootBox* SourceLootBox, const int32 LootSlotIndex, const FName ItemRowName, const int32 Amount, const ELSInventorySlotArea ToSlotArea, const int32 ToSlotIndex, FLSSessionItem& OutLootItem)
{
	OutLootItem = FLSSessionItem();
	if (!SourceLootBox || LootSlotIndex == INDEX_NONE || ItemRowName.IsNone() || Amount <= 0 || ToSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot request loot transfer to slot because request data is invalid. LootSlot=%d Row=%s Amount=%d To=%d"),
			LootSlotIndex,
			*ItemRowName.ToString(),
			Amount,
			ToSlotIndex);
		return false;
	}

	if (HasAuthority())
	{
		const bool bTransferred = TransferLootDropSlotToSessionSlotInternal(SourceLootBox, LootSlotIndex, ToSlotArea, ToSlotIndex, OutLootItem);
		if (bTransferred)
		{
			SyncRaidSessionAndLootFromServer(SourceLootBox);
		}
		return bTransferred;
	}

	ServerTransferLootDropSlotToSessionSlot(SourceLootBox, LootSlotIndex, ToSlotArea, ToSlotIndex);
	return true;
}

void ALSPlayerControllerBase::ServerTransferLootDropSlotToSessionSlot_Implementation(ALSLootBox* SourceLootBox, const int32 LootSlotIndex, const ELSInventorySlotArea ToSlotArea, const int32 ToSlotIndex)
{
	FLSSessionItem IgnoredLootItem;
	const bool bTransferred = TransferLootDropSlotToSessionSlotInternal(SourceLootBox, LootSlotIndex, ToSlotArea, ToSlotIndex, IgnoredLootItem);
	if (SourceLootBox || bTransferred)
	{
		SyncRaidSessionAndLootFromServer(SourceLootBox);
	}
}

bool ALSPlayerControllerBase::TransferSessionSlotToLootDropSlot(ALSLootBox* SourceLootBox, const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex, const int32 LootSlotIndex, const FLSDropResult& CurrentLootItem, FLSSessionItem& OutLootItem)
{
	OutLootItem = FLSSessionItem();
	if (!SourceLootBox || FromSlotIndex == INDEX_NONE || LootSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot request session slot transfer to loot because request data is invalid. From=%d Loot=%d"),
			FromSlotIndex,
			LootSlotIndex);
		return false;
	}

	if (HasAuthority())
	{
		const bool bTransferred = TransferSessionSlotToLootDropSlotInternal(SourceLootBox, FromSlotArea, FromSlotIndex, LootSlotIndex, OutLootItem);
		if (bTransferred)
		{
			SyncRaidSessionAndLootFromServer(SourceLootBox);
		}
		return bTransferred;
	}

	ServerTransferSessionSlotToLootDropSlot(SourceLootBox, FromSlotArea, FromSlotIndex, LootSlotIndex);
	return true;
}

void ALSPlayerControllerBase::ServerTransferSessionSlotToLootDropSlot_Implementation(ALSLootBox* SourceLootBox, const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex, const int32 LootSlotIndex)
{
	FLSSessionItem IgnoredLootItem;
	const bool bTransferred = TransferSessionSlotToLootDropSlotInternal(SourceLootBox, FromSlotArea, FromSlotIndex, LootSlotIndex, IgnoredLootItem);
	if (SourceLootBox || bTransferred)
	{
		SyncRaidSessionAndLootFromServer(SourceLootBox);
	}
}

bool ALSPlayerControllerBase::TransferHoveredLootDropItemToInventory()
{
	if (!LootDropWidgetInstance || !LootDropWidgetInstance->IsVisible())
	{
		return false;
	}

	return LootDropWidgetInstance->TransferHoveredLootSlotToInventory();
}

bool ALSPlayerControllerBase::TransferInventorySlotToLootDrop(const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex)
{
	if (!LootDropWidgetInstance || !LootDropWidgetInstance->IsVisible())
	{
		return false;
	}

	return LootDropWidgetInstance->TransferInventorySlotToFirstEmptyLootSlot(FromSlotArea, FromSlotIndex);
}

bool ALSPlayerControllerBase::DropInventorySlot(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	if (FromIndex == INDEX_NONE || ToIndex == INDEX_NONE)
	{
		return false;
	}

	if (HasAuthority())
	{
		ULSRaidInventoryComponent* InventoryComponent = GetRaidInventoryComponent();
		const bool bChanged = InventoryComponent && InventoryComponent->IsRaidActive() && InventoryComponent->DropSessionSlot(FromArea, FromIndex, ToArea, ToIndex);
		if (bChanged)
		{
			SyncRaidInventoryToClient();
		}
		return bChanged;
	}

	ServerDropInventorySlot(FromArea, FromIndex, ToArea, ToIndex);
	return true;
}

void ALSPlayerControllerBase::ServerDropInventorySlot_Implementation(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	ULSRaidInventoryComponent* InventoryComponent = GetRaidInventoryComponent();
	const bool bChanged = InventoryComponent && InventoryComponent->IsRaidActive() && InventoryComponent->DropSessionSlot(FromArea, FromIndex, ToArea, ToIndex);
	if (bChanged)
	{
		SyncRaidInventoryToClient();
	}
}

bool ALSPlayerControllerBase::SortRaidInventory()
{
	if (HasAuthority())
	{
		ULSRaidInventoryComponent* InventoryComponent = GetRaidInventoryComponent();
		if (!InventoryComponent || !InventoryComponent->IsRaidActive())
		{
			return false;
		}

		InventoryComponent->SortSessionInventory();
		SyncRaidInventoryToClient();
		return true;
	}

	ServerSortRaidInventory();
	return true;
}

void ALSPlayerControllerBase::ServerSortRaidInventory_Implementation()
{
	ULSRaidInventoryComponent* InventoryComponent = GetRaidInventoryComponent();
	if (!InventoryComponent || !InventoryComponent->IsRaidActive())
	{
		return;
	}

	InventoryComponent->SortSessionInventory();
	SyncRaidInventoryToClient();
}

void ALSPlayerControllerBase::SyncRaidSessionAndLootFromServer(ALSLootBox* SourceLootBox)
{
	if (!HasAuthority())
	{
		return;
	}

	ULSRaidInventoryComponent* InventoryComponent = GetRaidInventoryComponent();
	if (!InventoryComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot sync raid inventory because RaidInventoryComponent is missing on %s."), *GetNameSafe(this));
		return;
	}

	const TArray<FLSDropResult> LootResults = SourceLootBox ? SourceLootBox->GetLootResults() : TArray<FLSDropResult>();
	ClientSyncRaidSessionAndLoot(
		SourceLootBox,
		InventoryComponent->GetSessionInventory(),
		InventoryComponent->GetSessionSafeInventory(),
		LootResults);
}

bool ALSPlayerControllerBase::TransferLootDropSlotToSessionInternal(ALSLootBox* SourceLootBox, const int32 LootSlotIndex, FLSSessionItem& OutLootItem)
{
	if (!SourceLootBox)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot drop slot because source loot box is missing."));
		return false;
	}

	return SourceLootBox->TransferLootSlotToSession(LootSlotIndex, GetRaidInventoryComponent(), OutLootItem);
}

bool ALSPlayerControllerBase::TransferLootDropSlotToSessionSlotInternal(ALSLootBox* SourceLootBox, const int32 LootSlotIndex, const ELSInventorySlotArea ToSlotArea, const int32 ToSlotIndex, FLSSessionItem& OutLootItem)
{
	if (!SourceLootBox)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot drop slot to inventory slot because source loot box is missing."));
		return false;
	}

	return SourceLootBox->TransferLootSlotToSessionSlot(LootSlotIndex, GetRaidInventoryComponent(), ToSlotArea, ToSlotIndex, OutLootItem);
}

bool ALSPlayerControllerBase::TransferSessionSlotToLootDropSlotInternal(ALSLootBox* SourceLootBox, const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex, const int32 LootSlotIndex, FLSSessionItem& OutLootItem)
{
	if (!SourceLootBox)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer session slot to loot drop because source loot box is missing."));
		return false;
	}

	return SourceLootBox->TransferSessionSlotToLootSlot(LootSlotIndex, GetRaidInventoryComponent(), FromSlotArea, FromSlotIndex, OutLootItem);
}

bool ALSPlayerControllerBase::DropSessionSlotToWorld(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass)
{
	if (HasAuthority())
	{
		return DropSessionSlotToWorldInternal(SlotArea, SlotIndex, DroppedItemClass);
	}

	ServerDropSessionSlotToWorld(SlotArea, SlotIndex, DroppedItemClass);
	return true;
}

void ALSPlayerControllerBase::ServerDropSessionSlotToWorld_Implementation(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass)
{
	DropSessionSlotToWorldInternal(SlotArea, SlotIndex, DroppedItemClass);
}

bool ALSPlayerControllerBase::DropSessionSlotToWorldInternal(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass)
{
	if (!HasAuthority())
	{
		return false;
	}

	if (SlotArea != ELSInventorySlotArea::Inventory)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop non-inventory slot to world. Area=%d Index=%d"), static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	ULSRaidInventoryComponent* InventoryComponent = GetRaidInventoryComponent();
	if (!InventoryComponent || !InventoryComponent->IsRaidActive())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because raid session is not active."));
		return false;
	}

	FLSSessionItem SlotItem;
	if (!InventoryComponent->GetSessionSlotItem(SlotArea, SlotIndex, SlotItem))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because source slot is invalid. Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	FTransform SpawnTransform;
	if (!ResolveServerDroppedItemTransform(SpawnTransform))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because server drop transform is invalid."));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because World is missing."));
		return false;
	}

	if (!InventoryComponent->ClearSessionSlot(SlotArea, SlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because source slot could not be cleared. Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	TSubclassOf<ALSWorldDroppedItem> ClassToSpawn = DroppedItemClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ALSWorldDroppedItem::StaticClass();
		UE_LOG(LogLS, Warning, TEXT("DroppedItemClass is not set. Spawning native ALSWorldDroppedItem; interact hint widget class may be missing."));
	}

	ALSWorldDroppedItem* DroppedItem = World->SpawnActorDeferred<ALSWorldDroppedItem>(
		ClassToSpawn,
		SpawnTransform,
		nullptr,
		GetPawn(),
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (!DroppedItem)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to spawn dropped item for slot. Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		FLSSessionItem IgnoredPreviousItem;
		InventoryComponent->ReplaceSessionSlotItem(SlotArea, SlotIndex, SlotItem, IgnoredPreviousItem);
		return false;
	}

	DroppedItem->InitializeDroppedItem(SlotItem);
	DroppedItem->FinishSpawning(SpawnTransform);
	ClientSyncRaidSessionAndLoot(nullptr, InventoryComponent->GetSessionInventory(), InventoryComponent->GetSessionSafeInventory(), TArray<FLSDropResult>());
	return true;
}

bool ALSPlayerControllerBase::ResolveServerDroppedItemTransform(FTransform& OutDropTransform) const
{
	constexpr float DroppedItemGroundTraceDistance = 100.0f;
	constexpr float DroppedItemRandomGroundOffsetXY = 12.0f;
	constexpr float DroppedItemRandomGroundOffsetMin = 0.5f;
	constexpr float DroppedItemRandomGroundOffsetMax = 2.0f;

	const APawn* ControlledPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!ControlledPawn || !World)
	{
		return false;
	}

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	float CollisionRadius = 0.0f;
	float CollisionHalfHeight = 0.0f;
	ControlledPawn->GetSimpleCollisionCylinder(CollisionRadius, CollisionHalfHeight);

	const FVector FootLocation = PawnLocation - FVector(0.0f, 0.0f, CollisionHalfHeight);
	const FVector TraceStart = PawnLocation;
	const FVector TraceEnd = FootLocation - FVector(0.0f, 0.0f, DroppedItemGroundTraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LSDropInventoryItemToGround), false, ControlledPawn);
	const bool bHitGround = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	const FVector GroundLocation = bHitGround ? HitResult.ImpactPoint : FootLocation;
	const FVector2D RandomGroundOffset = FMath::RandPointInCircle(DroppedItemRandomGroundOffsetXY);
	const FVector DropLocation = GroundLocation + FVector(
		RandomGroundOffset.X,
		RandomGroundOffset.Y,
		FMath::FRandRange(DroppedItemRandomGroundOffsetMin, DroppedItemRandomGroundOffsetMax));

	float DropYaw = GetControlRotation().Yaw;
	if (const UCameraComponent* CameraComponent = ControlledPawn->FindComponentByClass<UCameraComponent>())
	{
		DropYaw = CameraComponent->GetComponentRotation().Yaw;
	}
	DropYaw = FRotator::NormalizeAxis(DropYaw + 180.0f);

	OutDropTransform = FTransform(FRotator(0.0f, DropYaw, 0.0f), DropLocation);
	return true;
}
