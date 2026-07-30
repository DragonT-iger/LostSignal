// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/LSPlayerControllerBase.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Characters/LSEquipmentStatComponent.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "Characters/LSPlayerCharacter.h"
#include "Core/LSFarmingGameMode.h"
#include "Core/LSLobbyGameMode.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "Gameplay/LSLootBox.h"
#include "Gameplay/LSNoiseTypes.h"
#include "Gameplay/LSWorldDroppedItem.h"
#include "InputMappingContext.h"
#include "Data/LSChipStats.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/Debug/LSHpDebugWidget.h"
#include "UI/Debug/LSProtocolDebugWidget.h"
#include "UI/LSBackgroundBlurWidget.h"
#include "UI/LSPlayerHUDWidget.h"
#include "UI/LSUILayer.h"
#include "UI/Settings/LSSettingsWidget.h"
#include "UI/Inventory/LSInventoryWidget.h"
#include "UI/LootDrop/LSLootDropWidget.h"
#include "UI/Protocol/LSProtocolUIWidget.h"
#include "UI/QuickSlot/LSQuickSlotBarWidget.h"
#include "UI/Storage/LSLobbyStorageWidget.h"
#include "UI/ChipSystem/LSChipStationWidget.h"

namespace
{
bool IsNoiseInstigatorPlayerForHUDDisplay(const AActor* NoiseInstigator)
{
	if (!NoiseInstigator)
	{
		return false;
	}

	if (NoiseInstigator->IsA<ALSPlayerCharacter>())
	{
		return true;
	}

	const APawn* InstigatorPawn = Cast<APawn>(NoiseInstigator);
	return InstigatorPawn && InstigatorPawn->IsPlayerControlled();
}
}

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

	CreatePlayerHUDWidgetLocal();
	CreateBackgroundBlurWidgetLocal();
}

void ALSPlayerControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	CreatePlayerHUDWidgetLocal();
}

void ALSPlayerControllerBase::AcknowledgePossession(APawn* InPawn)
{
	Super::AcknowledgePossession(InPawn);
	CreatePlayerHUDWidgetLocal();
}

void ALSPlayerControllerBase::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController() || bDefaultMappingContextsApplied)
	{
		return;
	}

	if (InputComponent)
	{
		// 시연용 프로토콜 패널 토글 (레거시 BindKey; Enhanced Input 과 공존).
		InputComponent->BindKey(EKeys::Insert, IE_Pressed, this, &ALSPlayerControllerBase::ToggleProtocolDebugWidget);
		// 레이드 중 세팅 화면 토글 (레거시 BindKey; 위 Insert 토글과 동일한 방식으로 공존).
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ALSPlayerControllerBase::ToggleRaidSettingsWidget);
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

	if (RaidInventoryComponent->IsRaidActive())
	{
		if (HasAuthority())
		{
			SyncRaidInventoryToClient();
		}
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULSSessionSubsystem* SessionSub = GameInstance ? GameInstance->GetSubsystem<ULSSessionSubsystem>() : nullptr;
	if (!SessionSub || !SessionSub->IsRaidActive())
	{
		return;
	}

	TArray<FLSSessionItem> PendingInventory;
	TArray<FLSSessionItem> PendingSafeInventory;
	TArray<FLSSessionItem> PendingEquipment;
	if (SessionSub->DequeuePendingRaidEntry(PendingInventory, PendingSafeInventory, PendingEquipment))
	{
		RaidInventoryComponent->StartRaidInventory(PendingInventory, PendingSafeInventory, PendingEquipment);
	}
	else
	{
		RaidInventoryComponent->MirrorRaidInventoryState(SessionSub->GetSessionInventory(), SessionSub->GetSessionSafeInventory(), SessionSub->GetSessionEquipmentSlots());
	}
	if (HasAuthority())
	{
		SyncRaidInventoryToClient();
	}
}

void ALSPlayerControllerBase::ClientStartRaidSession_Implementation(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems)
{
	if (!RaidInventoryComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot mirror raid inventory because RaidInventoryComponent is missing on %s."), *GetNameSafe(this));
		return;
	}

	RaidInventoryComponent->StartRaidInventory(Loadout, SafeItems, EquipmentItems);
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULSSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<ULSSaveSubsystem>())
		{
			SaveSubsystem->BeginRaidSave(Loadout);
		}
	}
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

void ALSPlayerControllerBase::ShowLobbyStorageWidget(TSubclassOf<ULSLobbyStorageWidget> LobbyStorageWidgetClass)
{
	if (IsLocalPlayerController())
	{
		ShowLobbyStorageWidgetLocal(LobbyStorageWidgetClass);
		return;
	}

	ClientShowLobbyStorageWidget(LobbyStorageWidgetClass);
}

void ALSPlayerControllerBase::HideLobbyStorageWidget()
{
	if (IsLocalPlayerController())
	{
		HideLobbyStorageWidgetLocal();
		return;
	}

	ClientHideLobbyStorageWidget();
}

bool ALSPlayerControllerBase::IsLobbyStorageWidgetOpen() const
{
	return LobbyStorageWidgetInstance && LobbyStorageWidgetInstance->IsVisible();
}

void ALSPlayerControllerBase::RefreshOpenLobbyStorageWidget()
{
	if (IsLobbyStorageWidgetOpen())
	{
		LobbyStorageWidgetInstance->RefreshStorage();
	}
}

void ALSPlayerControllerBase::RefreshOpenChipStationWidget()
{
	if (IsChipStationWidgetOpen())
	{
		ChipStationWidgetInstance->RefreshChipStation();
	}
}

void ALSPlayerControllerBase::ShowChipStationWidget(TSubclassOf<ULSChipStationWidget> ChipStationWidgetClass)
{
	if (IsLocalPlayerController())
	{
		ShowChipStationWidgetLocal(ChipStationWidgetClass);
		return;
	}

	ClientShowChipStationWidget(ChipStationWidgetClass);
}

void ALSPlayerControllerBase::HideChipStationWidget()
{
	if (IsLocalPlayerController())
	{
		HideChipStationWidgetLocal();
		return;
	}

	ClientHideChipStationWidget();
}

bool ALSPlayerControllerBase::IsChipStationWidgetOpen() const
{
	return ChipStationWidgetInstance && ChipStationWidgetInstance->IsVisible();
}

void ALSPlayerControllerBase::RegisterLobbyInventoryWidget(ULSInventoryWidget* InWidget)
{
	LobbyInventoryWidgetInstance = InWidget;
}

void ALSPlayerControllerBase::UnregisterLobbyInventoryWidget(const ULSInventoryWidget* InWidget)
{
	if (LobbyInventoryWidgetInstance == InWidget)
	{
		LobbyInventoryWidgetInstance = nullptr;
	}
}

void ALSPlayerControllerBase::RegisterQuickSlotBar(ULSQuickSlotBarWidget* InWidget)
{
	if (!InWidget)
	{
		return;
	}

	// 무효 참조 정리 후 중복 없이 등록한다(인벤토리 바 + HUD 바 공존 가능).
	RegisteredQuickSlotBars.RemoveAll([](const TWeakObjectPtr<ULSQuickSlotBarWidget>& Bar) { return !Bar.IsValid(); });
	RegisteredQuickSlotBars.AddUnique(InWidget);
}

