// 로비 루트 위젯의 패널 전환·리프레시 디스패치·탭 클릭 분기.
// 라이프사이클은 LSLobbyMenuWidget.cpp, 키 입력/포커스/오버레이는 LSLobbyMenuWidget_Input.cpp에 있다.
#include "UI/Lobby/LSLobbyMenuWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/WidgetSwitcher.h"
#include "LostSignal.h"
#include "UI/ChipSystem/LSChipStationWidget.h"
#include "UI/Inventory/LSInventoryWidget.h"
#include "UI/Lobby/LSLobbyQuestWidget.h"
#include "UI/Lobby/LSLobbyTabWidget.h"
#include "UI/Lobby/Store/LSStoreWidget.h"
#include "UI/Skill/LSSkillLoadoutWidget.h"
#include "UI/Storage/LSLobbyStorageWidget.h"

// 세 cpp 모두 같은 네임스페이스를 쓴다. 다르게 두면 LOCTEXT 키가 갈려 번역이 끊긴다.
#define LOCTEXT_NAMESPACE "LSLobbyMenu"

void ULSLobbyMenuWidget::ShowPanel(const ELSLobbyPanel Panel)
{
	// 같은 탭 재클릭은 무시한다. 이미 열려 있는 패널이라 stale 데이터가 생길 여지가 없고
	// (다른 패널을 거쳐 돌아오면 반드시 RefreshPanelOnOpen을 다시 탄다), 스크롤/필터 상태도 보존된다.
	if (Panel == ActivePanel)
	{
		return;
	}

	if (!TabSwitcher)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot show panel because TabSwitcher is not bound on %s."), *GetNameSafe(this));
		return;
	}
	if (Panel == ELSLobbyPanel::Quest && (!QuestPanelHost || !QuestPanelInstance))
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot show quest because QuestPanelHost or QuestPanelInstance is missing on %s."),
			*GetNameSafe(this));
		return;
	}

	// 떠나는 패널이 띄워 둔 모달은 스위처 전환과 무관한 별도 레이어에 있어 그냥 두면 화면에 남는다.
	ClosePanelModal(ActivePanel);

	if (!ApplyPanelVisibility(Panel))
	{
		return;
	}

	ActivePanel = Panel;
	UpdateSelectedTab(Panel);
	UpdateBackground(Panel);

	// 리프레시는 ActivePanel을 갱신한 뒤에 한다(리프레시 중 현재 패널을 조회하는 경로 대비).
	RefreshPanelOnOpen(Panel);
}

