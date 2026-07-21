// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Debug/LSProtocolDebugWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/LSFarmingGameMode.h"
#include "Core/LSLobbyGameMode.h"
#include "Core/LSPlayerControllerBase.h"
#include "Engine/GameInstance.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"

#define LOCTEXT_NAMESPACE "LSProtocolDebugWidget"

TSharedRef<SWidget> ULSProtocolDebugWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildPanel();
	}

	return Super::RebuildWidget();
}

void ULSProtocolDebugWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshLevelTexts();
	UpdateEndRaidVisibility();
}

void ULSProtocolDebugWidget::BuildPanel()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));

	UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	RootBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.7f));
	RootBorder->SetPadding(FMargin(14.f));

	UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootVBox"));
	RootBorder->SetContent(VBox);

	// 타이틀
	UTextBlock* Title = MakeText(TEXT("PROTOCOL DEBUG  (Insert)"), 18);
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.85f, 0.2f)));
	if (UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
	}

	// Clear / Max 행
	UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActionRow"));

	UButton* ClearButton = MakeButton(TEXT("Clear"), 16);
	ClearButton->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleClear);
	if (UHorizontalBoxSlot* ClearSlot = ActionRow->AddChildToHorizontalBox(ClearButton))
	{
		ClearSlot->SetPadding(FMargin(4.f, 2.f));
	}

	UButton* MaxButton = MakeButton(TEXT("Max (8)"), 16);
	MaxButton->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleMaxAll);
	if (UHorizontalBoxSlot* MaxSlot = ActionRow->AddChildToHorizontalBox(MaxButton))
	{
		MaxSlot->SetPadding(FMargin(4.f, 2.f));
	}

	UButton* AddGoldButton = MakeButton(LOCTEXT("AddGoldButton", "Gold +10,000"), 16);
	AddGoldButton->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleAddGold);
	if (UHorizontalBoxSlot* AddGoldSlot = ActionRow->AddChildToHorizontalBox(AddGoldButton))
	{
		AddGoldSlot->SetPadding(FMargin(4.f, 2.f));
	}

	if (UVerticalBoxSlot* ActionSlot = VBox->AddChildToVerticalBox(ActionRow))
	{
		ActionSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		ActionSlot->SetHorizontalAlignment(HAlign_Center);
	}

	// "테스트 맵 가기" 버튼 — 정식 레이드 진입을 타되 목적지만 TestRaidLevel 로. (로비에서만 표시)
	TestMapButton = MakeButton(TEXT("테스트 맵 가기"), 16);
	TestMapButton->SetBackgroundColor(FLinearColor(0.1f, 0.4f, 0.6f, 1.f));
	// 게임뷰포트에 포커스가 있는 로비에서 기본(DownAndUp) 방식은 첫 클릭을 포커스 이동에 쓰고
	// 두 번째 클릭에서야 발동한다. 누르는 즉시 발동하도록 바꿔 한 번 클릭으로 진입시킨다.
	TestMapButton->SetClickMethod(EButtonClickMethod::MouseDown);
	TestMapButton->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleGoToTestMap);
	if (UVerticalBoxSlot* TestMapSlot = VBox->AddChildToVerticalBox(TestMapButton))
	{
		TestMapSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		TestMapSlot->SetHorizontalAlignment(HAlign_Center);
	}

	// "레이드 종료" 버튼 — 레이드 중에만 보인다. (탈출 성공 처리 → 로비 복귀)
	EndRaidButton = MakeButton(TEXT("레이드 종료"), 16);
	EndRaidButton->SetBackgroundColor(FLinearColor(0.6f, 0.1f, 0.1f, 1.f));
	EndRaidButton->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleEndRaid);
	if (UVerticalBoxSlot* EndRaidSlot = VBox->AddChildToVerticalBox(EndRaidButton))
	{
		EndRaidSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		EndRaidSlot->SetHorizontalAlignment(HAlign_Center);
	}

	// 배속 행 — 신호 게이지 60초 드레인만 빠르게 돌려 칩 프로토콜이 사라지는 걸 바로 확인.
	BuildTimeScaleRow(VBox);

	// 프로토콜 4행
	BuildProtocolRow(VBox, ELSProtocolType::Survival, TEXT("Survival (생존)"));
	BuildProtocolRow(VBox, ELSProtocolType::Carrying, TEXT("Carrying (수송)"));
	BuildProtocolRow(VBox, ELSProtocolType::Battle, TEXT("Battle (전투)"));
	BuildProtocolRow(VBox, ELSProtocolType::Navigation, TEXT("Navigation (네비)"));

	if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(RootBorder))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
		CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
		CanvasSlot->SetPosition(FVector2D(40.f, 40.f));
	}

	WidgetTree->RootWidget = Canvas;
}