void ALSPlayerControllerBase::UnregisterQuickSlotBar(const ULSQuickSlotBarWidget* InWidget)
{
	RegisteredQuickSlotBars.RemoveAll([InWidget](const TWeakObjectPtr<ULSQuickSlotBarWidget>& Bar)
	{
		return !Bar.IsValid() || Bar.Get() == InWidget;
	});
}

void ALSPlayerControllerBase::ShowCastGauge(const FText& Label, const float Duration)
{
	if (!IsLocalPlayerController() || !PlayerHUDWidgetInstance)
	{
		return;
	}

	PlayerHUDWidgetInstance->ShowSkillCastGauge(Label, FMath::Max(Duration, 0.0f));
}

void ALSPlayerControllerBase::HideCastGauge()
{
	if (PlayerHUDWidgetInstance)
	{
		PlayerHUDWidgetInstance->HideSkillCastGauge();
	}
}

void ALSPlayerControllerBase::RegisterLobbyStorageWidget(ULSLobbyStorageWidget* InWidget)
{
	LobbyStorageWidgetInstance = InWidget;
}

void ALSPlayerControllerBase::UnregisterLobbyStorageWidget(const ULSLobbyStorageWidget* InWidget)
{
	if (LobbyStorageWidgetInstance == InWidget)
	{
		LobbyStorageWidgetInstance = nullptr;
	}
}

void ALSPlayerControllerBase::RefreshActiveInventoryWidget()
{
	// 폰이 있으면 폰의 인벤토리 위젯을, 없으면(로비) 등록된 로비 인벤토리 위젯을 갱신한다.
	if (ALSPlayerCharacter* PlayerCharacter = Cast<ALSPlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->RebuildInventoryWidgetSlots();
		return;
	}

	if (LobbyInventoryWidgetInstance)
	{
		LobbyInventoryWidgetInstance->RebuildInventorySlots();
		// Safe(영구보관) 영역도 함께 갱신한다. 폰 경로(RebuildInventoryWidgetSlots)는 둘 다 갱신하지만
		// 폰 없는 로비에서 이 함수만 인벤토리만 갱신하면, Safe→창고 이동 후 비워진 Safe 슬롯이 stale로 남는다.
		LobbyInventoryWidgetInstance->RebuildConfirmedStorageSlots();
	}
}

void ALSPlayerControllerBase::RefreshAllInventoryUI()
{
	// 인벤토리 + Safe (폰/로비 분기)는 기존 단일 경로를 재사용한다.
	RefreshActiveInventoryWidget();

	// 로비 장비칸은 폰 경로(RebuildInventoryWidgetSlots)가 갱신하지 않는다. 장비 편집은 로비 전용이므로,
	// 폰이 없는 로비에서만 등록된 로비 인벤토리 위젯의 장비칸을 함께 다시 그린다.
	if (!Cast<ALSPlayerCharacter>(GetPawn()) && LobbyInventoryWidgetInstance)
	{
		LobbyInventoryWidgetInstance->RebuildEquipmentSlots();
	}

	// 창고·칩 스테이션은 열려 있을 때만 각자 데이터에서 다시 그린다(내부에서 가시성 체크).
	RefreshOpenLobbyStorageWidget();
	RefreshOpenChipStationWidget();

	// 퀵슬롯 개수는 인벤토리에서 실시간 합산하므로, 인벤토리가 바뀔 때마다 등록된 모든 바(인벤토리+HUD)를 다시 그린다.
	RefreshRegisteredQuickSlotBars();
}

void ALSPlayerControllerBase::RefreshRegisteredQuickSlotBars()
{
	for (int32 Index = RegisteredQuickSlotBars.Num() - 1; Index >= 0; --Index)
	{
		if (ULSQuickSlotBarWidget* Bar = RegisteredQuickSlotBars[Index].Get())
		{
			Bar->RefreshAll();
		}
		else
		{
			RegisteredQuickSlotBars.RemoveAt(Index);
		}
	}
}

bool ALSPlayerControllerBase::IsInventoryUIOpen() const
{
	if (const ALSPlayerCharacter* PlayerCharacter = Cast<ALSPlayerCharacter>(GetPawn()))
	{
		return PlayerCharacter->IsInventoryWidgetOpen();
	}

	return LobbyInventoryWidgetInstance && LobbyInventoryWidgetInstance->IsVisible();
}

void ALSPlayerControllerBase::ClientShowLobbyStorageWidget_Implementation(TSubclassOf<ULSLobbyStorageWidget> LobbyStorageWidgetClass)
{
	ShowLobbyStorageWidgetLocal(LobbyStorageWidgetClass);
}

void ALSPlayerControllerBase::ClientHideLobbyStorageWidget_Implementation()
{
	HideLobbyStorageWidgetLocal();
}

void ALSPlayerControllerBase::ClientShowChipStationWidget_Implementation(TSubclassOf<ULSChipStationWidget> ChipStationWidgetClass)
{
	ShowChipStationWidgetLocal(ChipStationWidgetClass);
}

void ALSPlayerControllerBase::ClientHideChipStationWidget_Implementation()
{
	HideChipStationWidgetLocal();
}

void ALSPlayerControllerBase::ClientSyncRaidSessionAndLoot_Implementation(ALSLootBox* SourceLootBox, const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems, const TArray<FLSDropResult>& LootResults)
{
	if (!RaidInventoryComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot sync raid inventory because RaidInventoryComponent is missing on %s."), *GetNameSafe(this));
		return;
	}

	RaidInventoryComponent->MirrorRaidInventoryState(InventoryItems, SafeItems, EquipmentItems);

	// 미러 데이터가 갱신됐으니 열려 있는 인벤토리 계열 패널 전체를 funnel로 다시 그린다.
	RefreshAllInventoryUI();

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
		RaidInventoryComponent->GetSessionEquipmentSlots(),
		TArray<FLSDropResult>());
}

void ALSPlayerControllerBase::RequestRaidEntryDataForRaidStart()
{
	ClearSubmittedRaidEntryData();
	if (IsLocalPlayerController())
	{
		SubmitLocalRaidEntryData();
		return;
	}

	ClientRequestRaidEntryData();
}

void ALSPlayerControllerBase::ClearSubmittedRaidEntryData()
{
	bHasSubmittedRaidEntryData = false;
	SubmittedRaidLoadout.Reset();
	SubmittedRaidSafeItems.Reset();
	SubmittedRaidEquipment.Reset();
}

void ALSPlayerControllerBase::ClientRequestRaidEntryData_Implementation()
{
	SubmitLocalRaidEntryData();
}

void ALSPlayerControllerBase::ServerSubmitRaidEntryData_Implementation(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems)
{
	StoreSubmittedRaidEntryData(Loadout, SafeItems, EquipmentItems);

	if (ALSLobbyGameMode* LobbyGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALSLobbyGameMode>() : nullptr)
	{
		LobbyGameMode->NotifyRaidEntryDataSubmitted(this);
	}
}

void ALSPlayerControllerBase::SubmitLocalRaidEntryData()
{
	TArray<FLSSessionItem> Loadout;
	TArray<FLSSessionItem> SafeItems;
	TArray<FLSSessionItem> EquipmentItems;

	UGameInstance* GameInstance = GetGameInstance();
	const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (SaveSubsystem)
	{
		Loadout = SaveSubsystem->GetInventory();
		SafeItems = SaveSubsystem->GetSafeStash();
		EquipmentItems = SaveSubsystem->GetEquipmentSlots();
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot submit raid entry data because SaveSubsystem is missing on %s."), *GetNameSafe(this));
	}

	if (HasAuthority())
	{
		StoreSubmittedRaidEntryData(Loadout, SafeItems, EquipmentItems);
		if (ALSLobbyGameMode* LobbyGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALSLobbyGameMode>() : nullptr)
		{
			LobbyGameMode->NotifyRaidEntryDataSubmitted(this);
		}
		return;
	}

	ServerSubmitRaidEntryData(Loadout, SafeItems, EquipmentItems);
}

