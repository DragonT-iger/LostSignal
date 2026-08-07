// 로비 루트 위젯의 라이프사이클·바인딩 검증·데이터 setter·임무 시작.
// 패널 전환은 LSLobbyMenuWidget_Panels.cpp, 키 입력/포커스/오버레이는 LSLobbyMenuWidget_Input.cpp에 있다.
#include "UI/Lobby/LSLobbyMenuWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Blueprint/WidgetTree.h"
#include "Core/LSLobbyGameMode.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/CharacterNode/LSCharacterPanelWidget.h"
#include "UI/ChipSystem/LSChipStationWidget.h"
#include "UI/Common/LSConfirmDialogWidget.h"
#include "UI/Inventory/LSInventoryWidget.h"
#include "UI/Lobby/LSLobbyQuestWidget.h"
#include "UI/Lobby/LSLobbyTabWidget.h"
#include "UI/Lobby/Store/LSStoreWidget.h"
#include "UI/Storage/LSLobbyStorageWidget.h"

// 세 cpp 모두 같은 네임스페이스를 쓴다. 다르게 두면 LOCTEXT 키가 갈려 번역이 끊긴다.
#define LOCTEXT_NAMESPACE "LSLobbyMenu"

void ULSLobbyMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// TAB 키 입력을 받기 위해 포커스 가능하게 둔다. 실제 포커스는 GameMode가 SetWidgetToFocus로 준다.
	SetIsFocusable(true);

	BindLobbyTabs();
	BindActionButtons();
	ValidateDisplayBindings();
	CreatePanelWidgets();
	ValidatePanelBindings();

	if (ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		SaveSubsystem->OnGoldChanged.RemoveAll(this);
		SaveSubsystem->OnGoldChanged.AddUObject(this, &ULSLobbyMenuWidget::HandleGoldChanged);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] SaveSubsystem is missing on %s. GoldText cannot be refreshed."), *GetNameSafe(this));
	}
	RefreshGoldText();
	InitializeLobbyView();
	ShowSessionNoticeOnOpen();
}

void ULSLobbyMenuWidget::BindLobbyTabs()
{
	BindLobbyTab(LobbyTab, TEXT("LobbyTab"));
	BindLobbyTab(ChipTab, TEXT("ChipTab"));
	BindLobbyTab(SupplyTab, TEXT("SupplyTab"));
	BindLobbyTab(CharacterTab, TEXT("CharacterTab"));
	BindLobbyTab(BagTab, TEXT("BagTab"));
	BindLobbyTab(QuestTab, TEXT("QuestTab"));
	BindLobbyTab(MapTab, TEXT("MapTab"));
	BindLobbyTab(SettingsTab, TEXT("SettingsTab"));
}

void ULSLobbyMenuWidget::BindActionButtons()
{
	if (MissionStartButton)
	{
		MissionStartButton->OnClicked.AddDynamic(this, &ULSLobbyMenuWidget::HandleMissionStartClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("MissionStartButton is not bound on %s."), *GetNameSafe(this));
	}

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &ULSLobbyMenuWidget::HandleJoinClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("JoinButton is not bound on %s."), *GetNameSafe(this));
	}

	if (!JoinAddressTextBox)
	{
		UE_LOG(LogLS, Warning, TEXT("JoinAddressTextBox is not bound on %s."), *GetNameSafe(this));
	}
}

void ULSLobbyMenuWidget::CreatePanelWidgets()
{
	if (!TabSwitcher)
	{
		return;
	}

	const bool bHasRuntimePanel =
		ChipStationPanelInstance || StorePanelInstance || CharacterPanelInstance || BagPanelInstance || QuestPanelInstance;
	if (!bHasRuntimePanel && TabSwitcher->GetChildrenCount() > 0)
	{
		UE_LOG(LogLS, Warning,
			TEXT("[Lobby] TabSwitcher must be empty in WBP_LobbyMenu, but it already has %d children on %s."),
			TabSwitcher->GetChildrenCount(), *GetNameSafe(this));
	}

	if (!ChipStationPanelInstance)
	{
		ChipStationPanelInstance = Cast<ULSChipStationWidget>(CreatePanelWidget(ChipStationPanelClass.Get(), TEXT("ChipStationPanelClass")));
	}
	if (!StorePanelInstance)
	{
		StorePanelInstance = Cast<ULSStoreWidget>(CreatePanelWidget(StorePanelClass.Get(), TEXT("StorePanelClass")));
	}
	if (!CharacterPanelInstance)
	{
		CharacterPanelInstance = Cast<ULSCharacterPanelWidget>(CreatePanelWidget(CharacterPanelClass.Get(), TEXT("CharacterPanelClass")));
	}
	if (!BagPanelInstance)
	{
		CreateBagPanelWidget();
	}
	if (!QuestPanelInstance)
	{
		CreateQuestPanelWidget();
	}
}

