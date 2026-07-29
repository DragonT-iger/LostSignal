#include "UI/Lobby/Store/LSStoreWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "LostSignal.h"
#include "UI/Lobby/Crafting/LSCraftingWidget.h"
#include "UI/Lobby/Store/LSStoreButtonWidget.h"
#include "UI/Lobby/Store/LSVendingWidget.h"

#define LOCTEXT_NAMESPACE "LSStore"

namespace
{
// 대화 목록에 한 번에 표시하는 항목 수(기획: 3개 표시, 화살표로 하나씩 넘김).
constexpr int32 StoreTalkListVisibleCount = 3;

// CASHIER-9 기본 대기 대사. 기능 선택/대화 목록 상태에서 공통으로 표시한다.
FText GetStoreIdleDialogue()
{
	return LOCTEXT("IdleDialogue", "CASHIER-9, 거래/제작 모드로 대기 중-");
}
}

void ULSStoreWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ButtonBox)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] ButtonBox is not bound on %s."), *GetNameSafe(this));
	}
	if (!SelectionPanel)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] SelectionPanel is not bound on %s."), *GetNameSafe(this));
	}
	if (!VendingWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] VendingWidgetClass is not set on %s."), *GetNameSafe(this));
	}
	if (!CraftingWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] CraftingWidgetClass is not set on %s."), *GetNameSafe(this));
	}
	if (!DialogueText)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] DialogueText is not bound on %s."), *GetNameSafe(this));
	}
	if (!StoreButtonClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] StoreButtonClass is not set on %s."), *GetNameSafe(this));
	}

	if (TalkUpButton)
	{
		TalkUpButton->OnClicked.AddDynamic(this, &ULSStoreWidget::HandleTalkUpClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] TalkUpButton is not bound on %s."), *GetNameSafe(this));
	}
	if (TalkDownButton)
	{
		TalkDownButton->OnClicked.AddDynamic(this, &ULSStoreWidget::HandleTalkDownClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] TalkDownButton is not bound on %s."), *GetNameSafe(this));
	}

	BuildTalkEntries();
	ResetStore();
}

void ULSStoreWidget::NativeDestruct()
{
	// 숨겨져 ActiveButtons에 없는 버튼까지 풀 전체를 해제한다.
	for (ULSStoreButtonWidget* StoreButton : ButtonPool)
	{
		if (StoreButton)
		{
			StoreButton->OnClicked.RemoveAll(this);
		}
	}
	if (TalkUpButton)
	{
		TalkUpButton->OnClicked.RemoveDynamic(this, &ULSStoreWidget::HandleTalkUpClicked);
	}
	if (TalkDownButton)
	{
		TalkDownButton->OnClicked.RemoveDynamic(this, &ULSStoreWidget::HandleTalkDownClicked);
	}
	if (VendingPanel)
	{
		VendingPanel->OnBackRequested.RemoveDynamic(this, &ULSStoreWidget::HandleVendingBackRequested);
	}

	Super::NativeDestruct();
}

void ULSStoreWidget::ResetStore()
{
	TalkListStartIndex = 0;
	SetVendingVisible(false);
	SetCraftingVisible(false);
	ShowState(ELSStoreState::FunctionSelect);
}

bool ULSStoreWidget::TryHandleBack()
{
	if (VendingPanel && VendingPanel->IsVisible())
	{
		HandleVendingBackRequested();
		return true;
	}
	if (CraftingPanel && CraftingPanel->IsVisible())
	{
		SetCraftingVisible(false);
		ShowState(ELSStoreState::FunctionSelect);
		return true;
	}
	return false;
}

void ULSStoreWidget::SetConfirmDialogClass(const TSubclassOf<ULSConfirmDialogWidget> InConfirmDialogClass)
{
	ConfirmDialogClass = InConfirmDialogClass;
	if (VendingPanel)
	{
		VendingPanel->SetConfirmDialogClass(ConfirmDialogClass);
	}
}

bool ULSStoreWidget::HasActiveConfirmDialog() const
{
	return VendingPanel && VendingPanel->HasActiveConfirmDialog();
}

void ULSStoreWidget::CloseActiveConfirmDialog()
{
	if (VendingPanel)
	{
		VendingPanel->CloseActiveConfirmDialog();
	}
}