void ALSPlayerControllerBase::StoreSubmittedRaidEntryData(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems)
{
	if (!HasAuthority())
	{
		return;
	}

	SubmittedRaidLoadout = Loadout;
	SubmittedRaidSafeItems = SafeItems;
	LSInventorySlotUtils::NormalizeSlotArray(SubmittedRaidLoadout);
	LSInventorySlotUtils::NormalizeSlotArray(SubmittedRaidSafeItems);
	// 장비는 인덱스=슬롯 타입이므로 Normalize(빈 칸 압축) 금지 — 5칸 패딩만 한다.
	SubmittedRaidEquipment = EquipmentItems;
	SubmittedRaidEquipment.SetNum(LSInventorySlotUtils::EquipmentSlotCount);
	bHasSubmittedRaidEntryData = true;

	UE_LOG(LogLS, Log, TEXT("Raid entry data submitted for %s. LoadoutSlots=%d SafeSlots=%d EquipmentSlots=%d"),
		*GetNameSafe(this),
		SubmittedRaidLoadout.Num(),
		SubmittedRaidSafeItems.Num(),
		SubmittedRaidEquipment.Num());
}

void ALSPlayerControllerBase::RequestRaidResultSave(const ELSRaidResult Result, const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems, const bool bSaveInventory, const bool bSaveSafeStash, const bool bSaveEquipment)
{
	if (IsLocalPlayerController())
	{
		ApplyRaidResultToLocalSave(Result, InventoryItems, SafeItems, EquipmentItems, bSaveInventory, bSaveSafeStash, bSaveEquipment);
		return;
	}

	ClientApplyRaidResult(Result, InventoryItems, SafeItems, EquipmentItems, bSaveInventory, bSaveSafeStash, bSaveEquipment);
}

void ALSPlayerControllerBase::ClientApplyRaidResult_Implementation(const ELSRaidResult Result, const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems, const bool bSaveInventory, const bool bSaveSafeStash, const bool bSaveEquipment)
{
	ApplyRaidResultToLocalSave(Result, InventoryItems, SafeItems, EquipmentItems, bSaveInventory, bSaveSafeStash, bSaveEquipment);
}

void ALSPlayerControllerBase::ApplyRaidResultToLocalSave(const ELSRaidResult Result, const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems, const bool bSaveInventory, const bool bSaveSafeStash, const bool bSaveEquipment)
{
	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot apply raid result because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return;
	}

	if (bSaveInventory)
	{
		SaveSubsystem->ReplaceInventory(InventoryItems);
	}

	if (bSaveSafeStash)
	{
		SaveSubsystem->ReplaceSafeStash(SafeItems);
	}

	// bSaveEquipment=false(Quit)면 세이브의 장착 상태가 입장 시점 그대로라 저장 생략이 곧 복구다.
	if (bSaveEquipment)
	{
		SaveSubsystem->ReplaceEquipmentSlots(EquipmentItems);
	}

	SaveSubsystem->ClearRaidSave();

	UE_LOG(LogLS, Log, TEXT("Raid result applied locally on %s. Result=%d SaveInventory=%s SaveSafe=%s SaveEquipment=%s InventorySlots=%d SafeSlots=%d"),
		*GetNameSafe(this),
		static_cast<int32>(Result),
		bSaveInventory ? TEXT("true") : TEXT("false"),
		bSaveSafeStash ? TEXT("true") : TEXT("false"),
		bSaveEquipment ? TEXT("true") : TEXT("false"),
		InventoryItems.Num(),
		SafeItems.Num());

	if (HasAuthority())
	{
		if (ALSFarmingGameMode* FarmingGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALSFarmingGameMode>() : nullptr)
		{
			FarmingGameMode->NotifyRaidResultSaved(this);
		}
		return;
	}

	ServerConfirmRaidResultSaved();
}

void ALSPlayerControllerBase::ServerConfirmRaidResultSaved_Implementation()
{
	if (ALSFarmingGameMode* FarmingGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALSFarmingGameMode>() : nullptr)
	{
		FarmingGameMode->NotifyRaidResultSaved(this);
	}
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
		LootDropWidgetInstance->AddToViewport(LSUILayer::ModalPanel);
	}

	LootDropWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	LootDropWidgetInstance->SetLootSourceName(LootSourceName);
	LootDropWidgetInstance->SetSourceLootBox(SourceLootBox);
	LootDropWidgetInstance->SetLootItems(Results);
	UpdateBackgroundBlurVisibility();
}

void ALSPlayerControllerBase::HideLootDropWidgetLocal()
{
	if (LootDropWidgetInstance)
	{
		LootDropWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
		LootDropWidgetInstance->ClearLootItems();
	}

	UpdateBackgroundBlurVisibility();
}

void ALSPlayerControllerBase::ShowLobbyStorageWidgetLocal(TSubclassOf<ULSLobbyStorageWidget> LobbyStorageWidgetClass)
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (!LobbyStorageWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("LobbyStorageWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	if (LobbyStorageWidgetInstance && LobbyStorageWidgetInstance->GetClass() != LobbyStorageWidgetClass)
	{
		LobbyStorageWidgetInstance->RemoveFromParent();
		LobbyStorageWidgetInstance = nullptr;
	}

	if (!LobbyStorageWidgetInstance)
	{
		LobbyStorageWidgetInstance = CreateWidget<ULSLobbyStorageWidget>(this, LobbyStorageWidgetClass);
		if (!LobbyStorageWidgetInstance)
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create lobby storage widget on %s."), *GetNameSafe(this));
			return;
		}
	}

	if (!LobbyStorageWidgetInstance->IsInViewport())
	{
		LobbyStorageWidgetInstance->AddToViewport(LSUILayer::ModalPanel);
	}

	LobbyStorageWidgetInstance->RefreshStorage();
	LobbyStorageWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UpdateBackgroundBlurVisibility();
}