void ULSLobbyMenuWidget::CreateBagPanelWidget()
{
	LobbyInventoryInstance = Cast<ULSInventoryWidget>(
		CreateRuntimeWidget(LobbyInventoryClass.Get(), TEXT("LobbyInventoryClass")));
	LobbyStorageInstance = Cast<ULSLobbyStorageWidget>(
		CreateRuntimeWidget(LobbyStorageClass.Get(), TEXT("LobbyStorageClass")));
	if (!LobbyInventoryInstance || !LobbyStorageInstance || !WidgetTree)
	{
		LobbyInventoryInstance = nullptr;
		LobbyStorageInstance = nullptr;
		return;
	}

	BagPanelInstance = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RuntimeBagPanel"));
	if (!BagPanelInstance)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Failed to create runtime bag panel on %s."), *GetNameSafe(this));
		LobbyInventoryInstance = nullptr;
		LobbyStorageInstance = nullptr;
		return;
	}

	UOverlaySlot* InventorySlot = BagPanelInstance->AddChildToOverlay(LobbyInventoryInstance);
	UOverlaySlot* StorageSlot = BagPanelInstance->AddChildToOverlay(LobbyStorageInstance);
	if (InventorySlot)
	{
		InventorySlot->SetHorizontalAlignment(HAlign_Fill);
		InventorySlot->SetVerticalAlignment(VAlign_Fill);
	}
	if (StorageSlot)
	{
		StorageSlot->SetHorizontalAlignment(HAlign_Fill);
		StorageSlot->SetVerticalAlignment(VAlign_Fill);
	}
	TabSwitcher->AddChild(BagPanelInstance);
}

void ULSLobbyMenuWidget::CreateQuestPanelWidget()
{
	if (!QuestPanelHost)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] QuestPanelHost is not bound on %s."), *GetNameSafe(this));
		return;
	}

	QuestPanelInstance = Cast<ULSLobbyQuestWidget>(
		CreateRuntimeWidget(QuestPanelClass.Get(), TEXT("QuestPanelClass")));
	if (!QuestPanelInstance)
	{
		return;
	}

	if (QuestPanelHost->GetContent())
	{
		UE_LOG(LogLS, Warning,
			TEXT("[Lobby] QuestPanelHost must be empty in WBP_LobbyMenu. Replacing its design-time child on %s."),
			*GetNameSafe(this));
		QuestPanelHost->SetContent(nullptr);
	}
	QuestPanelHost->SetContent(QuestPanelInstance);
}

UUserWidget* ULSLobbyMenuWidget::CreateRuntimeWidget(UClass* WidgetClass, const TCHAR* WidgetClassName)
{
	if (!WidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] %s is not set on %s."), WidgetClassName, *GetNameSafe(this));
		return nullptr;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	UUserWidget* Widget = OwningPlayer
		? CreateWidget<UUserWidget>(OwningPlayer, WidgetClass)
		: CreateWidget<UUserWidget>(this, WidgetClass);
	if (!Widget)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Failed to create %s on %s."), WidgetClassName, *GetNameSafe(this));
	}
	return Widget;
}

UUserWidget* ULSLobbyMenuWidget::CreatePanelWidget(UClass* PanelClass, const TCHAR* PanelClassName)
{
	UUserWidget* Panel = CreateRuntimeWidget(PanelClass, PanelClassName);
	if (Panel)
	{
		TabSwitcher->AddChild(Panel);
	}
	return Panel;
}

void ULSLobbyMenuWidget::ValidateDisplayBindings()
{
	if (!CharacterPortraitButton)
	{
		UE_LOG(LogLS, Warning, TEXT("CharacterPortraitButton is not bound on %s."), *GetNameSafe(this));
	}
	if (!PlayerNameText)
	{
		UE_LOG(LogLS, Warning, TEXT("PlayerNameText is not bound on %s."), *GetNameSafe(this));
	}
	if (!GoldText)
	{
		UE_LOG(LogLS, Warning, TEXT("GoldText is not bound on %s."), *GetNameSafe(this));
	}
}

