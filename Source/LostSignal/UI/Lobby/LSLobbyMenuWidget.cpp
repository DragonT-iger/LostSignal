#include "UI/Lobby/LSLobbyMenuWidget.h"

#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Core/LSLobbyGameMode.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "LostSignal.h"
#include "UI/Lobby/LSLobbyQuestWidget.h"
#include "UI/Lobby/LSLobbyTabWidget.h"

void ULSLobbyMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// TAB 키 입력을 받기 위해 포커스 가능하게 둔다. 실제 포커스는 GameMode가 SetWidgetToFocus로 준다.
	SetIsFocusable(true);

	// 플레이 탭만 클릭을 구독한다. 나머지 탭은 바인딩 검증만 한다.
	if (PlayTab)
	{
		PlayTab->OnClicked.AddDynamic(this, &ULSLobbyMenuWidget::HandlePlayTabClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("PlayTab is not bound on %s."), *GetNameSafe(this));
	}

	// 개인정비 탭은 WidgetSwitcher를 개인정비(칩 스테이션) 페이지로 전환한다.
	if (EquipTab)
	{
		EquipTab->OnClicked.AddDynamic(this, &ULSLobbyMenuWidget::HandleEquipTabClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("EquipTab is not bound on %s."), *GetNameSafe(this));
	}
	if (!QuestTab)
	{
		UE_LOG(LogLS, Warning, TEXT("QuestTab is not bound on %s."), *GetNameSafe(this));
	}
	if (!CharacterTab)
	{
		UE_LOG(LogLS, Warning, TEXT("CharacterTab is not bound on %s."), *GetNameSafe(this));
	}

	if (MissionStartButton)
	{
		MissionStartButton->OnClicked.AddDynamic(this, &ULSLobbyMenuWidget::HandleMissionStartClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("MissionStartButton is not bound on %s."), *GetNameSafe(this));
	}

	if (!TabSwitcher)
	{
		UE_LOG(LogLS, Warning, TEXT("TabSwitcher is not bound on %s."), *GetNameSafe(this));
	}
	if (!LobbyQuest)
	{
		UE_LOG(LogLS, Warning, TEXT("LobbyQuest is not bound on %s."), *GetNameSafe(this));
	}
	if (!LevelText)
	{
		UE_LOG(LogLS, Warning, TEXT("LevelText is not bound on %s."), *GetNameSafe(this));
	}
	if (!LevelProgressBar)
	{
		UE_LOG(LogLS, Warning, TEXT("LevelProgressBar is not bound on %s."), *GetNameSafe(this));
	}
	if (!PlayerNameText)
	{
		UE_LOG(LogLS, Warning, TEXT("PlayerNameText is not bound on %s."), *GetNameSafe(this));
	}
	if (!CharacterPortraitButton)
	{
		UE_LOG(LogLS, Warning, TEXT("CharacterPortraitButton is not bound on %s."), *GetNameSafe(this));
	}
	if (!BackButton)
	{
		UE_LOG(LogLS, Warning, TEXT("BackButton is not bound on %s."), *GetNameSafe(this));
	}
	// 인벤토리 버튼: TAB 키와 동일하게 인벤토리(창고+인벤토리) 페이지를 토글한다.
	if (InventoryButton)
	{
		InventoryButton->OnClicked.AddDynamic(this, &ULSLobbyMenuWidget::HandleInventoryButtonClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("InventoryButton is not bound on %s."), *GetNameSafe(this));
	}

	// 로비는 플레이 탭에서 시작한다.
	ShowTab(ELSLobbyTab::Play);
}

void ULSLobbyMenuWidget::NativeDestruct()
{
	if (PlayTab)
	{
		PlayTab->OnClicked.RemoveDynamic(this, &ULSLobbyMenuWidget::HandlePlayTabClicked);
	}
	if (EquipTab)
	{
		EquipTab->OnClicked.RemoveDynamic(this, &ULSLobbyMenuWidget::HandleEquipTabClicked);
	}
	if (InventoryButton)
	{
		InventoryButton->OnClicked.RemoveDynamic(this, &ULSLobbyMenuWidget::HandleInventoryButtonClicked);
	}
	if (MissionStartButton)
	{
		MissionStartButton->OnClicked.RemoveDynamic(this, &ULSLobbyMenuWidget::HandleMissionStartClicked);
	}

	Super::NativeDestruct();
}

void ULSLobbyMenuWidget::SetLevelText(const FText& NewLevelText) const
{
	if (!LevelText)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set level text because LevelText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	LevelText->SetText(NewLevelText);
}

void ULSLobbyMenuWidget::SetLevelProgress(const float Progress) const
{
	if (!LevelProgressBar)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set level progress because LevelProgressBar is not bound on %s."), *GetNameSafe(this));
		return;
	}

	LevelProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
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

void ULSLobbyMenuWidget::HandlePlayTabClicked()
{
	ShowTab(ELSLobbyTab::Play);
}

void ULSLobbyMenuWidget::HandleEquipTabClicked()
{
	ShowTab(ELSLobbyTab::Equip);
}

void ULSLobbyMenuWidget::HandleInventoryButtonClicked()
{
	ToggleInventoryTab();
}

FReply ULSLobbyMenuWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// 포커스가 버튼으로 넘어가도 터널링 단계라 루트가 먼저 받는다. TAB은 여기서 소비해 포커스 이동을 막는다.
	if (InKeyEvent.GetKey() == EKeys::Tab)
	{
		ToggleInventoryTab();
		return FReply::Handled();
	}

	// 플레이 페이지가 아니면 ESC로 플레이로 돌아온다. 이미 플레이면 통과시킨다.
	if (InKeyEvent.GetKey() == EKeys::Escape && TabSwitcher
		&& TabSwitcher->GetActiveWidgetIndex() != static_cast<int32>(ELSLobbyTab::Play))
	{
		ShowTab(ELSLobbyTab::Play);
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void ULSLobbyMenuWidget::ToggleInventoryTab() const
{
	if (!TabSwitcher)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot toggle inventory because TabSwitcher is not bound on %s."), *GetNameSafe(this));
		return;
	}

	const int32 InventoryIndex = static_cast<int32>(ELSLobbyTab::Inventory);
	const bool bInventoryActive = TabSwitcher->GetActiveWidgetIndex() == InventoryIndex;
	ShowTab(bInventoryActive ? ELSLobbyTab::Play : ELSLobbyTab::Inventory);
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

void ULSLobbyMenuWidget::ShowTab(const ELSLobbyTab Tab) const
{
	if (!TabSwitcher)
	{
		return;
	}

	TabSwitcher->SetActiveWidgetIndex(static_cast<int32>(Tab));
}