void ALSPlayerControllerBase::HideLobbyStorageWidgetLocal()
{
	if (LobbyStorageWidgetInstance)
	{
		LobbyStorageWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	UpdateBackgroundBlurVisibility();
}

void ALSPlayerControllerBase::ShowChipStationWidgetLocal(TSubclassOf<ULSChipStationWidget> ChipStationWidgetClass)
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (!ChipStationWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("ChipStationWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	if (ChipStationWidgetInstance && ChipStationWidgetInstance->GetClass() != ChipStationWidgetClass)
	{
		ChipStationWidgetInstance->RemoveFromParent();
		ChipStationWidgetInstance = nullptr;
	}

	if (!ChipStationWidgetInstance)
	{
		ChipStationWidgetInstance = CreateWidget<ULSChipStationWidget>(this, ChipStationWidgetClass);
		if (!ChipStationWidgetInstance)
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create chip station widget on %s."), *GetNameSafe(this));
			return;
		}
	}

	if (!ChipStationWidgetInstance->IsInViewport())
	{
		ChipStationWidgetInstance->AddToViewport(LSUILayer::ModalPanel);
	}

	ChipStationWidgetInstance->RefreshChipStation();
	ChipStationWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UpdateBackgroundBlurVisibility();
}

void ALSPlayerControllerBase::HideChipStationWidgetLocal()
{
	if (ChipStationWidgetInstance)
	{
		ChipStationWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	UpdateBackgroundBlurVisibility();
}

void ALSPlayerControllerBase::CreatePlayerHUDWidgetLocal()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	// HUD(미니맵/체력 등)는 폰이 있어야 의미가 있다. 폰이 없으면(예: 로비) 만들지 않는다.
	// 폰을 점유하면 OnPossess/AcknowledgePossession에서 다시 호출돼 그때 생성된다.
	APawn* CurrentPawn = GetPawn();
	if (!CurrentPawn)
	{
		return;
	}

	if (!PlayerHUDWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("PlayerHUDWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	if (!PlayerHUDWidgetInstance)
	{
		PlayerHUDWidgetInstance = CreateWidget<ULSPlayerHUDWidget>(this, PlayerHUDWidgetClass);
		if (!PlayerHUDWidgetInstance)
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create player HUD widget on %s."), *GetNameSafe(this));
			return;
		}
	}

	if (!PlayerHUDWidgetInstance->IsInViewport())
	{
		PlayerHUDWidgetInstance->AddToViewport();
	}

	PlayerHUDWidgetInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	PlayerHUDWidgetInstance->InitializeHUDForPawn(CurrentPawn);
}

void ALSPlayerControllerBase::CreateBackgroundBlurWidgetLocal()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (!BackgroundBlurWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("BackgroundBlurWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	if (!BackgroundBlurWidgetInstance)
	{
		BackgroundBlurWidgetInstance = CreateWidget<ULSBackgroundBlurWidget>(this, BackgroundBlurWidgetClass);
		if (!BackgroundBlurWidgetInstance)
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create modal backdrop widget on %s."), *GetNameSafe(this));
			return;
		}
	}

	if (!BackgroundBlurWidgetInstance->IsInViewport())
	{
		BackgroundBlurWidgetInstance->AddToViewport(LSUILayer::BackgroundBlur);
	}

	// 패널 뒤에 상주시키되 평상시엔 숨겨 둔다. 표시는 UpdateBackgroundBlurVisibility가 켠다.
	BackgroundBlurWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
}

bool ALSPlayerControllerBase::IsAnyModalPanelOpen() const
{
	if (LootDropWidgetInstance && LootDropWidgetInstance->IsVisible())
	{
		return true;
	}

	if (LobbyStorageWidgetInstance && LobbyStorageWidgetInstance->IsVisible())
	{
		return true;
	}

	if (ChipStationWidgetInstance && ChipStationWidgetInstance->IsVisible())
	{
		return true;
	}

	// 인벤토리 위젯은 Pawn(ALSPlayerCharacter)이 소유하므로 Pawn에 상태를 묻는다.
	if (const ALSPlayerCharacter* PlayerCharacter = Cast<ALSPlayerCharacter>(GetPawn()))
	{
		if (PlayerCharacter->IsInventoryWidgetOpen())
		{
			return true;
		}
	}

	return false;
}

void ALSPlayerControllerBase::UpdateBackgroundBlurVisibility()
{
	if (!IsLocalPlayerController() || !BackgroundBlurWidgetInstance)
	{
		return;
	}

	// 입력은 위에 깔린 패널이 받도록 표시 시 HitTestInvisible로 둔다.
	const ESlateVisibility NewVisibility = IsAnyModalPanelOpen()
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed;
	BackgroundBlurWidgetInstance->SetVisibility(NewVisibility);
}

void ALSPlayerControllerBase::NotifyNoiseForHUD(const FLSNoiseEvent& NoiseEvent)
{
	if (NoiseEvent.RadiusCm <= 0.0f || GetPawn() == NoiseEvent.NoiseInstigator || IsNoiseInstigatorPlayerForHUDDisplay(NoiseEvent.NoiseInstigator))
	{
		return;
	}

	if (IsLocalPlayerController())
	{
		HandleNoiseForHUD(NoiseEvent.Location, NoiseEvent.RadiusCm, NoiseEvent.NoiseTag, NoiseEvent.NoiseInstigator);
		return;
	}

	ClientReceiveNoiseForHUD(NoiseEvent.Location, NoiseEvent.RadiusCm, NoiseEvent.NoiseTag, NoiseEvent.NoiseInstigator);
}

void ALSPlayerControllerBase::ShowDamageNumber(const FLSDamageNumberPayload& Payload)
{
	if (Payload.DamageAmount <= 0.0f)
	{
		return;
	}

	if (IsLocalPlayerController())
	{
		ShowDamageNumberLocal(Payload);
		return;
	}

	ClientShowDamageNumber(Payload);
}

float ALSPlayerControllerBase::GetSoundIndicatorDetectionRadiusCm() const
{
	return FMath::Max(0.0f, SoundIndicatorDetectionRadiusMeters) * 100.0f;
}

void ALSPlayerControllerBase::ClientReceiveNoiseForHUD_Implementation(
	const FVector_NetQuantize NoiseLocation,
	const float RadiusCm,
	const FGameplayTag NoiseTag,
	AActor* NoiseInstigator)
{
	HandleNoiseForHUD(NoiseLocation, RadiusCm, NoiseTag, NoiseInstigator);
}

void ALSPlayerControllerBase::HandleNoiseForHUD(
	const FVector NoiseLocation,
	const float RadiusCm,
	const FGameplayTag NoiseTag,
	AActor* NoiseInstigator)
{
	if (!IsLocalPlayerController() || !PlayerHUDWidgetInstance || GetPawn() == NoiseInstigator || IsNoiseInstigatorPlayerForHUDDisplay(NoiseInstigator))
	{
		return;
	}

	PlayerHUDWidgetInstance->HandleNoiseForSoundIndicator(NoiseLocation, RadiusCm, NoiseTag, NoiseInstigator);
}

void ALSPlayerControllerBase::ClientShowDamageNumber_Implementation(const FLSDamageNumberPayload& Payload)
{
	ShowDamageNumberLocal(Payload);
}

void ALSPlayerControllerBase::ShowDamageNumberLocal(const FLSDamageNumberPayload& Payload)
{
	if (!IsLocalPlayerController() || !PlayerHUDWidgetInstance)
	{
		return;
	}

	PlayerHUDWidgetInstance->ShowDamageNumber(Payload);
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
			// 룻박스에서 장착칸으로 직접 장착하면 장비 스탯을 재적용한다.
			RefreshEquipmentStatsIfEquipmentTouched(ToSlotArea, ToSlotArea);
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
	if (bTransferred)
	{
		// 룻박스에서 장착칸으로 직접 장착하면 장비 스탯을 재적용한다.
		RefreshEquipmentStatsIfEquipmentTouched(ToSlotArea, ToSlotArea);
	}
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
			// 장착칸을 룻박스로 이송하면 즉시 해제이므로 장비 스탯을 재적용한다.
			RefreshEquipmentStatsIfEquipmentTouched(FromSlotArea, FromSlotArea);
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
	if (bTransferred)
	{
		// 장착칸을 룻박스로 이송하면 즉시 해제이므로 장비 스탯을 재적용한다.
		RefreshEquipmentStatsIfEquipmentTouched(FromSlotArea, FromSlotArea);
	}
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

void ALSPlayerControllerBase::LSTestSurvivalProtocol(const int32 Level)
{
	SetProtocolTestLevel(ELSProtocolType::Survival, Level);
	RefreshProtocolTestTargets();

	UE_LOG(LogLS, Log, TEXT("[SurvivalProtocolTest] OverrideLevel=%d"), SurvivalProtocolTestLevel);
}

void ALSPlayerControllerBase::LSTestCarryingProtocol(const int32 Level)
{
	SetProtocolTestLevel(ELSProtocolType::Carrying, Level);
	RefreshProtocolTestTargets();

	UE_LOG(LogLS, Log, TEXT("[CarryingProtocolTest] OverrideLevel=%d"), CarryingProtocolTestLevel);
}

void ALSPlayerControllerBase::LSTestBattleProtocol(const int32 Level)
{
	SetProtocolTestLevel(ELSProtocolType::Battle, Level);
	RefreshProtocolTestTargets();

	UE_LOG(LogLS, Log, TEXT("[BattleProtocolTest] OverrideLevel=%d"), BattleProtocolTestLevel);
}

void ALSPlayerControllerBase::LSTestNavigationProtocol(const int32 Level)
{
	SetProtocolTestLevel(ELSProtocolType::Navigation, Level);
	RefreshProtocolTestTargets();

	UE_LOG(LogLS, Log, TEXT("[NavigationProtocolTest] OverrideLevel=%d"), NavigationProtocolTestLevel);
}

void ALSPlayerControllerBase::LSTestAllProtocols(const int32 Survival, const int32 Carrying, const int32 Battle, const int32 Navigation)
{
	SetProtocolTestLevel(ELSProtocolType::Survival, Survival);
	SetProtocolTestLevel(ELSProtocolType::Carrying, Carrying);
	SetProtocolTestLevel(ELSProtocolType::Battle, Battle);
	SetProtocolTestLevel(ELSProtocolType::Navigation, Navigation);
	RefreshProtocolTestTargets();

	UE_LOG(LogLS, Log, TEXT("[ProtocolTest] Survival=%d Carrying=%d Battle=%d Navigation=%d"),
		SurvivalProtocolTestLevel,
		CarryingProtocolTestLevel,
		BattleProtocolTestLevel,
		NavigationProtocolTestLevel);
}

void ALSPlayerControllerBase::LSTestSkillCastGauge(const float Duration)
{
	if (!PlayerHUDWidgetInstance)
	{
		CreatePlayerHUDWidgetLocal();
	}

	if (!PlayerHUDWidgetInstance)
	{
		UE_LOG(LogLS, Warning, TEXT("[SkillCastGaugeTest] Cannot show cast gauge because player HUD is missing."));
		return;
	}

	PlayerHUDWidgetInstance->ShowSkillCastGauge(NSLOCTEXT("LSPlayerControllerBase", "TestSkillCastGauge", "캐스팅"), FMath::Max(Duration, 0.0f));
}

void ALSPlayerControllerBase::LSClearSurvivalProtocolTest()
{
	SurvivalProtocolTestLevel = 0;
	RefreshProtocolTestTargets();

	UE_LOG(LogLS, Log, TEXT("[SurvivalProtocolTest] OverrideLevel=0"));
}

void ALSPlayerControllerBase::LSClearProtocolTest()
{
	SurvivalProtocolTestLevel = 0;
	CarryingProtocolTestLevel = 0;
	BattleProtocolTestLevel = 0;
	NavigationProtocolTestLevel = 0;
	RefreshProtocolTestTargets();

	UE_LOG(LogLS, Log, TEXT("[ProtocolTest] Survival=0 Carrying=0 Battle=0 Navigation=0"));
}

void ALSPlayerControllerBase::LSToggleProtocolDebug()
{
	ToggleProtocolDebugWidget();
}

void ALSPlayerControllerBase::ToggleProtocolDebugWidget()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (!ProtocolDebugWidgetInstance)
	{
		// C++ 전용 위젯이라 Blueprint 에셋 없이 StaticClass 로 생성한다.
		ProtocolDebugWidgetInstance = CreateWidget<ULSProtocolDebugWidget>(this, ULSProtocolDebugWidget::StaticClass());
		if (ProtocolDebugWidgetInstance)
		{
			ProtocolDebugWidgetInstance->AddToViewport(LSUILayer::ProtocolDebug);
		}
		// 패널이 떠서 오버라이드가 활성화되었으므로 프로토콜 표시를 다시 그린다.
		RefreshProtocolTestTargets();
		return;
	}

	const bool bCurrentlyVisible = ProtocolDebugWidgetInstance->GetVisibility() != ESlateVisibility::Collapsed;
	const bool bWillBeVisible = !bCurrentlyVisible;
	// Visible 로 켜면 풀스크린 루트 캔버스가 히트테스트를 전부 먹어 아래 UI(루트박스 등) 클릭이 막힌다.
	// SelfHitTestInvisible 이면 패널 버튼(자식)은 클릭되고 빈 영역은 클릭이 통과한다.
	ProtocolDebugWidgetInstance->SetVisibility(bWillBeVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	// 다시 켤 때 현재 효과 레벨로 패널 숫자를 갱신한다.
	if (bWillBeVisible)
	{
		ProtocolDebugWidgetInstance->RefreshLevelTexts();
	}
	// 패널 표시/숨김에 따라 오버라이드 적용 여부가 바뀌므로 프로토콜 표시를 다시 그린다.
	RefreshProtocolTestTargets();
}

bool ALSPlayerControllerBase::IsProtocolDebugWidgetVisible() const
{
	return ProtocolDebugWidgetInstance && ProtocolDebugWidgetInstance->GetVisibility() != ESlateVisibility::Collapsed;
}

void ALSPlayerControllerBase::ToggleRaidSettingsWidget()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	// 레이드 중이 아니면 무시한다. (로비 등에서 ESC 오작동 방지)
	if (!RaidInventoryComponent || !RaidInventoryComponent->IsRaidActive())
	{
		return;
	}

	// 이미 떠 있으면 ESC로 닫는다. 위젯이 스스로 RemoveFromParent + OnBackToMenu 브로드캐스트 →
	// HandleRaidSettingsClosed가 캐시를 비운다. (위젯에 포커스가 있으면 위젯의 NativeOnKeyDown이 먼저
	// ESC를 소비하므로 이 분기는 포커스가 게임 뷰포트에 있을 때의 폴백이다.)
	if (RaidSettingsWidgetInstance)
	{
		RaidSettingsWidgetInstance->CloseSettings();
		return;
	}

	// 루트박스가 떠 있으면 설정을 열지 않고 루트박스를 먼저 닫는다.
	if (LootDropWidgetInstance && LootDropWidgetInstance->IsVisible())
	{
		HideLootDropWidget();
		return;
	}

	if (!RaidSettingsWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("RaidSettingsWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	RaidSettingsWidgetInstance = CreateWidget<ULSSettingsWidget>(this, RaidSettingsWidgetClass);
	if (RaidSettingsWidgetInstance)
	{
		// BackButton/ESC로 스스로 닫히면 캐시를 비워, 다음 ESC에서 다시 생성되도록 한다.
		RaidSettingsWidgetInstance->OnBackToMenu.AddDynamic(this, &ALSPlayerControllerBase::HandleRaidSettingsClosed);
		RaidSettingsWidgetInstance->AddToViewport(LSUILayer::Settings);
	}
}

void ALSPlayerControllerBase::HandleRaidSettingsClosed()
{
	// 위젯은 스스로 RemoveFromParent 했으므로 캐시만 비우면 된다.
	RaidSettingsWidgetInstance = nullptr;
}

int32 ALSPlayerControllerBase::GetEffectiveProtocolLevel(const ELSProtocolType ProtocolType) const
{
	if (HasProtocolTestLevel(ProtocolType))
	{
		return GetProtocolTestLevel(ProtocolType);
	}

	const UGameInstance* GameInstance = GetGameInstance();
	const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		return 0;
	}

	const int32 InactiveSlotCount = LSChipStats::ResolveInactiveSignalSlotCount(SaveSubsystem->GetChipSignalGaugePercent());
	const TArray<FLSSessionItem> ActiveItems = LSChipStats::BuildSignalActiveEquipmentItems(SaveSubsystem->GetChipEquipmentSlots(), InactiveSlotCount);
	const FLSChipProtocolTotals Totals = LSChipStats::AggregateChipProtocolTotals(ActiveItems, this);
	switch (ProtocolType)
	{
	case ELSProtocolType::Survival:
		return Totals.Survival;
	case ELSProtocolType::Carrying:
		return Totals.Carrying;
	case ELSProtocolType::Battle:
		return Totals.Battle;
	case ELSProtocolType::Navigation:
		return Totals.Navigation;
	default:
		return 0;
	}
}

int32 ALSPlayerControllerBase::GetUnlockedQuickSlotCount() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		return 0;
	}

	// 디버그 패널이 떠 있고 적재 오버라이드가 설정돼 있으면 그 레벨로 해금 칸 수를 계산한다.
	// (스킬 바 ResolveBattleProtocolLevels / 칩스테이션 ResolveProtocolPreviewLevels와 동일한 게이트.)
	if (HasProtocolTestLevel(ELSProtocolType::Carrying))
	{
		return SaveSubsystem->GetUnlockedQuickSlotCountForCarryingLevel(GetProtocolTestLevel(ELSProtocolType::Carrying));
	}

	return SaveSubsystem->GetUnlockedQuickSlotCount();
}

bool ALSPlayerControllerBase::HasProtocolTestLevel(const ELSProtocolType ProtocolType) const
{
	return IsProtocolDebugWidgetVisible() && GetProtocolTestLevel(ProtocolType) >= 0;
}

int32 ALSPlayerControllerBase::GetProtocolTestLevel(const ELSProtocolType ProtocolType) const
{
	switch (ProtocolType)
	{
	case ELSProtocolType::Survival:
		return SurvivalProtocolTestLevel;
	case ELSProtocolType::Carrying:
		return CarryingProtocolTestLevel;
	case ELSProtocolType::Battle:
		return BattleProtocolTestLevel;
	case ELSProtocolType::Navigation:
		return NavigationProtocolTestLevel;
	default:
		return -1;
	}
}

void ALSPlayerControllerBase::SetProtocolTestLevel(const ELSProtocolType ProtocolType, const int32 Level)
{
	int32* TargetLevel = nullptr;
	switch (ProtocolType)
	{
	case ELSProtocolType::Survival:
		TargetLevel = &SurvivalProtocolTestLevel;
		break;
	case ELSProtocolType::Carrying:
		TargetLevel = &CarryingProtocolTestLevel;
		break;
	case ELSProtocolType::Battle:
		TargetLevel = &BattleProtocolTestLevel;
		break;
	case ELSProtocolType::Navigation:
		TargetLevel = &NavigationProtocolTestLevel;
		break;
	default:
		break;
	}

	if (TargetLevel)
	{
		*TargetLevel = FMath::Max(Level, 0);
	}
}

void ALSPlayerControllerBase::RefreshProtocolTestTargets()
{
	if (PlayerHUDWidgetInstance)
	{
		PlayerHUDWidgetInstance->InitializeHUDForPawn(GetPawn());
	}

	TArray<UUserWidget*> ProtocolUIWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, ProtocolUIWidgets, ULSProtocolUIWidget::StaticClass(), false);
	for (UUserWidget* Widget : ProtocolUIWidgets)
	{
		if (ULSProtocolUIWidget* ProtocolUIWidget = Cast<ULSProtocolUIWidget>(Widget))
		{
			ProtocolUIWidget->RefreshProtocolUI();
		}
	}

	// 로비 프로토콜 표시는 칩스테이션이 담당하므로, 열려 있으면 디버그 오버라이드 변경을 즉시 반영한다.
	if (ChipStationWidgetInstance && ChipStationWidgetInstance->IsVisible())
	{
		ChipStationWidgetInstance->RefreshChipStation();
	}

	// 적재 오버라이드 변경은 퀵슬롯 해금 칸 수를 바꾸므로 등록된 바의 가시성을 즉시 다시 평가한다.
	RefreshRegisteredQuickSlotBars();
}