void ULSLobbyMenuWidget::InitializeLobbyView()
{
	// 로비는 패널이 하나도 열리지 않은 상태에서 시작한다. WBP 디자이너 설정과 무관하게 C++이 강제한다.
	// ActivePanel 초기값이 None이라 ShowPanel(None)은 조기 반환하므로 초기 상태를 직접 적용한다.
	if (TabSwitcher)
	{
		TabSwitcher->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (QuestPanelHost)
	{
		QuestPanelHost->SetVisibility(ESlateVisibility::Collapsed);
	}
	UpdateSelectedTab(ELSLobbyPanel::None);
}

void ULSLobbyMenuWidget::NativeDestruct()
{
	UnbindLobbyTab(LobbyTab);
	UnbindLobbyTab(ChipTab);
	UnbindLobbyTab(SupplyTab);
	UnbindLobbyTab(CharacterTab);
	UnbindLobbyTab(BagTab);
	UnbindLobbyTab(QuestTab);
	UnbindLobbyTab(MapTab);
	UnbindLobbyTab(SettingsTab);

	if (MissionStartButton)
	{
		MissionStartButton->OnClicked.RemoveDynamic(this, &ULSLobbyMenuWidget::HandleMissionStartClicked);
	}

	if (JoinButton)
	{
		JoinButton->OnClicked.RemoveDynamic(this, &ULSLobbyMenuWidget::HandleJoinClicked);
	}

	if (ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		SaveSubsystem->OnGoldChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void ULSLobbyMenuWidget::BindLobbyTab(ULSLobbyTabWidget* Tab, const TCHAR* TabName)
{
	if (!Tab)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is not bound on %s."), TabName, *GetNameSafe(this));
		return;
	}

	Tab->OnClicked.AddDynamic(this, &ULSLobbyMenuWidget::HandleLobbyTabClicked);
}

void ULSLobbyMenuWidget::UnbindLobbyTab(ULSLobbyTabWidget* Tab) const
{
	if (!Tab)
	{
		return;
	}

	Tab->OnClicked.RemoveDynamic(this, &ULSLobbyMenuWidget::HandleLobbyTabClicked);
}

void ULSLobbyMenuWidget::ValidatePanelBindings() const
{
	if (!TabSwitcher)
	{
		UE_LOG(LogLS, Warning, TEXT("TabSwitcher is not bound on %s."), *GetNameSafe(this));
		return;
	}

	const int32 BoundPageCount = ValidatePanelPages();
	if (TabSwitcher->GetChildrenCount() != BoundPageCount)
	{
		UE_LOG(LogLS, Warning,
			TEXT("[Lobby] TabSwitcher has %d children but %d panel pages are bound on %s. Check for orphan pages."),
			TabSwitcher->GetChildrenCount(), BoundPageCount, *GetNameSafe(this));
	}
	if (!ConfirmDialogClass)
	{
		UE_LOG(LogLS, Warning,
			TEXT("[Lobby] ConfirmDialogClass is not set on %s. Notices and store dialogs will not show. Check WBP_LobbyMenu."),
			*GetNameSafe(this));
	}
	if (QuestPanelHost && QuestPanelInstance && QuestPanelHost->GetContent() != QuestPanelInstance)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] QuestPanelInstance is not attached to QuestPanelHost on %s."), *GetNameSafe(this));
	}
}

int32 ULSLobbyMenuWidget::ValidatePanelPages() const
{
	const TObjectPtr<UWidget> PanelPages[] = {
		ChipStationPanelInstance, StorePanelInstance, CharacterPanelInstance, BagPanelInstance };
	const TCHAR* PanelPageNames[] = {
		TEXT("ChipStationPanel"), TEXT("StorePanel"), TEXT("CharacterPanel"), TEXT("BagPanel") };

	int32 BoundPageCount = 0;
	for (int32 PageIndex = 0; PageIndex < UE_ARRAY_COUNT(PanelPages); ++PageIndex)
	{
		UWidget* Page = PanelPages[PageIndex];
		if (!Page)
		{
			UE_LOG(LogLS, Warning, TEXT("%s is not bound on %s."), PanelPageNames[PageIndex], *GetNameSafe(this));
			continue;
		}

		++BoundPageCount;
		// SetActiveWidget은 직속 자식만 받으므로 아트가 페이지를 한 겹 감싸면 전환이 조용히 실패한다.
		if (TabSwitcher->GetChildIndex(Page) == INDEX_NONE)
		{
			UE_LOG(LogLS, Warning,
				TEXT("[Lobby] %s is not a direct child of TabSwitcher on %s. Panel switching will silently fail."),
				PanelPageNames[PageIndex], *GetNameSafe(this));
		}
	}

	return BoundPageCount;
}

void ULSLobbyMenuWidget::SetPlayerName(const FText& NewPlayerName) const
{
	if (!PlayerNameText)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set player name because PlayerNameText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	PlayerNameText->SetText(NewPlayerName);
}

ULSSaveSubsystem* ULSLobbyMenuWidget::GetSaveSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
}

void ULSLobbyMenuWidget::RefreshGoldText() const
{
	if (!GoldText)
	{
		return;
	}

	if (const ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		GoldText->SetText(FText::AsNumber(SaveSubsystem->GetGold()));
	}
}

void ULSLobbyMenuWidget::HandleGoldChanged()
{
	RefreshGoldText();
}

void ULSLobbyMenuWidget::HandleMissionStartClicked()
{
	// 싱글 전용 경로: 로컬이 곧 서버다. 멀티 전환 시 PlayerController Server RPC로 교체한다.
	ALSLobbyGameMode* LobbyGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALSLobbyGameMode>() : nullptr;
	if (!LobbyGameMode)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Mission start clicked but LobbyGameMode is missing."));
		return;
	}

	UE_LOG(LogLS, Log, TEXT("[Lobby] Mission start - requesting raid start."));
	LobbyGameMode->StartRaid();
}

#undef LOCTEXT_NAMESPACE