bool ULSLobbyMenuWidget::ApplyPanelVisibility(const ELSLobbyPanel Panel)
{
	if (Panel == ELSLobbyPanel::None)
	{
		TabSwitcher->SetVisibility(ESlateVisibility::Collapsed);
		if (QuestPanelHost)
		{
			QuestPanelHost->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else if (Panel == ELSLobbyPanel::Quest)
	{
		TabSwitcher->SetVisibility(ESlateVisibility::Collapsed);
		QuestPanelHost->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		UWidget* Page = ResolvePanelPage(Panel);
		if (!Page)
		{
			UE_LOG(LogLS, Warning, TEXT("[Lobby] Panel page is not bound for panel %d on %s."),
				static_cast<int32>(Panel), *GetNameSafe(this));
			return false;
		}

		// 인덱스가 아니라 포인터로 전환한다. 아트가 스위처 안 페이지 순서를 바꿔도 깨지지 않는다.
		TabSwitcher->SetActiveWidget(Page);
		TabSwitcher->SetVisibility(ESlateVisibility::Visible);
		if (QuestPanelHost)
		{
			QuestPanelHost->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	return true;
}

UWidget* ULSLobbyMenuWidget::ResolvePanelPage(const ELSLobbyPanel Panel) const
{
	switch (Panel)
	{
	case ELSLobbyPanel::ChipStation:  return ChipStationPanelInstance;
	case ELSLobbyPanel::Supply:       return StorePanelInstance;
	case ELSLobbyPanel::SkillLoadout: return SkillLoadoutPanelInstance;
	case ELSLobbyPanel::Bag:          return BagPanelInstance;
	case ELSLobbyPanel::Quest:
	case ELSLobbyPanel::None:
	default:
		return nullptr;
	}
}

void ULSLobbyMenuWidget::RefreshPanelOnOpen(const ELSLobbyPanel Panel)
{
	switch (Panel)
	{
	// 열 때마다 리빌드해 가방 조작 뒤 stale 저장 인덱스를 쓰지 않게 한다. 이 방어선은 제거하면 안 된다.
	case ELSLobbyPanel::ChipStation:
		if (ChipStationPanelInstance)
		{
			ChipStationPanelInstance->RefreshChipStation();
		}
		break;

	// 다른 패널에서 아이템이 바뀌었을 수 있으므로 최신 세이브 기준으로 리빌드한다.
	case ELSLobbyPanel::Bag:
		if (LobbyInventoryInstance)
		{
			LobbyInventoryInstance->RebuildInventorySlots();
			LobbyInventoryInstance->RebuildConfirmedStorageSlots();
			LobbyInventoryInstance->RebuildEquipmentSlots();
		}
		if (LobbyStorageInstance)
		{
			LobbyStorageInstance->RefreshStorage();
		}
		break;

	// 보급소는 열 때마다 초기 상태(기능 선택)로 되돌린다. 대화 도중 닫았다 열어도 이어지지 않게.
	case ELSLobbyPanel::Supply:
		if (StorePanelInstance)
		{
			StorePanelInstance->SetConfirmDialogClass(ConfirmDialogClass);
			StorePanelInstance->ResetStore();
		}
		break;

	case ELSLobbyPanel::SkillLoadout:
		if (SkillLoadoutPanelInstance)
		{
			SkillLoadoutPanelInstance->RefreshSkillLoadout();
		}
		break;

	// 퀘스트는 아직 데이터 소스 연동이 없어 리프레시할 것이 없다(setter만 열려 있음).
	case ELSLobbyPanel::Quest:
	case ELSLobbyPanel::None:
	default:
		break;
	}
}

void ULSLobbyMenuWidget::ClosePanelModal(const ELSLobbyPanel Panel) const
{
	// 새 패널이 자체 모달을 띄우면 여기에 합류시킨다. 안 하면 패널을 바꿀 때 다이얼로그만 화면에 남는다.
	switch (Panel)
	{
	case ELSLobbyPanel::ChipStation:
		if (ChipStationPanelInstance)
		{
			ChipStationPanelInstance->CloseActiveConfirmDialog();
		}
		break;

	case ELSLobbyPanel::Supply:
		if (StorePanelInstance)
		{
			StorePanelInstance->CloseActiveConfirmDialog();
		}
		break;

	case ELSLobbyPanel::Bag:
		if (LobbyInventoryInstance)
		{
			LobbyInventoryInstance->CloseActiveNotificationDialog();
		}
		break;

	default:
		break;
	}
}

void ULSLobbyMenuWidget::UpdateSelectedTab(const ELSLobbyPanel Panel, const bool bSettingsSelected) const
{
	if (LobbyTab)
	{
		LobbyTab->SetSelected(!bSettingsSelected && Panel == ELSLobbyPanel::None);
	}
	if (ChipTab)
	{
		ChipTab->SetSelected(!bSettingsSelected && Panel == ELSLobbyPanel::ChipStation);
	}
	if (SupplyTab)
	{
		SupplyTab->SetSelected(!bSettingsSelected && Panel == ELSLobbyPanel::Supply);
	}
	if (CharacterTab)
	{
		CharacterTab->SetSelected(!bSettingsSelected && Panel == ELSLobbyPanel::SkillLoadout);
	}
	if (BagTab)
	{
		BagTab->SetSelected(!bSettingsSelected && Panel == ELSLobbyPanel::Bag);
	}
	if (QuestTab)
	{
		QuestTab->SetSelected(!bSettingsSelected && Panel == ELSLobbyPanel::Quest);
	}
	// 지도는 패널이 없어 항상 비선택이다. 매핑이 빠진 게 아니라 의도다.
	if (MapTab)
	{
		MapTab->SetSelected(false);
	}
	if (SettingsTab)
	{
		SettingsTab->SetSelected(bSettingsSelected);
	}
	UpdateMissionStartVisibility(!bSettingsSelected && Panel == ELSLobbyPanel::None);
}

void ULSLobbyMenuWidget::UpdateMissionStartVisibility(const bool bVisible) const
{
	if (MissionStartButton)
	{
		MissionStartButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void ULSLobbyMenuWidget::UpdateBackground(const ELSLobbyPanel Panel) const
{
	if (!BackgroundImage)
	{
		return;
	}

	// 패널이 열렸고 전용 배경 리소스가 지정돼 있으면 교체하고, 로비(패널 없음)에서는 기본 배경으로 복원한다.
	const bool bUsePanelBackground = Panel != ELSLobbyPanel::None
		&& LoadoutPreparationBackgroundBrush.GetResourceObject() != nullptr;
	BackgroundImage->SetBrush(bUsePanelBackground ? LoadoutPreparationBackgroundBrush : DefaultBackgroundBrush);
}

void ULSLobbyMenuWidget::HandleLobbyTabClicked(ULSLobbyTabWidget* ClickedTab)
{
	if (!ClickedTab)
	{
		return;
	}

	CloseSettingsForTabSwitch(ClickedTab);

	// 지도는 안내창 토글, 패널이 있는 탭은 안내창을 닫고 전환을 이어간다.
	const bool bClosedNotice = CloseNotImplementedNotice();
	if (bClosedNotice && ClickedTab == MapTab)
	{
		return;
	}

	if (ClickedTab == LobbyTab)
	{
		ShowPanel(ELSLobbyPanel::None);
	}
	else if (ClickedTab == ChipTab)
	{
		ShowPanel(ELSLobbyPanel::ChipStation);
	}
	else if (ClickedTab == SupplyTab)
	{
		ShowPanel(ELSLobbyPanel::Supply);
	}
	else if (ClickedTab == CharacterTab)
	{
		ShowPanel(ELSLobbyPanel::SkillLoadout);
	}
	else if (ClickedTab == BagTab)
	{
		ShowPanel(ELSLobbyPanel::Bag);
	}
	else if (ClickedTab == QuestTab)
	{
		ShowPanel(ELSLobbyPanel::Quest);
	}
	else if (ClickedTab == MapTab)
	{
		// 목적지 선택 화면은 미구현. 레벨은 ULSSessionSettings.FarmingLevel 하나로 고정이다.
		ShowPanel(ELSLobbyPanel::None);
		UpdateMissionStartVisibility(false);
		ShowNotImplementedNotice();
	}
	else if (ClickedTab == SettingsTab)
	{
		ShowSettingsWidget();
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Unmapped lobby tab clicked: %s."), *GetNameSafe(ClickedTab));
	}
}

#undef LOCTEXT_NAMESPACE