bool ALSPlayerControllerBase::TransferInventorySlotToLootDrop(const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex)
{
	if (!LootDropWidgetInstance || !LootDropWidgetInstance->IsVisible())
	{
		return false;
	}

	return LootDropWidgetInstance->TransferInventorySlotToFirstEmptyLootSlot(FromSlotArea, FromSlotIndex);
}

bool ALSPlayerControllerBase::TransferInventorySlotToOpenContainer(const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex, const bool bRefreshOpenContainer)
{
	if (TransferInventorySlotToLootDrop(FromSlotArea, FromSlotIndex))
	{
		return true;
	}

	if (!LobbyStorageWidgetInstance || !LobbyStorageWidgetInstance->IsVisible())
	{
		return false;
	}

	if (ULSRaidInventoryComponent* InventoryComponent = GetRaidInventoryComponent())
	{
		if (InventoryComponent->IsRaidActive())
		{
			return false;
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot quick-transfer inventory slot because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return false;
	}

	const bool bTransferred = SaveSubsystem->TransferStoredSlotToArea(FromSlotArea, FromSlotIndex, ELSInventorySlotArea::Warehouse);
	if (bTransferred && bRefreshOpenContainer)
	{
		LobbyStorageWidgetInstance->RefreshStorage();
	}
	return bTransferred;
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
			RefreshEquipmentStatsIfEquipmentTouched(FromArea, ToArea);
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
		RefreshEquipmentStatsIfEquipmentTouched(FromArea, ToArea);
		SyncRaidInventoryToClient();
	}
}

