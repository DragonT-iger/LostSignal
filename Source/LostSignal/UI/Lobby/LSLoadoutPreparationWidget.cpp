#include "UI/Lobby/LSLoadoutPreparationWidget.h"

#include "Components/WidgetSwitcher.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "UI/Common/LSConfirmDialogWidget.h"
#include "UI/LSUILayer.h"
#include "UI/Lobby/LSLobbyTabWidget.h"

#define LOCTEXT_NAMESPACE "LSLoadoutPreparation"

void ULSLoadoutPreparationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 미구현 안내창이 닫힌 뒤 포커스를 이 위젯으로 회수하기 위해 포커스 가능하게 둔다.
	// 이 위젯은 로비 루트의 자식이라, 여기에 포커스가 있으면 로비의 TAB/ESC 처리가 계속 동작한다.
	SetIsFocusable(true);

	if (SupplyTab)
	{
		SupplyTab->OnClicked.AddDynamic(this, &ULSLoadoutPreparationWidget::HandleSupplyTabClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("SupplyTab is not bound on %s."), *GetNameSafe(this));
	}

	if (StorageTab)
	{
		StorageTab->OnClicked.AddDynamic(this, &ULSLoadoutPreparationWidget::HandleStorageTabClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("StorageTab is not bound on %s."), *GetNameSafe(this));
	}

	if (UpgradeTab)
	{
		UpgradeTab->OnClicked.AddDynamic(this, &ULSLoadoutPreparationWidget::HandleUpgradeTabClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("UpgradeTab is not bound on %s."), *GetNameSafe(this));
	}

	if (ChipTab)
	{
		ChipTab->OnClicked.AddDynamic(this, &ULSLoadoutPreparationWidget::HandleChipTabClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("ChipTab is not bound on %s."), *GetNameSafe(this));
	}

	if (!ContentSwitcher)
	{
		UE_LOG(LogLS, Warning, TEXT("ContentSwitcher is not bound on %s."), *GetNameSafe(this));
	}

	// 처음에는 탭만 보이고 콘텐츠는 숨긴다. WBP 디자이너 설정과 무관하게 C++이 초기 상태를 강제한다.
	ResetToTabs();
}

void ULSLoadoutPreparationWidget::NativeDestruct()
{
	if (SupplyTab)
	{
		SupplyTab->OnClicked.RemoveDynamic(this, &ULSLoadoutPreparationWidget::HandleSupplyTabClicked);
	}
	if (StorageTab)
	{
		StorageTab->OnClicked.RemoveDynamic(this, &ULSLoadoutPreparationWidget::HandleStorageTabClicked);
	}
	if (UpgradeTab)
	{
		UpgradeTab->OnClicked.RemoveDynamic(this, &ULSLoadoutPreparationWidget::HandleUpgradeTabClicked);
	}
	if (ChipTab)
	{
		ChipTab->OnClicked.RemoveDynamic(this, &ULSLoadoutPreparationWidget::HandleChipTabClicked);
	}

	Super::NativeDestruct();
}

void ULSLoadoutPreparationWidget::OpenTab(const ELSLoadoutTab Tab) const
{
	ShowTab(Tab);
}

void ULSLoadoutPreparationWidget::ResetToTabs() const
{
	SetTabBarVisible(true);
	if (ContentSwitcher)
	{
		ContentSwitcher->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool ULSLoadoutPreparationWidget::IsContentOpen() const
{
	return ContentSwitcher && ContentSwitcher->GetVisibility() == ESlateVisibility::Visible;
}

void ULSLoadoutPreparationWidget::HandleSupplyTabClicked()
{
	// 에이베리 보급소(상점/제작)는 아직 안 만들어서 나중에 구현할 예정입니다. 그때까지 탭 목록을 유지한다.
	ShowNotImplementedNotice();
}

void ULSLoadoutPreparationWidget::HandleStorageTabClicked()
{
	ShowTab(ELSLoadoutTab::Storage);
}

void ULSLoadoutPreparationWidget::HandleUpgradeTabClicked()
{
	// 업그레이드(캐릭터/기지 강화)는 아직 안 만들어서 나중에 구현할 예정입니다. 그때까지 탭 목록을 유지한다.
	ShowNotImplementedNotice();
}

void ULSLoadoutPreparationWidget::HandleChipTabClicked()
{
	ShowTab(ELSLoadoutTab::Chip);
}

void ULSLoadoutPreparationWidget::ShowTab(const ELSLoadoutTab Tab) const
{
	if (!ContentSwitcher)
	{
		UE_LOG(LogLS, Warning, TEXT("[Loadout] Cannot show tab because ContentSwitcher is not bound on %s."), *GetNameSafe(this));
		return;
	}

	ContentSwitcher->SetActiveWidgetIndex(static_cast<int32>(Tab));
	ContentSwitcher->SetVisibility(ESlateVisibility::Visible);
	SetTabBarVisible(false);
}

void ULSLoadoutPreparationWidget::ShowNotImplementedNotice()
{
	// 이미 안내창이 떠 있으면 중복 생성하지 않는다.
	if (ActiveConfirmDialog && ActiveConfirmDialog->IsInViewport())
	{
		return;
	}

	if (!ConfirmDialogClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Loadout] ConfirmDialogClass is not set on %s. Check WBP_LoadoutPreparation."), *GetNameSafe(this));
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	ULSConfirmDialogWidget* Dialog = OwningPlayer
		? CreateWidget<ULSConfirmDialogWidget>(OwningPlayer, ConfirmDialogClass)
		: CreateWidget<ULSConfirmDialogWidget>(this, ConfirmDialogClass);
	if (!Dialog)
	{
		UE_LOG(LogLS, Warning, TEXT("[Loadout] Failed to create confirm dialog on %s."), *GetNameSafe(this));
		return;
	}

	// 확인/취소 어느 쪽을 눌러도(또는 ESC) 그냥 닫히고 탭 목록으로 돌아온다.
	Dialog->SetMessage(LOCTEXT("NotImplemented", "아직 <Emph>구현</>되지 않았습니다."));
	Dialog->OnConfirmed.AddDynamic(this, &ULSLoadoutPreparationWidget::HandleNotImplementedDialogClosed);
	Dialog->OnCancelled.AddDynamic(this, &ULSLoadoutPreparationWidget::HandleNotImplementedDialogClosed);
	Dialog->AddToViewport(LSUILayer::ModalPanel);
	ActiveConfirmDialog = Dialog;
}

void ULSLoadoutPreparationWidget::HandleNotImplementedDialogClosed()
{
	ActiveConfirmDialog = nullptr;
	// 안내창이 닫혔으니 TAB/ESC가 다시 로비로 오도록 포커스를 회수한다.
	SetKeyboardFocus();
}

void ULSLoadoutPreparationWidget::SetTabBarVisible(const bool bVisible) const
{
	const ESlateVisibility TabVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (ChipTab)
	{
		ChipTab->SetVisibility(TabVisibility);
	}
	if (StorageTab)
	{
		StorageTab->SetVisibility(TabVisibility);
	}
	if (SupplyTab)
	{
		SupplyTab->SetVisibility(TabVisibility);
	}
	if (UpgradeTab)
	{
		UpgradeTab->SetVisibility(TabVisibility);
	}
}

#undef LOCTEXT_NAMESPACE
