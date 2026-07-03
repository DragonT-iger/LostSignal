#include "UI/Lobby/LSLoadoutPreparationWidget.h"

#include "Components/WidgetSwitcher.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "UI/Common/LSConfirmDialogWidget.h"
#include "UI/Common/LSNoiseDissolveWidget.h"
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
	// 초기 상태는 연출 없이 즉시 Collapse(디졸브가 시작하자마자 터지는 것 방지).
	CollapseContentToTabs();
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

void ULSLoadoutPreparationWidget::OpenTab(const ELSLoadoutTab Tab)
{
	ShowTab(Tab);
}

void ULSLoadoutPreparationWidget::ResetToTabs()
{
	// 콘텐츠가 열려 있고 현재 페이지가 디졸브 가능하면, 좌우 노이즈로 내보낸 뒤 완료 콜백에서 Collapse한다.
	// 스위처는 전환 애니메이션이 없어(활성 자식을 즉시 교체) 이렇게 순서를 직접 만들어야 한다.
	if (ContentSwitcher && ContentSwitcher->GetVisibility() == ESlateVisibility::Visible)
	{
		if (ULSNoiseDissolveWidget* ActivePage = Cast<ULSNoiseDissolveWidget>(ContentSwitcher->GetActiveWidget()))
		{
			// 대기 인덱스 없음 = 전환이 아니라 닫기. 디졸브 완료 후 탭 목록으로 돌아간다.
			PendingSwitchIndex = INDEX_NONE;
			if (!ActivePage->IsDissolvingOut())
			{
				ActivePage->OnDissolveFinished.AddUniqueDynamic(this, &ULSLoadoutPreparationWidget::HandleContentDissolveFinished);
				ActivePage->StartDissolveOut();
			}
			return;
		}
	}

	// 디졸브 대상이 아니거나(미변환 페이지) 이미 닫힌 상태 → 즉시 탭 목록으로.
	CollapseContentToTabs();
}

void ULSLoadoutPreparationWidget::CollapseContentToTabs()
{
	SetTabBarVisible(true);
	if (ContentSwitcher)
	{
		ContentSwitcher->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULSLoadoutPreparationWidget::HandleContentDissolveFinished()
{
	// 전환 대기 인덱스가 있으면 그 페이지로 전환, 없으면 닫기(탭 목록 복귀).
	const int32 SwitchTo = PendingSwitchIndex;
	PendingSwitchIndex = INDEX_NONE;
	if (SwitchTo != INDEX_NONE)
	{
		ApplyActivePage(SwitchTo);
		return;
	}

	CollapseContentToTabs();
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

void ULSLoadoutPreparationWidget::ShowTab(const ELSLoadoutTab Tab)
{
	if (!ContentSwitcher)
	{
		UE_LOG(LogLS, Warning, TEXT("[Loadout] Cannot show tab because ContentSwitcher is not bound on %s."), *GetNameSafe(this));
		return;
	}

	const int32 TargetIndex = static_cast<int32>(Tab);

	// 콘텐츠가 이미 열려 있고 다른 페이지로 바꾸는 경우: 현재 페이지를 디졸브로 내보낸 뒤 완료 콜백에서 전환한다.
	if (ContentSwitcher->GetVisibility() == ESlateVisibility::Visible
		&& ContentSwitcher->GetActiveWidgetIndex() != TargetIndex)
	{
		if (ULSNoiseDissolveWidget* CurrentPage = Cast<ULSNoiseDissolveWidget>(ContentSwitcher->GetActiveWidget()))
		{
			if (!CurrentPage->IsDissolvingOut())
			{
				PendingSwitchIndex = TargetIndex;
				CurrentPage->OnDissolveFinished.AddUniqueDynamic(this, &ULSLoadoutPreparationWidget::HandleContentDissolveFinished);
				CurrentPage->StartDissolveOut();
			}
			return;
		}
	}

	// 닫힌 상태에서 열거나(첫 진입) 디졸브 대상이 아니면 바로 전환한다.
	ApplyActivePage(TargetIndex);
}

void ULSLoadoutPreparationWidget::ApplyActivePage(const int32 PageIndex)
{
	if (!ContentSwitcher)
	{
		return;
	}

	ContentSwitcher->SetActiveWidgetIndex(PageIndex);

	// 이전에 디졸브로 사라진(Collapsed) 페이지라면 노이즈량과 가시성을 복구해 다시 선명히 보이게 한다.
	// 스위처는 활성 자식의 자체 visibility도 따르므로, 이걸 안 하면 재진입 시 페이지가 안 보인다.
	if (ULSNoiseDissolveWidget* ActivePage = Cast<ULSNoiseDissolveWidget>(ContentSwitcher->GetActiveWidget()))
	{
		ActivePage->ResetDissolve();
		ActivePage->SetVisibility(ESlateVisibility::Visible);
	}

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