void ULSStoreWidget::SetVendingVisible(const bool bVisible)
{
	// 자판기 위젯은 처음 열 때만 생성한다. 닫기 경로에서는 이미 있는 위젯만 숨긴다.
	ULSVendingWidget* Vending = bVisible ? EnsureVendingWidget() : VendingPanel.Get();
	if (bVisible && !Vending)
	{
		return;
	}

	if (SelectionPanel)
	{
		SelectionPanel->SetVisibility(bVisible ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (Vending)
	{
		Vending->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		if (bVisible)
		{
			Vending->OpenVending();
		}
	}
	if (bVisible && CraftingPanel)
	{
		CraftingPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

ULSVendingWidget* ULSStoreWidget::EnsureVendingWidget()
{
	if (VendingPanel)
	{
		return VendingPanel;
	}
	if (!VendingWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] Cannot open vending. VendingWidgetClass is not set on %s."), *GetNameSafe(this));
		return nullptr;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!RootCanvas)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] Cannot open vending. Root widget of %s is not a CanvasPanel."), *GetNameSafe(this));
		return nullptr;
	}
	if (RootCanvas == SelectionPanel)
	{
		// 루트가 SelectionPanel이면 자판기가 그 자식으로 붙어, SelectionPanel을 숨길 때 같이 숨는다.
		// WBP_Store는 [루트 캔버스 > SelectionPanel > 기존 콘텐츠] 구조여야 한다.
		UE_LOG(LogLS, Warning, TEXT("[Store] Cannot open vending. SelectionPanel must not be the root widget of %s."), *GetNameSafe(this));
		return nullptr;
	}

	VendingPanel = CreateWidget<ULSVendingWidget>(this, VendingWidgetClass);
	if (!VendingPanel)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] Failed to create vending widget on %s."), *GetNameSafe(this));
		return nullptr;
	}
	VendingPanel->SetConfirmDialogClass(ConfirmDialogClass);

	VendingPanel->OnBackRequested.AddDynamic(this, &ULSStoreWidget::HandleVendingBackRequested);

	// 루트 캔버스에 전체 화면으로 붙인다.
	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(VendingPanel);
	if (PanelSlot)
	{
		PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		PanelSlot->SetOffsets(FMargin(0.0f));
	}
	return VendingPanel;
}

void ULSStoreWidget::HandleVendingBackRequested()
{
	SetVendingVisible(false);
	ShowState(ELSStoreState::FunctionSelect);
}

void ULSStoreWidget::SetCraftingVisible(const bool bVisible)
{
	ULSCraftingWidget* Crafting = bVisible ? EnsureCraftingWidget() : CraftingPanel.Get();
	if (bVisible && !Crafting)
	{
		return;
	}

	if (SelectionPanel)
	{
		SelectionPanel->SetVisibility(bVisible ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (VendingPanel && bVisible)
	{
		VendingPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Crafting)
	{
		Crafting->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		if (bVisible)
		{
			Crafting->OpenCrafting();
		}
	}
}

ULSCraftingWidget* ULSStoreWidget::EnsureCraftingWidget()
{
	if (CraftingPanel)
	{
		return CraftingPanel;
	}
	if (!CraftingWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] Cannot open crafting. CraftingWidgetClass is not set on %s."), *GetNameSafe(this));
		return nullptr;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!RootCanvas || RootCanvas == SelectionPanel)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] Cannot attach crafting to the root canvas on %s."), *GetNameSafe(this));
		return nullptr;
	}

	CraftingPanel = CreateWidget<ULSCraftingWidget>(this, CraftingWidgetClass);
	if (!CraftingPanel)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] Failed to create crafting widget on %s."), *GetNameSafe(this));
		return nullptr;
	}

	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(CraftingPanel))
	{
		PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		PanelSlot->SetOffsets(FMargin(0.0f));
	}
	return CraftingPanel;
}

void ULSStoreWidget::HandleStoreButtonClicked(ULSStoreButtonWidget* ClickedButton)
{
	const int32 ButtonOrdinal = ActiveButtons.IndexOfByKey(ClickedButton);
	if (ButtonOrdinal != INDEX_NONE)
	{
		HandleClickForState(ButtonOrdinal);
	}
}

void ULSStoreWidget::HandleTalkUpClicked()
{
	// 한 칸씩 위로. 더 보여줄 대화가 없으면 목록을 고정한다.
	if (TalkListStartIndex > 0)
	{
		--TalkListStartIndex;
		RefreshTalkList();
	}
}

void ULSStoreWidget::HandleTalkDownClicked()
{
	// 한 칸씩 아래로. 마지막 항목들이 보이는 위치를 넘지 않는다.
	if (TalkListStartIndex < TalkEntries.Num() - StoreTalkListVisibleCount)
	{
		++TalkListStartIndex;
		RefreshTalkList();
	}
}

