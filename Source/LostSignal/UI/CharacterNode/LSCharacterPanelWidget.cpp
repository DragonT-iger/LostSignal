#include "UI/CharacterNode/LSCharacterPanelWidget.h"

#include "Components/WidgetSwitcher.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "UI/CharacterNode/LSSkillNodeGraphWidget.h"
#include "UI/Lobby/LSLobbyTabWidget.h"
#include "UI/Skill/LSSkillLoadoutWidget.h"

namespace
{
	// 유니티 빌드 충돌을 피하려고 도메인 접두사를 붙인다.
	UUserWidget* CharacterPanelCreatePage(UUserWidget& Owner, UClass* PageClass, const TCHAR* PageClassName)
	{
		if (!PageClass)
		{
			UE_LOG(LogLS, Warning, TEXT("[CharacterPanel] %s가 미설정이라 페이지를 만들 수 없다 - %s"),
				PageClassName, *GetNameSafe(&Owner));
			return nullptr;
		}

		APlayerController* OwningPlayer = Owner.GetOwningPlayer();
		UUserWidget* Page = OwningPlayer
			? CreateWidget<UUserWidget>(OwningPlayer, PageClass)
			: CreateWidget<UUserWidget>(&Owner, PageClass);
		if (!Page)
		{
			UE_LOG(LogLS, Warning, TEXT("[CharacterPanel] %s 생성에 실패했다 - %s"), PageClassName, *GetNameSafe(&Owner));
		}
		return Page;
	}
}

void ULSCharacterPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CreateSubPages();
	ApplyCharacterToPages();

	if (SkillLoadoutTab)
	{
		SkillLoadoutTab->OnClicked.RemoveAll(this);
		SkillLoadoutTab->OnClicked.AddDynamic(this, &ULSCharacterPanelWidget::HandleSubTabClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[CharacterPanel] SkillLoadoutTab이 바인드되지 않았다 - %s"), *GetNameSafe(this));
	}

	if (NodeGraphTab)
	{
		NodeGraphTab->OnClicked.RemoveAll(this);
		NodeGraphTab->OnClicked.AddDynamic(this, &ULSCharacterPanelWidget::HandleSubTabClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[CharacterPanel] NodeGraphTab이 바인드되지 않았다 - %s"), *GetNameSafe(this));
	}

	// 최초 표시를 강제한다. ShowSubTab 은 같은 탭 재요청을 무시하므로 여기서 직접 반영한다.
	if (UWidget* Page = ResolveSubTabPage(ActiveSubTab))
	{
		if (SubTabSwitcher)
		{
			SubTabSwitcher->SetActiveWidget(Page);
		}
	}
	UpdateSubTabSelection();
}

void ULSCharacterPanelWidget::CreateSubPages()
{
	if (!SubTabSwitcher)
	{
		UE_LOG(LogLS, Warning,
			TEXT("[CharacterPanel] SubTabSwitcher가 바인드되지 않았다 - WBP에 WidgetSwitcher를 두고 이름을 SubTabSwitcher로 맞춰라 (%s)"),
			*GetNameSafe(this));
		return;
	}

	// 로비 루트와 같은 방어선이다. 디자인 타임에 페이지를 넣으면 런타임 생성분과 겹쳐 두 벌이 된다.
	const bool bHasRuntimePage = SkillLoadoutPage || NodeGraphPage;
	if (!bHasRuntimePage && SubTabSwitcher->GetChildrenCount() > 0)
	{
		UE_LOG(LogLS, Warning,
			TEXT("[CharacterPanel] SubTabSwitcher는 WBP에서 비워 둬야 하는데 이미 자식이 %d개다 - %s"),
			SubTabSwitcher->GetChildrenCount(), *GetNameSafe(this));
	}

	if (!SkillLoadoutPage)
	{
		SkillLoadoutPage = Cast<ULSSkillLoadoutWidget>(
			CharacterPanelCreatePage(*this, SkillLoadoutPageClass.Get(), TEXT("SkillLoadoutPageClass")));
		if (SkillLoadoutPage)
		{
			SubTabSwitcher->AddChild(SkillLoadoutPage);
		}
	}

	if (!NodeGraphPage)
	{
		NodeGraphPage = Cast<ULSSkillNodeGraphWidget>(
			CharacterPanelCreatePage(*this, NodeGraphPageClass.Get(), TEXT("NodeGraphPageClass")));
		if (NodeGraphPage)
		{
			SubTabSwitcher->AddChild(NodeGraphPage);
		}
	}
}

void ULSCharacterPanelWidget::ApplyCharacterToPages()
{
	// 캐릭터를 페이지들에 전파하는 유일한 지점.
	//
	// 스킬 로드아웃은 여기에 없다. SkillPool 이 EditDefaultsOnly 자산이라 캐릭터가 WBP 에 구워져 있고,
	// CharacterID 로 SkillPool 을 찾는 경로가 프로젝트에 아직 없다. 세터가 생기면 이 줄 옆에 붙인다.
	if (NodeGraphPage)
	{
		NodeGraphPage->SetCharacterID(CharacterID);
	}
}

void ULSCharacterPanelWidget::RefreshCharacterPanel()
{
	RefreshSubTabPage(ActiveSubTab);
}

void ULSCharacterPanelWidget::ShowSubTab(const ELSCharacterSubTab SubTab)
{
	// 같은 탭 재클릭은 무시한다. 이미 보고 있는 페이지라 stale 데이터가 생길 여지가 없고 선택 상태도 보존된다.
	if (SubTab == ActiveSubTab)
	{
		return;
	}

	if (!SubTabSwitcher)
	{
		return;
	}

	UWidget* Page = ResolveSubTabPage(SubTab);
	if (!Page)
	{
		UE_LOG(LogLS, Warning, TEXT("[CharacterPanel] 서브탭 %d의 페이지가 없다 - %s"),
			static_cast<int32>(SubTab), *GetNameSafe(this));
		return;
	}

	// 인덱스가 아니라 포인터로 전환한다. 아트가 스위처 안 페이지 순서를 바꿔도 깨지지 않는다.
	SubTabSwitcher->SetActiveWidget(Page);
	ActiveSubTab = SubTab;
	UpdateSubTabSelection();

	// 다른 서브탭에서 값이 바뀌었을 수 있으므로 여는 쪽을 최신 기준으로 다시 그린다(가방 패널과 같은 방어선).
	RefreshSubTabPage(SubTab);
}

UWidget* ULSCharacterPanelWidget::ResolveSubTabPage(const ELSCharacterSubTab SubTab) const
{
	switch (SubTab)
	{
	case ELSCharacterSubTab::SkillLoadout: return SkillLoadoutPage;
	case ELSCharacterSubTab::NodeGraph:    return NodeGraphPage;
	default:                               return nullptr;
	}
}

void ULSCharacterPanelWidget::RefreshSubTabPage(const ELSCharacterSubTab SubTab) const
{
	switch (SubTab)
	{
	case ELSCharacterSubTab::SkillLoadout:
		if (SkillLoadoutPage)
		{
			SkillLoadoutPage->RefreshSkillLoadout();
		}
		break;

	case ELSCharacterSubTab::NodeGraph:
		if (NodeGraphPage)
		{
			NodeGraphPage->RefreshGraph();
		}
		break;

	default:
		break;
	}
}

void ULSCharacterPanelWidget::UpdateSubTabSelection() const
{
	if (SkillLoadoutTab)
	{
		SkillLoadoutTab->SetSelected(ActiveSubTab == ELSCharacterSubTab::SkillLoadout);
	}
	if (NodeGraphTab)
	{
		NodeGraphTab->SetSelected(ActiveSubTab == ELSCharacterSubTab::NodeGraph);
	}
}

void ULSCharacterPanelWidget::HandleSubTabClicked(ULSLobbyTabWidget* ClickedTab)
{
	if (!ClickedTab)
	{
		return;
	}

	if (ClickedTab == SkillLoadoutTab)
	{
		ShowSubTab(ELSCharacterSubTab::SkillLoadout);
	}
	else if (ClickedTab == NodeGraphTab)
	{
		ShowSubTab(ELSCharacterSubTab::NodeGraph);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[CharacterPanel] 매핑되지 않은 서브탭 클릭: %s"), *GetNameSafe(ClickedTab));
	}
}
