#include "UI/Lobby/LSLoadoutPreparationWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/WidgetSwitcher.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "UI/ChipSystem/LSChipStationWidget.h"
#include "UI/Common/LSConfirmDialogWidget.h"
#include "UI/LSUILayer.h"
#include "UI/Inventory/LSInventoryWidget.h"
#include "UI/Lobby/LSLobbyTabWidget.h"
#include "UI/Lobby/Store/LSStoreWidget.h"
#include "UI/Storage/LSLobbyStorageWidget.h"
#include "UI/Skill/LSSkillLoadoutWidget.h"

#define LOCTEXT_NAMESPACE "LSLoadoutPreparation"

namespace
{
// 스위처 페이지(아트가 WBP에 배치) 아래에서 칩 스테이션 위젯을 찾는다. UserWidget 내부 트리까지 재귀 탐색.
ULSChipStationWidget* FindLoadoutChipStationWidget(UWidget* Widget)
{
	if (!Widget)
	{
		return nullptr;
	}

	if (ULSChipStationWidget* ChipStation = Cast<ULSChipStationWidget>(Widget))
	{
		return ChipStation;
	}

	if (const UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
	{
		return UserWidget->WidgetTree ? FindLoadoutChipStationWidget(UserWidget->WidgetTree->RootWidget) : nullptr;
	}

	if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
	{
		for (int32 ChildIndex = 0; ChildIndex < Panel->GetChildrenCount(); ++ChildIndex)
		{
			if (ULSChipStationWidget* ChipStation = FindLoadoutChipStationWidget(Panel->GetChildAt(ChildIndex)))
			{
				return ChipStation;
			}
		}
	}

	return nullptr;
}

// 스위처 페이지 아래에서 스킬 로드아웃 위젯을 찾는다. UserWidget 내부 트리까지 재귀 탐색.
ULSSkillLoadoutWidget* FindLoadoutSkillLoadoutWidget(UWidget* Widget)
{
	if (!Widget)
	{
		return nullptr;
	}

	if (ULSSkillLoadoutWidget* SkillLoadout = Cast<ULSSkillLoadoutWidget>(Widget))
	{
		return SkillLoadout;
	}

	if (const UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
	{
		return UserWidget->WidgetTree ? FindLoadoutSkillLoadoutWidget(UserWidget->WidgetTree->RootWidget) : nullptr;
	}

	if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
	{
		for (int32 ChildIndex = 0; ChildIndex < Panel->GetChildrenCount(); ++ChildIndex)
		{
			if (ULSSkillLoadoutWidget* SkillLoadout = FindLoadoutSkillLoadoutWidget(Panel->GetChildAt(ChildIndex)))
			{
				return SkillLoadout;
			}
		}
	}

	return nullptr;
}

// 스위처 페이지 아래에서 지정 타입 위젯을 찾는다. UserWidget 내부 트리까지 재귀 탐색.
// (기존 타입별 Find 함수들의 공용 버전. 새 타입은 이걸 쓴다.)
template<typename WidgetType>
WidgetType* FindLoadoutWidgetByType(UWidget* Widget)
{
	if (!Widget)
	{
		return nullptr;
	}

	if (WidgetType* Found = Cast<WidgetType>(Widget))
	{
		return Found;
	}

	if (const UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
	{
		return UserWidget->WidgetTree ? FindLoadoutWidgetByType<WidgetType>(UserWidget->WidgetTree->RootWidget) : nullptr;
	}

	if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
	{
		for (int32 ChildIndex = 0; ChildIndex < Panel->GetChildrenCount(); ++ChildIndex)
		{
			if (WidgetType* Found = FindLoadoutWidgetByType<WidgetType>(Panel->GetChildAt(ChildIndex)))
			{
				return Found;
			}
		}
	}

	return nullptr;
}

// 스위처 페이지 아래에서 에이베리 보급소 상점 위젯을 찾는다. UserWidget 내부 트리까지 재귀 탐색.
ULSStoreWidget* FindLoadoutStoreWidget(UWidget* Widget)
{
	if (!Widget)
	{
		return nullptr;
	}

	if (ULSStoreWidget* Store = Cast<ULSStoreWidget>(Widget))
	{
		return Store;
	}

	if (const UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
	{
		return UserWidget->WidgetTree ? FindLoadoutStoreWidget(UserWidget->WidgetTree->RootWidget) : nullptr;
	}

	if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
	{
		for (int32 ChildIndex = 0; ChildIndex < Panel->GetChildrenCount(); ++ChildIndex)
		{
			if (ULSStoreWidget* Store = FindLoadoutStoreWidget(Panel->GetChildAt(ChildIndex)))
			{
				return Store;
			}
		}
	}

	return nullptr;
}
}

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

bool ULSLoadoutPreparationWidget::TryHandleBack() const
{
	if (!IsContentOpen())
	{
		return false;
	}

	if (ContentSwitcher->GetActiveWidgetIndex() == static_cast<int32>(ELSLoadoutTab::Supply))
	{
		if (ULSStoreWidget* Store = FindLoadoutStoreWidget(ContentSwitcher->GetActiveWidget()))
		{
			if (Store->TryHandleBack())
			{
				return true;
			}
		}
	}

	ResetToTabs();
	return true;
}

bool ULSLoadoutPreparationWidget::HasActiveConfirmDialog() const
{
	if (ActiveConfirmDialog && ActiveConfirmDialog->IsInViewport())
	{
		return true;
	}

	// 개인정비 콘텐츠에 중첩된 칩 스테이션도 자체 확인 다이얼로그를 띄운다. 그 다이얼로그가 떠 있으면
	// 로비 메뉴(ULSLobbyMenuWidget)의 매 틱 포커스 회수 가드가 예외로 두도록 여기서 함께 보고한다.
	// (보고하지 않으면 다이얼로그가 매 틱 포커스를 뺏겨 확인 버튼 첫 클릭이 씹힌다.)
	if (ContentSwitcher)
	{
		if (const ULSStoreWidget* Store = FindLoadoutStoreWidget(ContentSwitcher->GetActiveWidget()))
		{
			if (Store->HasActiveConfirmDialog())
			{
				return true;
			}
		}
		if (const ULSChipStationWidget* ChipStation = FindLoadoutChipStationWidget(ContentSwitcher->GetActiveWidget()))
		{
			return ChipStation->HasActiveConfirmDialog();
		}
	}

	return false;
}

void ULSLoadoutPreparationWidget::CloseActiveConfirmDialog()
{
	if (ActiveConfirmDialog && ActiveConfirmDialog->IsInViewport())
	{
		// Cancel이 OnCancelled를 브로드캐스트해 HandleNotImplementedDialogClosed에서 참조 정리까지 이어진다.
		ActiveConfirmDialog->Cancel();
	}
}

void ULSLoadoutPreparationWidget::HandleSupplyTabClicked()
{
	CloseActiveConfirmDialog();
	ShowTab(ELSLoadoutTab::Supply);
}

void ULSLoadoutPreparationWidget::HandleStorageTabClicked()
{
	CloseActiveConfirmDialog();
	ShowTab(ELSLoadoutTab::Storage);
}

void ULSLoadoutPreparationWidget::HandleUpgradeTabClicked()
{
	// 전용 스킬 탭 버튼이 없어서, 업그레이드 버튼으로 스킬 로드아웃 페이지를 연다.
	CloseActiveConfirmDialog();
	ShowTab(ELSLoadoutTab::Skill);
}

void ULSLoadoutPreparationWidget::HandleChipTabClicked()
{
	CloseActiveConfirmDialog();
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

	// 칩 스테이션 페이지는 열 때마다 최신 데이터로 리빌드한다. 다른 탭(창고 정렬/이동 등)에서 인벤토리·창고
	// 인덱스가 바뀌어도 칩 리스트가 stale 인덱스를 들고 있지 않도록 한다(장착이 조용히 실패하던 원인).
	if (Tab == ELSLoadoutTab::Chip)
	{
		if (ULSChipStationWidget* ChipStation = FindLoadoutChipStationWidget(ContentSwitcher->GetActiveWidget()))
		{
			ChipStation->RefreshChipStation();
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("[Loadout] Chip station widget not found under chip tab page on %s."), *GetNameSafe(this));
		}
	}
	// 물품창고 페이지도 열 때마다 최신 세이브 기준으로 리빌드한다.
	// 상점 구매/판매 등 다른 페이지에서 인벤토리·창고가 바뀐 뒤 열어도 stale 표시가 없게 한다.
	else if (Tab == ELSLoadoutTab::Storage)
	{
		UWidget* StoragePage = ContentSwitcher->GetActiveWidget();
		if (ULSInventoryWidget* Inventory = FindLoadoutWidgetByType<ULSInventoryWidget>(StoragePage))
		{
			Inventory->RebuildInventorySlots();
			Inventory->RebuildConfirmedStorageSlots();
			Inventory->RebuildEquipmentSlots();
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("[Loadout] Inventory widget not found under storage tab page on %s."), *GetNameSafe(this));
		}
		if (ULSLobbyStorageWidget* Storage = FindLoadoutWidgetByType<ULSLobbyStorageWidget>(StoragePage))
		{
			Storage->RefreshStorage();
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("[Loadout] Storage widget not found under storage tab page on %s."), *GetNameSafe(this));
		}
	}
	// 보급소(상점) 페이지는 열 때마다 초기 상태(기능 선택)로 되돌린다. 대화 도중 닫았다 열어도 이어지지 않게.
	else if (Tab == ELSLoadoutTab::Supply)
	{
		if (ULSStoreWidget* Store = FindLoadoutStoreWidget(ContentSwitcher->GetActiveWidget()))
		{
			Store->SetConfirmDialogClass(ConfirmDialogClass);
			Store->ResetStore();
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("[Loadout] Store widget not found under supply tab page on %s."), *GetNameSafe(this));
		}
	}
	// 스킬 로드아웃 페이지도 열 때마다 최신 세이브 기준으로 리빌드한다.
	else if (Tab == ELSLoadoutTab::Skill)
	{
		if (ULSSkillLoadoutWidget* SkillLoadout = FindLoadoutSkillLoadoutWidget(ContentSwitcher->GetActiveWidget()))
		{
			SkillLoadout->RefreshSkillLoadout();
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("[Loadout] Skill loadout widget not found under skill tab page on %s."), *GetNameSafe(this));
		}
	}
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

	// 탭 옆 장식 이미지도 탭 목록과 같이 숨긴다.
	const TObjectPtr<UImage> TabImages[] = { ChipTabImage, StorageTabImage, SupplyTabImage, UpgradeTabImage };
	for (const TObjectPtr<UImage>& TabImage : TabImages)
	{
		if (TabImage)
		{
			TabImage->SetVisibility(TabVisibility);
		}
	}
}

#undef LOCTEXT_NAMESPACE