void ULSStoreWidget::ShowState(const ELSStoreState NewState)
{
	CurrentState = NewState;

	switch (NewState)
	{
	case ELSStoreState::FunctionSelect:
	{
		RebuildButtons(3);
		const FText Labels[] = { LOCTEXT("Vending", "자판기"), LOCTEXT("Craft", "제작대"), LOCTEXT("Talk", "대화하기") };
		for (int32 Index = 0; Index < ActiveButtons.Num(); ++Index)
		{
			ActiveButtons[Index]->SetLabel(Labels[Index]);
		}
		// 퀘스트 발생 시(수락 전) 대화하기 버튼에 퀘스트 아이콘을 표시한다.
		if (ActiveButtons.IsValidIndex(2))
		{
			ActiveButtons[2]->SetQuestIconVisible(HasPendingQuest());
		}
		break;
	}
	case ELSStoreState::TalkList:
		RebuildButtons(FMath::Min(StoreTalkListVisibleCount, TalkEntries.Num()));
		RefreshTalkList();
		break;
	case ELSStoreState::QuestOffer:
	{
		RebuildButtons(2);
		const FText Labels[] = { LOCTEXT("Accept", "수락"), LOCTEXT("Decline", "거절") };
		for (int32 Index = 0; Index < ActiveButtons.Num(); ++Index)
		{
			ActiveButtons[Index]->SetLabel(Labels[Index]);
		}
		break;
	}
	case ELSStoreState::TalkResult:
		RebuildButtons(1);
		if (ActiveButtons.IsValidIndex(0))
		{
			ActiveButtons[0]->SetLabel(LOCTEXT("Confirm", "확인"));
			// 확인 버튼은 버티컬 박스 안에서 맨 아래로 정렬한다(기획서 6/8 슬라이드 위치).
			if (UVerticalBoxSlot* ConfirmSlot = Cast<UVerticalBoxSlot>(ActiveButtons[0]->Slot))
			{
				ConfirmSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				ConfirmSlot->SetVerticalAlignment(VAlign_Bottom);
			}
		}
		break;
	}

	// 화살표는 대화 목록에서 한 화면(3개)에 다 못 담을 때만 보인다(퀘스트가 여러 개 쌓인 상황 등).
	const bool bShowArrows =
		(NewState == ELSStoreState::TalkList) && (TalkEntries.Num() > StoreTalkListVisibleCount);
	const ESlateVisibility ArrowVisibility =
		bShowArrows ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (TalkUpButton)
	{
		TalkUpButton->SetVisibility(ArrowVisibility);
	}
	if (TalkDownButton)
	{
		TalkDownButton->SetVisibility(ArrowVisibility);
	}

	// 기능 선택/대화 목록은 대기 대사. 수락·거절/결과 대사는 전환한 쪽에서 덮어쓴다.
	if (DialogueText &&
		(NewState == ELSStoreState::FunctionSelect || NewState == ELSStoreState::TalkList))
	{
		DialogueText->SetText(GetStoreIdleDialogue());
	}
}

void ULSStoreWidget::HandleClickForState(const int32 ButtonOrdinal)
{
	switch (CurrentState)
	{
	case ELSStoreState::FunctionSelect:
		// 자판기(0)는 구매/판매, 제작대(1)는 제작 화면으로 전환한다.
		if (ButtonOrdinal == 0)
		{
			SetVendingVisible(true);
		}
		else if (ButtonOrdinal == 1)
		{
			SetCraftingVisible(true);
		}
		else if (ButtonOrdinal == 2)
		{
			TalkListStartIndex = 0;
			ShowState(ELSStoreState::TalkList);
		}
		break;
	case ELSStoreState::TalkList:
		HandleTalkEntrySelected(TalkListStartIndex + ButtonOrdinal);
		break;
	case ELSStoreState::QuestOffer:
		HandleQuestOfferSelected(ButtonOrdinal == 0);
		break;
	case ELSStoreState::TalkResult:
		// 확인을 누르면 기능 선택(2번 슬라이드 UI)으로 돌아간다.
		ShowState(ELSStoreState::FunctionSelect);
		break;
	}
}