void ALSPlayerControllerBase::RefreshEquipmentStatsIfEquipmentTouched(const ELSInventorySlotArea FromArea, const ELSInventorySlotArea ToArea)
{
	if (!HasAuthority())
	{
		return;
	}

	// 장착칸(Equipment)이 관여한 변경만 스탯 재적용 대상이다. 그 외 영역끼리의 이동은 스탯과 무관.
	if (FromArea != ELSInventorySlotArea::Equipment && ToArea != ELSInventorySlotArea::Equipment)
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	if (ULSEquipmentStatComponent* EquipmentStat = ControlledPawn->FindComponentByClass<ULSEquipmentStatComponent>())
	{
		// 레이드 중 장비 교체가 회복 수단이 되지 않도록 체력 클램프만 한다(bRestoreFullHealth=false).
		EquipmentStat->RefreshEquipmentStats(false);
	}
}

bool ALSPlayerControllerBase::DropLootDropSlot(ALSLootBox* SourceLootBox, const int32 FromLootSlotIndex, const int32 ToLootSlotIndex)
{
	if (!SourceLootBox || FromLootSlotIndex == INDEX_NONE || ToLootSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot request loot slot drop because request data is invalid. From=%d To=%d"),
			FromLootSlotIndex,
			ToLootSlotIndex);
		return false;
	}

	if (HasAuthority())
	{
		const bool bChanged = DropLootDropSlotInternal(SourceLootBox, FromLootSlotIndex, ToLootSlotIndex);
		if (bChanged)
		{
			SyncRaidSessionAndLootFromServer(SourceLootBox);
		}
		return bChanged;
	}

	ServerDropLootDropSlot(SourceLootBox, FromLootSlotIndex, ToLootSlotIndex);
	return true;
}