void ULSProtocolDebugWidget::BuildProtocolRow(UVerticalBox* Parent, const ELSProtocolType Type, const FString& DisplayName)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

	UTextBlock* NameText = MakeText(DisplayName, 16);
	NameText->SetMinDesiredWidth(170.f);
	if (UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(NameText))
	{
		NameSlot->SetPadding(FMargin(4.f, 2.f));
		NameSlot->SetVerticalAlignment(VAlign_Center);
	}

	UButton* MinusButton = MakeButton(TEXT("-"), 18);
	if (UHorizontalBoxSlot* MinusSlot = Row->AddChildToHorizontalBox(MinusButton))
	{
		MinusSlot->SetPadding(FMargin(4.f, 2.f));
		MinusSlot->SetVerticalAlignment(VAlign_Center);
	}

	UTextBlock* LevelText = MakeText(TEXT("0"), 18);
	LevelText->SetMinDesiredWidth(40.f);
	LevelText->SetJustification(ETextJustify::Center);
	if (UHorizontalBoxSlot* LevelSlot = Row->AddChildToHorizontalBox(LevelText))
	{
		LevelSlot->SetPadding(FMargin(8.f, 2.f));
		LevelSlot->SetVerticalAlignment(VAlign_Center);
	}

	UButton* PlusButton = MakeButton(TEXT("+"), 18);
	if (UHorizontalBoxSlot* PlusSlot = Row->AddChildToHorizontalBox(PlusButton))
	{
		PlusSlot->SetPadding(FMargin(4.f, 2.f));
		PlusSlot->SetVerticalAlignment(VAlign_Center);
	}

	switch (Type)
	{
	case ELSProtocolType::Survival:
		SurvivalLevelText = LevelText;
		MinusButton->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleSurvivalMinus);
		PlusButton->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleSurvivalPlus);
		break;
	case ELSProtocolType::Carrying:
		CarryingLevelText = LevelText;
		MinusButton->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleCarryingMinus);
		PlusButton->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleCarryingPlus);
		break;
	case ELSProtocolType::Battle:
		BattleLevelText = LevelText;
		MinusButton->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleBattleMinus);
		PlusButton->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleBattlePlus);
		break;
	case ELSProtocolType::Navigation:
		NavigationLevelText = LevelText;
		MinusButton->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleNavigationMinus);
		PlusButton->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleNavigationPlus);
		break;
	default:
		break;
	}

	if (UVerticalBoxSlot* RowSlot = Parent->AddChildToVerticalBox(Row))
	{
		RowSlot->SetPadding(FMargin(0.f, 3.f));
	}
}

void ULSProtocolDebugWidget::BuildTimeScaleRow(UVerticalBox* Parent)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

	UTextBlock* Label = MakeText(TEXT("배속"), 16);
	Label->SetMinDesiredWidth(60.f);
	if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label))
	{
		LabelSlot->SetPadding(FMargin(4.f, 2.f));
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}

	auto AddButton = [&](const FString& InLabel) -> UButton*
	{
		UButton* Button = MakeButton(InLabel, 16);
		if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Button))
		{
			Slot->SetPadding(FMargin(3.f, 2.f));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
		return Button;
	};

	AddButton(TEXT("x1"))->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleTimeScale1x);
	AddButton(TEXT("x2"))->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleTimeScale2x);
	AddButton(TEXT("x5"))->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleTimeScale5x);
	AddButton(TEXT("x10"))->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleTimeScale10x);
	AddButton(TEXT("x20"))->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleTimeScale20x);
	AddButton(TEXT("x30"))->OnClicked.AddDynamic(this, &ULSProtocolDebugWidget::HandleTimeScale30x);

	TimeScaleText = MakeText(TEXT("x1"), 16);
	TimeScaleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.9f, 0.5f)));
	if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(TimeScaleText))
	{
		ValueSlot->SetPadding(FMargin(10.f, 2.f));
		ValueSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* RowSlot = Parent->AddChildToVerticalBox(Row))
	{
		RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		RowSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

void ULSProtocolDebugWidget::ApplyTimeScale(const float Scale)
{
	// 게임 전체가 아니라 신호 게이지 드레인(1분 주기)만 배속한다. 레이드(ALSFarmingGameMode)에서만 유효.
	ALSFarmingGameMode* FarmingGameMode = Cast<ALSFarmingGameMode>(UGameplayStatics::GetGameMode(this));
	if (!FarmingGameMode)
	{
		if (TimeScaleText)
		{
			TimeScaleText->SetText(FText::FromString(TEXT("- (레이드 아님)")));
		}
		UE_LOG(LogLS, Warning, TEXT("[ProtocolDebug] 배속은 레이드(ALSFarmingGameMode)에서만 적용됩니다."));
		return;
	}

	FarmingGameMode->SetSignalGaugeDrainDebugSpeed(Scale);
	if (TimeScaleText)
	{
		const float Applied = FarmingGameMode->GetSignalGaugeDrainDebugSpeed();
		TimeScaleText->SetText(FText::FromString(FString::Printf(TEXT("x%g"), static_cast<double>(Applied))));
	}
}

UButton* ULSProtocolDebugWidget::MakeButton(const FString& Label, const int32 FontSize)
{
	return MakeButton(FText::FromString(Label), FontSize);
}

UButton* ULSProtocolDebugWidget::MakeButton(const FText& Label, const int32 FontSize)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();

	UTextBlock* Text = MakeText(Label, FontSize);
	Text->SetJustification(ETextJustify::Center);
	Button->AddChild(Text);

	return Button;
}