void ULSStoreWidget::HandleTalkEntrySelected(const int32 EntryIndex)
{
	if (!TalkEntries.IsValidIndex(EntryIndex))
	{
		return;
	}

	const FLSStoreTalkEntry& Entry = TalkEntries[EntryIndex];
	// 퀘스트 대화는 수락/거절 선택으로, 일반 대화는 본문 + 확인으로 분기한다.
	ShowState(Entry.bQuest ? ELSStoreState::QuestOffer : ELSStoreState::TalkResult);
	if (DialogueText)
	{
		DialogueText->SetText(Entry.Body);
	}
}

void ULSStoreWidget::HandleQuestOfferSelected(const bool bAccepted)
{
	if (bAccepted)
	{
		// 임시 하드코딩: 수락하면 퀘스트 대화를 목록에서 제거한다. 진행/완료 처리는 퀘스트 시스템에서 추후 구현.
		bQuestAccepted = true;
		BuildTalkEntries();
	}

	ShowState(ELSStoreState::TalkResult);
	if (DialogueText)
	{
		DialogueText->SetText(bAccepted
			? LOCTEXT("QuestAccepted", "고마워.\n10마리를 잡은 후 다시 말을 걸어줘.")
			: LOCTEXT("QuestDeclined", "왜 거절하는 거야?\n통신이 끊기면 너도 손해라고!"));
	}
}

void ULSStoreWidget::EnsureButtonPool()
{
	if (bButtonPoolInitialized || !ButtonBox)
	{
		return;
	}
	bButtonPoolInitialized = true;

	// 화살표(UButton)는 타입이 달라 자연히 걸러진다.
	for (int32 ChildIndex = 0; ChildIndex < ButtonBox->GetChildrenCount(); ++ChildIndex)
	{
		if (ULSStoreButtonWidget* StoreButton = Cast<ULSStoreButtonWidget>(ButtonBox->GetChildAt(ChildIndex)))
		{
			ButtonPool.Add(StoreButton);
		}
	}

	if (ButtonPool.IsEmpty())
	{
		UE_LOG(LogLS, Warning,
			TEXT("[Store] ButtonBox has no designer-placed store buttons on %s. Runtime buttons fall back to default slots."),
			*GetNameSafe(this));
		return;
	}

	// 아트가 디자이너에서 잡은 슬롯 값을 원본으로 캡처한다. 이후 상태 전환은 이 값으로 되돌린다.
	if (const UVerticalBoxSlot* TemplateSlot = Cast<UVerticalBoxSlot>(ButtonPool[0]->Slot))
	{
		ButtonSlotSize = TemplateSlot->GetSize();
		ButtonSlotPadding = TemplateSlot->GetPadding();
		ButtonSlotHAlign = TemplateSlot->GetHorizontalAlignment();
		ButtonSlotVAlign = TemplateSlot->GetVerticalAlignment();
		bSlotTemplateCaptured = true;
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] Failed to capture the button slot template on %s."), *GetNameSafe(this));
	}

	const ESlateVisibility DesignerVisibility = ButtonPool[0]->GetVisibility();
	ButtonVisibleState = (DesignerVisibility == ESlateVisibility::Collapsed || DesignerVisibility == ESlateVisibility::Hidden)
		? ESlateVisibility::Visible
		: DesignerVisibility;
}

int32 ULSStoreWidget::GetButtonInsertIndex() const
{
	if (!ButtonPool.IsEmpty())
	{
		const int32 LastPoolIndex = ButtonBox->GetChildIndex(ButtonPool.Last());
		if (LastPoolIndex != INDEX_NONE)
		{
			return LastPoolIndex + 1;
		}
	}

	// 풀이 비어 있을 때만 쓰는 경로. 아래 화살표가 박스 안에 있으면 그 앞에 넣는다.
	if (TalkDownButton)
	{
		const int32 DownIndex = ButtonBox->GetChildIndex(TalkDownButton);
		if (DownIndex != INDEX_NONE)
		{
			return DownIndex;
		}
	}
	return ButtonBox->GetChildrenCount();
}

void ULSStoreWidget::ApplyButtonSlotTemplate(ULSStoreButtonWidget* StoreButton) const
{
	if (!bSlotTemplateCaptured || !StoreButton)
	{
		return;
	}

	if (UVerticalBoxSlot* BoxSlot = Cast<UVerticalBoxSlot>(StoreButton->Slot))
	{
		BoxSlot->SetSize(ButtonSlotSize);
		BoxSlot->SetPadding(ButtonSlotPadding);
		BoxSlot->SetHorizontalAlignment(ButtonSlotHAlign);
		BoxSlot->SetVerticalAlignment(ButtonSlotVAlign);
	}
}