void ALSPlayerControllerBase::ServerDropLootDropSlot_Implementation(ALSLootBox* SourceLootBox, const int32 FromLootSlotIndex, const int32 ToLootSlotIndex)
{
	const bool bChanged = DropLootDropSlotInternal(SourceLootBox, FromLootSlotIndex, ToLootSlotIndex);
	if (SourceLootBox || bChanged)
	{
		SyncRaidSessionAndLootFromServer(SourceLootBox);
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

	const TArray<FLSDropResult> LootResults = SourceLootBox ? SourceLootBox->GetLootResults() : TArray<FLSDropResult>();

	// 로비 파밍: 레이드 세션이 없으므로 세이브 기반 인벤토리/룻박스 UI만 갱신한다.
	if (IsLobbyLootMode())
	{
		ClientRefreshLobbyLoot(SourceLootBox, LootResults);
		return;
	}

	ULSRaidInventoryComponent* InventoryComponent = GetRaidInventoryComponent();
	if (!InventoryComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot sync raid inventory because RaidInventoryComponent is missing on %s."), *GetNameSafe(this));
		return;
	}

	ClientSyncRaidSessionAndLoot(
		SourceLootBox,
		InventoryComponent->GetSessionInventory(),
		InventoryComponent->GetSessionSafeInventory(),
		InventoryComponent->GetSessionEquipmentSlots(),
		LootResults);
}

void ALSPlayerControllerBase::ClientRefreshLobbyLoot_Implementation(ALSLootBox* SourceLootBox, const TArray<FLSDropResult>& LootResults)
{
	RefreshLootDropWidgetForSource(SourceLootBox, LootResults);

	// 로비 파밍 중 룻박스 상호작용 결과로 인벤토리가 바뀌었을 수 있으니 계열 패널 전체를 funnel로 다시 그린다.
	RefreshAllInventoryUI();
}

bool ALSPlayerControllerBase::IsLobbyLootMode() const
{
	const ULSRaidInventoryComponent* RaidInventory = GetRaidInventoryComponent();
	return !RaidInventory || !RaidInventory->IsRaidActive();
}

ULSSaveSubsystem* ALSPlayerControllerBase::ResolveSaveSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
}

bool ALSPlayerControllerBase::TransferLootDropSlotToSessionInternal(ALSLootBox* SourceLootBox, const int32 LootSlotIndex, FLSSessionItem& OutLootItem)
{
	if (!SourceLootBox)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot drop slot because source loot box is missing."));
		return false;
	}

	if (IsLobbyLootMode())
	{
		return SourceLootBox->TransferLootSlotToSave(LootSlotIndex, ResolveSaveSubsystem(), OutLootItem);
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

	if (IsLobbyLootMode())
	{
		return SourceLootBox->TransferLootSlotToSaveSlot(LootSlotIndex, ResolveSaveSubsystem(), ToSlotArea, ToSlotIndex, OutLootItem);
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

	if (IsLobbyLootMode())
	{
		return SourceLootBox->TransferSaveSlotToLootSlot(LootSlotIndex, ResolveSaveSubsystem(), FromSlotArea, FromSlotIndex, OutLootItem);
	}

	return SourceLootBox->TransferSessionSlotToLootSlot(LootSlotIndex, GetRaidInventoryComponent(), FromSlotArea, FromSlotIndex, OutLootItem);
}

bool ALSPlayerControllerBase::DropLootDropSlotInternal(ALSLootBox* SourceLootBox, const int32 FromLootSlotIndex, const int32 ToLootSlotIndex)
{
	if (!SourceLootBox)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop loot slot because source loot box is missing."));
		return false;
	}

	return SourceLootBox->DropLootSlot(FromLootSlotIndex, ToLootSlotIndex);
}

bool ALSPlayerControllerBase::DropSessionSlotToWorld(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector DropDirection)
{
	DropDirection.Z = 0.0f;
	DropDirection = DropDirection.GetSafeNormal();

	if (HasAuthority())
	{
		return DropSessionSlotToWorldInternal(SlotArea, SlotIndex, DroppedItemClass, DropDirection);
	}

	ServerDropSessionSlotToWorld(SlotArea, SlotIndex, DroppedItemClass, DropDirection);
	return true;
}

bool ALSPlayerControllerBase::DropOverflowInventorySlotsToWorld(TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector DropDirection)
{
	DropDirection.Z = 0.0f;
	DropDirection = DropDirection.GetSafeNormal();

	if (HasAuthority())
	{
		return DropOverflowInventorySlotsToWorldInternal(DroppedItemClass, DropDirection);
	}

	ServerDropOverflowInventorySlotsToWorld(DroppedItemClass, DropDirection);
	return true;
}

void ALSPlayerControllerBase::ServerDropSessionSlotToWorld_Implementation(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, const FVector_NetQuantizeNormal DropDirection)
{
	DropSessionSlotToWorldInternal(SlotArea, SlotIndex, DroppedItemClass, FVector(DropDirection));
}

void ALSPlayerControllerBase::ServerDropOverflowInventorySlotsToWorld_Implementation(TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, const FVector_NetQuantizeNormal DropDirection)
{
	DropOverflowInventorySlotsToWorldInternal(DroppedItemClass, FVector(DropDirection));
}

