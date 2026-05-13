// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/LSPlayerControllerBase.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Characters/LSCharacterBase.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Gameplay/LSWorldDroppedItem.h"
#include "InputMappingContext.h"
#include "LostSignal.h"
#include "Session/LSSessionSubsystem.h"
#include "UI/Debug/LSHpDebugWidget.h"
#include "UI/LootDrop/LSLootDropWidget.h"

void ALSPlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

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

void ALSPlayerControllerBase::ShowLootDropWidget(const FText& LootSourceName, const TArray<FLSDropResult>& Results)
{
	if (IsLocalPlayerController())
	{
		ShowLootDropWidgetLocal(LootSourceName, Results);
		return;
	}

	ClientShowLootDropWidget(LootSourceName, Results);
}

void ALSPlayerControllerBase::ClientShowLootDropWidget_Implementation(const FText& LootSourceName, const TArray<FLSDropResult>& Results)
{
	ShowLootDropWidgetLocal(LootSourceName, Results);
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

void ALSPlayerControllerBase::ShowLootDropWidgetLocal(const FText& LootSourceName, const TArray<FLSDropResult>& Results)
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

bool ALSPlayerControllerBase::DropSessionSlotToWorld(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, const FVector& DropLocation, const float DropYaw)
{
	if (HasAuthority())
	{
		return DropSessionSlotToWorldInternal(SlotArea, SlotIndex, DroppedItemClass, DropLocation, DropYaw);
	}

	ServerDropSessionSlotToWorld(SlotArea, SlotIndex, DroppedItemClass, DropLocation, DropYaw);
	return true;
}

void ALSPlayerControllerBase::ServerDropSessionSlotToWorld_Implementation(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, const FVector_NetQuantize DropLocation, const float DropYaw)
{
	DropSessionSlotToWorldInternal(SlotArea, SlotIndex, DroppedItemClass, FVector(DropLocation), DropYaw);
}

bool ALSPlayerControllerBase::DropSessionSlotToWorldInternal(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, const FVector& DropLocation, const float DropYaw)
{
	if (!HasAuthority())
	{
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULSSessionSubsystem* SessionSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSessionSubsystem>() : nullptr;
	if (!SessionSubsystem || !SessionSubsystem->IsRaidActive())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because raid session is not active."));
		return false;
	}

	FLSSessionItem SlotItem;
	if (!SessionSubsystem->GetSessionSlotItem(SlotArea, SlotIndex, SlotItem))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because source slot is invalid. Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because World is missing."));
		return false;
	}

	TSubclassOf<ALSWorldDroppedItem> ClassToSpawn = DroppedItemClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ALSWorldDroppedItem::StaticClass();
	}
	const FTransform SpawnTransform(FRotator(0.0f, DropYaw, 0.0f), DropLocation);
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
		return false;
	}

	DroppedItem->InitializeDroppedItem(SlotItem);
	DroppedItem->FinishSpawning(SpawnTransform);

	if (!SessionSubsystem->ClearSessionSlot(SlotArea, SlotIndex))
	{
		DroppedItem->Destroy();
		return false;
	}

	return true;
}