void ULSStoreWidget::BindStoreButton(ULSStoreButtonWidget* StoreButton)
{
	if (StoreButton)
	{
		StoreButton->OnClicked.AddUniqueDynamic(this, &ULSStoreWidget::HandleStoreButtonClicked);
	}
}

void ULSStoreWidget::RebuildButtons(const int32 Count)
{
	if (!ButtonBox)
	{
		return;
	}

	EnsureButtonPool();

	// 디자이너 버튼이 모자랄 때만(퀘스트가 여러 개 쌓인 대화 목록 등) 부족분을 만들어 같은 슬롯으로 끼운다.
	while (ButtonPool.Num() < Count && StoreButtonClass)
	{
		ULSStoreButtonWidget* NewButton = CreateWidget<ULSStoreButtonWidget>(this, StoreButtonClass);
		if (!NewButton)
		{
			UE_LOG(LogLS, Warning, TEXT("[Store] Failed to create store button on %s."), *GetNameSafe(this));
			break;
		}
		ButtonBox->InsertChildAt(GetButtonInsertIndex(), NewButton);
		ButtonPool.Add(NewButton);
		ApplyButtonSlotTemplate(NewButton);
	}

	// 남는 버튼은 지우지 않고 숨긴다. 지우면 디자이너가 잡아둔 슬롯 설정과 화살표 배치를 잃는다.
	ActiveButtons.Reset();
	for (int32 PoolIndex = 0; PoolIndex < ButtonPool.Num(); ++PoolIndex)
	{
		ULSStoreButtonWidget* StoreButton = ButtonPool[PoolIndex];
		if (!StoreButton)
		{
			continue;
		}

		BindStoreButton(StoreButton);
		if (PoolIndex < Count)
		{
			// 확인 버튼 상태에서 덮어쓴 슬롯이 다음 상태까지 남지 않도록 매번 원본으로 되돌린다.
			ApplyButtonSlotTemplate(StoreButton);
			StoreButton->SetVisibility(ButtonVisibleState);
			ActiveButtons.Add(StoreButton);
		}
		else
		{
			StoreButton->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ULSStoreWidget::RefreshTalkList()
{
	for (int32 Ordinal = 0; Ordinal < ActiveButtons.Num(); ++Ordinal)
	{
		const int32 EntryIndex = TalkListStartIndex + Ordinal;
		if (ActiveButtons[Ordinal] && TalkEntries.IsValidIndex(EntryIndex))
		{
			ActiveButtons[Ordinal]->SetLabel(TalkEntries[EntryIndex].Label);
			ActiveButtons[Ordinal]->SetQuestIconVisible(TalkEntries[EntryIndex].bQuest);
		}
	}
}

void ULSStoreWidget::BuildTalkEntries()
{
	// 퀘스트 시스템/DataTable이 아직 없어 임시로 하드코딩한다. 퀘스트 대화는 최상단(오래된 순 오름차순 가정).
	TalkEntries.Reset();

	if (!bQuestAccepted)
	{
		FLSStoreTalkEntry QuestEntry;
		QuestEntry.Label = LOCTEXT("QuestRatLabel", "쥐를 잡자");
		QuestEntry.Body = LOCTEXT("QuestRatBody", "쥐형 몬스터가 돌아다니며 전선을 갉아먹는 통에 통신이 어렵다고 함.\n일단 10마리 정도 잡아왔으면 좋겠음.");
		QuestEntry.bQuest = true;
		TalkEntries.Add(QuestEntry);
	}

	FLSStoreTalkEntry GuideEntry;
	GuideEntry.Label = LOCTEXT("GuideLabel", "안내");
	GuideEntry.Body = LOCTEXT("GuideBody", "에이베리 보급소에서는 물자 거래와 장비 제작을 지원함.\n필요한 기능을 선택하면 됨.");
	TalkEntries.Add(GuideEntry);

	FLSStoreTalkEntry AboutEntry;
	AboutEntry.Label = LOCTEXT("AboutLabel", "에이베리에 대해서");
	AboutEntry.Body = LOCTEXT("AboutBody", "에이베리 보급소는 기지의 물류를 담당하는 자동화 거점임.\n관리 유닛은 나, CASHIER-9.");
	TalkEntries.Add(AboutEntry);
}

bool ULSStoreWidget::HasPendingQuest() const
{
	for (const FLSStoreTalkEntry& Entry : TalkEntries)
	{
		if (Entry.bQuest)
		{
			return true;
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