bool ALSPlayerControllerBase::DropSessionSlotToWorldInternal(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, const FVector DropDirection)
{
	if (!HasAuthority())
	{
		return false;
	}

	ULSRaidInventoryComponent* InventoryComponent = GetRaidInventoryComponent();

	// 로비에서는 아이템을 월드에 버리지 않는다 — 수동 드래그 드랍(인벤토리/창고를 창 밖으로)과 적재 프로토콜 축소 초과분 드랍이
	// 모두 이 경로를 통과하므로 여기서 한 번에 막는다(판매 외 로비 아이템 손실 금지 정책). 월드 드랍은 레이드 중에만 허용한다
	// (익스트렉션 리스크 / 신호 유실 초과분). 서버 권한 단일 관문이라 어떤 UI 경로로 들어와도 동일하게 차단된다.
	if (!InventoryComponent || !InventoryComponent->IsRaidActive())
	{
		UE_LOG(LogLS, Warning, TEXT("Refused to drop slot to world outside a raid (lobby item loss prevented). Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	const bool bUseRaidInventory = InventoryComponent && InventoryComponent->IsRaidActive() && SlotArea != ELSInventorySlotArea::Warehouse;
	ULSSaveSubsystem* SaveSubsystem = nullptr;

	if (bUseRaidInventory && SlotArea == ELSInventorySlotArea::Safe && SlotIndex >= InventoryComponent->GetMaxSafeSlotCount())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop raid safe slot to world because source slot is locked. Index=%d"), SlotIndex);
		return false;
	}

	FLSSessionItem SlotItem;
	if (bUseRaidInventory)
	{
		if (!InventoryComponent->GetSessionSlotItem(SlotArea, SlotIndex, SlotItem))
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot drop raid slot to world because source slot is invalid. Area=%d Index=%d"),
				static_cast<int32>(SlotArea), SlotIndex);
			return false;
		}
	}
	else
	{
		UGameInstance* GameInstance = GetGameInstance();
		SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
		if (SaveSubsystem && SlotArea == ELSInventorySlotArea::Safe && SlotIndex >= SaveSubsystem->GetMaxSafeStashSlotCount())
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot drop stored safe slot to world because source slot is locked. Index=%d"), SlotIndex);
			return false;
		}

		if (!SaveSubsystem || !SaveSubsystem->GetStoredSlotItem(SlotArea, SlotIndex, SlotItem))
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot drop stored slot to world because source slot is invalid. Area=%d Index=%d"),
				static_cast<int32>(SlotArea), SlotIndex);
			return false;
		}
	}

	const bool bClearedSourceSlot = bUseRaidInventory
		? InventoryComponent->ClearSessionSlot(SlotArea, SlotIndex)
		: SaveSubsystem && SaveSubsystem->ClearStoredSlot(SlotArea, SlotIndex);
	if (!bClearedSourceSlot)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop slot to world because source slot could not be cleared. Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	if (!SpawnDroppedItemToWorld(SlotItem, DroppedItemClass, DropDirection))
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to spawn dropped item for slot. Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		FLSSessionItem IgnoredPreviousItem;
		if (bUseRaidInventory)
		{
			InventoryComponent->ReplaceSessionSlotItem(SlotArea, SlotIndex, SlotItem, IgnoredPreviousItem);
		}
		else if (SaveSubsystem)
		{
			SaveSubsystem->ReplaceStoredSlotItem(SlotArea, SlotIndex, SlotItem, IgnoredPreviousItem);
		}
		return false;
	}

	if (bUseRaidInventory)
	{
		// 장착칸에서 곧바로 월드 드랍하면 즉시 해제이므로 장비 스탯을 재적용한다.
		RefreshEquipmentStatsIfEquipmentTouched(SlotArea, SlotArea);
		ClientSyncRaidSessionAndLoot(nullptr, InventoryComponent->GetSessionInventory(), InventoryComponent->GetSessionSafeInventory(), InventoryComponent->GetSessionEquipmentSlots(), TArray<FLSDropResult>());
	}
	return true;
}

bool ALSPlayerControllerBase::DropOverflowInventorySlotsToWorldInternal(TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, const FVector DropDirection)
{
	if (!HasAuthority())
	{
		return false;
	}

	ULSRaidInventoryComponent* InventoryComponent = GetRaidInventoryComponent();
	const bool bUseRaidInventory = InventoryComponent && InventoryComponent->IsRaidActive();
	const int32 MaxInventorySlotCount = bUseRaidInventory
		? InventoryComponent->GetMaxInventorySlotCount()
		: [this]()
		{
			UGameInstance* GameInstance = GetGameInstance();
			const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
			return SaveSubsystem ? SaveSubsystem->GetMaxInventorySlotCount() : 0;
		}();

	int32 DroppedCount = 0;
	if (bUseRaidInventory)
	{
		const TArray<FLSSessionItem>& InventoryItems = InventoryComponent->GetSessionInventory();
		for (int32 SlotIndex = MaxInventorySlotCount; SlotIndex < InventoryItems.Num(); ++SlotIndex)
		{
			if (LSInventorySlotUtils::IsFilled(InventoryItems[SlotIndex]) &&
				DropSessionSlotToWorldInternal(ELSInventorySlotArea::Inventory, SlotIndex, DroppedItemClass, DropDirection))
			{
				++DroppedCount;
			}
		}
	}
	else
	{
		UGameInstance* GameInstance = GetGameInstance();
		ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
		const TArray<FLSSessionItem>* InventoryItems = SaveSubsystem ? &SaveSubsystem->GetInventory() : nullptr;
		for (int32 SlotIndex = MaxInventorySlotCount; InventoryItems && SlotIndex < InventoryItems->Num(); ++SlotIndex)
		{
			if (LSInventorySlotUtils::IsFilled((*InventoryItems)[SlotIndex]) &&
				DropSessionSlotToWorldInternal(ELSInventorySlotArea::Inventory, SlotIndex, DroppedItemClass, DropDirection))
			{
				++DroppedCount;
			}
		}
	}

	if (DroppedCount > 0)
	{
		UE_LOG(LogLS, Log, TEXT("Dropped %d overflow inventory slots to world on %s."), DroppedCount, *GetNameSafe(this));
	}
	return DroppedCount > 0;
}

bool ALSPlayerControllerBase::SpawnDroppedItemToWorld(const FLSSessionItem& SlotItem, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, const FVector DropDirection)
{
	if (!LSInventorySlotUtils::IsFilled(SlotItem))
	{
		return false;
	}

	FTransform SpawnTransform;
	if (!ResolveServerDroppedItemTransform(SpawnTransform, DropDirection))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop slot to world because server drop transform is invalid."));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop slot to world because World is missing."));
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
		return false;
	}

	DroppedItem->InitializeDroppedItem(SlotItem);
	DroppedItem->FinishSpawning(SpawnTransform);
	return true;
}

bool ALSPlayerControllerBase::ResolveDropDirectionFromSlatePosition(const FVector2D SlatePosition, FVector& OutDropDirection) const
{
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return false;
	}

	FVector2D PixelPosition = FVector2D::ZeroVector;
	FVector2D ViewportPosition = FVector2D::ZeroVector;
	USlateBlueprintLibrary::AbsoluteToViewport(this, SlatePosition, PixelPosition, ViewportPosition);

	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;
	if (!DeprojectScreenPositionToWorld(PixelPosition.X, PixelPosition.Y, WorldOrigin, WorldDirection))
	{
		return false;
	}

	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}

	float CollisionRadius = 0.0f;
	float CollisionHalfHeight = 0.0f;
	ControlledPawn->GetSimpleCollisionCylinder(CollisionRadius, CollisionHalfHeight);

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const float TargetPlaneZ = PawnLocation.Z - CollisionHalfHeight;
	const float RayDistance = (TargetPlaneZ - WorldOrigin.Z) / WorldDirection.Z;
	if (RayDistance < 0.0f)
	{
		return false;
	}

	const FVector MouseWorldPoint = WorldOrigin + (WorldDirection * RayDistance);
	OutDropDirection = MouseWorldPoint - PawnLocation;
	OutDropDirection.Z = 0.0f;
	return !OutDropDirection.IsNearlyZero();
}

bool ALSPlayerControllerBase::ResolveServerDroppedItemTransform(FTransform& OutDropTransform, FVector DropDirection) const
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

	DropDirection.Z = 0.0f;
	DropDirection = DropDirection.GetSafeNormal();
	if (DropDirection.IsNearlyZero())
	{
		DropDirection = ControlledPawn->GetActorForwardVector().GetSafeNormal2D();
	}

	const FVector FootLocation = PawnLocation - FVector(0.0f, 0.0f, CollisionHalfHeight);
	const FVector TargetFootLocation = FootLocation + (DropDirection * DroppedItemForwardDistance);
	const FVector TraceStart = TargetFootLocation + FVector(0.0f, 0.0f, DroppedItemGroundTraceDistance);
	const FVector TraceEnd = TargetFootLocation - FVector(0.0f, 0.0f, DroppedItemGroundTraceDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LSDropInventoryItemToGround), false, ControlledPawn);
	const bool bHitGround = World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	const FVector GroundLocation = bHitGround ? HitResult.ImpactPoint : TargetFootLocation;
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