UTextBlock* ULSProtocolDebugWidget::MakeText(const FString& InText, const int32 FontSize)
{
	return MakeText(FText::FromString(InText), FontSize);
}

UTextBlock* ULSProtocolDebugWidget::MakeText(const FText& InText, const int32 FontSize)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(InText);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));

	FSlateFontInfo Font = Text->GetFont();
	Font.Size = FontSize;
	Text->SetFont(Font);

	return Text;
}

ALSPlayerControllerBase* ULSProtocolDebugWidget::ResolvePC() const
{
	return Cast<ALSPlayerControllerBase>(GetOwningPlayer());
}

int32 ULSProtocolDebugWidget::GetDisplayLevel(const ELSProtocolType Type) const
{
	// 오버라이드가 없으면 0이 아니라 현재 장착 칩 기반 레벨을 보여준다. (첫 오픈 시 실제 값 표시)
	if (const ALSPlayerControllerBase* PC = ResolvePC())
	{
		return FMath::Max(PC->GetEffectiveProtocolLevel(Type), 0);
	}

	return 0;
}

void ULSProtocolDebugWidget::ApplyLevel(const ELSProtocolType Type, const int32 NewLevel)
{
	ALSPlayerControllerBase* PC = ResolvePC();
	if (!PC)
	{
		return;
	}

	switch (Type)
	{
	case ELSProtocolType::Survival:
		PC->LSTestSurvivalProtocol(NewLevel);
		break;
	case ELSProtocolType::Carrying:
		PC->LSTestCarryingProtocol(NewLevel);
		break;
	case ELSProtocolType::Battle:
		PC->LSTestBattleProtocol(NewLevel);
		break;
	case ELSProtocolType::Navigation:
		PC->LSTestNavigationProtocol(NewLevel);
		break;
	default:
		break;
	}
}

void ULSProtocolDebugWidget::AdjustLevel(const ELSProtocolType Type, const int32 Delta)
{
	const int32 NewLevel = FMath::Clamp(GetDisplayLevel(Type) + Delta, 0, MaxProtocolLevel);
	ApplyLevel(Type, NewLevel);
	RefreshLevelTexts();
}

void ULSProtocolDebugWidget::RefreshLevelTexts()
{
	if (SurvivalLevelText)
	{
		SurvivalLevelText->SetText(FText::FromString(FString::FromInt(GetDisplayLevel(ELSProtocolType::Survival))));
	}
	if (CarryingLevelText)
	{
		CarryingLevelText->SetText(FText::FromString(FString::FromInt(GetDisplayLevel(ELSProtocolType::Carrying))));
	}
	if (BattleLevelText)
	{
		BattleLevelText->SetText(FText::FromString(FString::FromInt(GetDisplayLevel(ELSProtocolType::Battle))));
	}
	if (NavigationLevelText)
	{
		NavigationLevelText->SetText(FText::FromString(FString::FromInt(GetDisplayLevel(ELSProtocolType::Navigation))));
	}

	// 패널을 다시 열 때마다 레이드 상태가 바뀌었을 수 있으므로 종료 버튼 표시를 재평가한다.
	UpdateEndRaidVisibility();
}

bool ULSProtocolDebugWidget::IsRaidActive() const
{
	if (const ALSPlayerControllerBase* PC = ResolvePC())
	{
		if (const ULSRaidInventoryComponent* RaidInventory = PC->GetRaidInventoryComponent())
		{
			return RaidInventory->IsRaidActive();
		}
	}

	return false;
}

void ULSProtocolDebugWidget::UpdateEndRaidVisibility()
{
	if (EndRaidButton)
	{
		EndRaidButton->SetVisibility(IsRaidActive() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	// 테스트 맵 진입은 정식 진입과 동일하게 로비에서만 가능하므로 로비에서만 보인다.
	if (TestMapButton)
	{
		TestMapButton->SetVisibility(IsLobbyActive() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

bool ULSProtocolDebugWidget::IsLobbyActive() const
{
	return Cast<ALSLobbyGameMode>(UGameplayStatics::GetGameMode(this)) != nullptr;
}

void ULSProtocolDebugWidget::HandleEndRaid()
{
	// 레이드 중이 아니면 무시한다. (로비 등에서 오작동 방지)
	if (!IsRaidActive())
	{
		return;
	}

	if (ALSFarmingGameMode* FarmingGameMode = Cast<ALSFarmingGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		FarmingGameMode->OnExtraction();
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[ProtocolDebug] 레이드 종료 실패: ALSFarmingGameMode 를 찾지 못했습니다."));
	}
}

void ULSProtocolDebugWidget::HandleGoToTestMap()
{
	// 정식 레이드 진입 시퀀스를 그대로 타되 목적지만 테스트 레벨로 바꾼다(레이드 세션·세이브 동일).
	// 로비에서만 가능 — 로드아웃 제출 등 진입 셋업이 로비 게임모드 권한에서 이뤄지기 때문.
	if (ALSLobbyGameMode* LobbyGameMode = Cast<ALSLobbyGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		LobbyGameMode->StartRaidToTestLevel();
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[ProtocolDebug] 테스트 맵 이동은 로비에서만 가능합니다. (ALSLobbyGameMode 아님)"));
	}
}

void ULSProtocolDebugWidget::HandleTimeScale1x()
{
	ApplyTimeScale(1.0f);
}

void ULSProtocolDebugWidget::HandleTimeScale2x()
{
	ApplyTimeScale(2.0f);
}

void ULSProtocolDebugWidget::HandleTimeScale5x()
{
	ApplyTimeScale(5.0f);
}

void ULSProtocolDebugWidget::HandleTimeScale10x()
{
	ApplyTimeScale(10.0f);
}

void ULSProtocolDebugWidget::HandleTimeScale20x()
{
	ApplyTimeScale(20.0f);
}

void ULSProtocolDebugWidget::HandleTimeScale30x()
{
	ApplyTimeScale(30.0f);
}

void ULSProtocolDebugWidget::HandleSurvivalMinus()
{
	AdjustLevel(ELSProtocolType::Survival, -1);
}

void ULSProtocolDebugWidget::HandleSurvivalPlus()
{
	AdjustLevel(ELSProtocolType::Survival, 1);
}

void ULSProtocolDebugWidget::HandleCarryingMinus()
{
	AdjustLevel(ELSProtocolType::Carrying, -1);
}

void ULSProtocolDebugWidget::HandleCarryingPlus()
{
	AdjustLevel(ELSProtocolType::Carrying, 1);
}

void ULSProtocolDebugWidget::HandleBattleMinus()
{
	AdjustLevel(ELSProtocolType::Battle, -1);
}

void ULSProtocolDebugWidget::HandleBattlePlus()
{
	AdjustLevel(ELSProtocolType::Battle, 1);
}

void ULSProtocolDebugWidget::HandleNavigationMinus()
{
	AdjustLevel(ELSProtocolType::Navigation, -1);
}

void ULSProtocolDebugWidget::HandleNavigationPlus()
{
	AdjustLevel(ELSProtocolType::Navigation, 1);
}

void ULSProtocolDebugWidget::HandleClear()
{
	if (ALSPlayerControllerBase* PC = ResolvePC())
	{
		PC->LSClearProtocolTest();
	}
	RefreshLevelTexts();
}

void ULSProtocolDebugWidget::HandleMaxAll()
{
	if (ALSPlayerControllerBase* PC = ResolvePC())
	{
		PC->LSTestAllProtocols(MaxProtocolLevel, MaxProtocolLevel, MaxProtocolLevel, MaxProtocolLevel);
	}
	RefreshLevelTexts();
}

void ULSProtocolDebugWidget::HandleAddGold()
{
	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("[ProtocolDebug] 골드 추가 실패: SaveSubsystem을 찾지 못했습니다."));
		return;
	}

	SaveSubsystem->AddGold(DebugGoldAmount);
}

#undef LOCTEXT_NAMESPACE
