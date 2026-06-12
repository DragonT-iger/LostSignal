#include "MiniGame/RatSteal/UI/LSRatHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "LostSignal.h"
#include "MiniGame/RatSteal/LSRatGameMode.h"
#include "MiniGame/RatSteal/LSRatInventoryComponent.h"
#include "MiniGame/RatSteal/LSRatPlayer.h"

#define LOCTEXT_NAMESPACE "RatSteal"

namespace
{
	constexpr const TCHAR* TextureRoot = TEXT("/Game/LostSignal/MiniGame/RatSteal/Imported/Textures");

	void SetFontSize(UTextBlock* Text, int32 Size)
	{
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
	}

	/** 원작 1920x1080 절대좌표(중심 기준)를 하단 중앙 앵커 오프셋으로 배치 */
	void PlaceBottom(UCanvasPanelSlot* PanelSlot, float OrigX, float OrigY, float W, float H)
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 1.f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetPosition(FVector2D(OrigX - 960.f, OrigY - 1080.f));
		PanelSlot->SetSize(FVector2D(W, H));
	}
}

UTexture2D* ULSRatHUDWidget::LoadHUDTexture(const TCHAR* AssetName)
{
	const FString Path = FString::Printf(TEXT("%s/%s"), TextureRoot, AssetName);
	UTexture2D* Texture = Cast<UTexture2D>(FSoftObjectPath(Path).TryLoad());
	if (!Texture)
	{
		UE_LOG(LogLS, Warning, TEXT("[RatSteal] HUD 텍스처 로드 실패: %s"), *Path);
	}
	return Texture;
}

void ULSRatHUDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!ScoreText && !TimerText && WidgetTree)
	{
		BuildFallbackLayout();
	}
}

void ULSRatHUDWidget::BuildFallbackLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Canvas;

	// 타이머 — 상단 중앙 (원작은 씬 타이머, 위치 신규)
	TimerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TimerText"));
	SetFontSize(TimerText, 48);
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(TimerText))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.f));
		PanelSlot->SetPosition(FVector2D(0.f, 25.f));
		PanelSlot->SetAutoSize(true);
	}

	BuildScoreSign(Canvas);
	BuildBottomPanel(Canvas);
}

void ULSRatHUDWidget::BuildScoreSign(UCanvasPanel* Canvas)
{
	// 원작 UI_Score (1770, 25) — Sign_Panel 표지판 + "Score : N"
	UImage* Sign = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ScoreSign"));
	if (UTexture2D* SignTexture = LoadHUDTexture(TEXT("UI/Sign_Panel")))
	{
		Sign->SetBrushFromTexture(SignTexture, false);
	}
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Sign))
	{
		PanelSlot->SetAnchors(FAnchors(1.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(1.f, 0.f));
		PanelSlot->SetPosition(FVector2D(-20.f, 15.f));
		PanelSlot->SetSize(FVector2D(280.f, 110.f));
	}

	ScoreText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ScoreText"));
	SetFontSize(ScoreText, 34);
	ScoreText->SetText(FText::Format(LOCTEXT("ScoreFormat", "Score : {0}"), FText::AsNumber(0)));
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(ScoreText))
	{
		PanelSlot->SetAnchors(FAnchors(1.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetPosition(FVector2D(-160.f, 70.f));
		PanelSlot->SetAutoSize(true);
	}
}

void ULSRatHUDWidget::BuildBottomPanel(UCanvasPanel* Canvas)
{
	FrameTexture = LoadHUDTexture(TEXT("UI/Icon_Frame"));
	FrameSelectedTexture = LoadHUDTexture(TEXT("UI/Icon_Frame_Selected"));

	// 패널 배경 (원작 Main_Panel (960,930) 1030x250)
	UImage* Panel = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MainPanel"));
	if (UTexture2D* PanelTexture = LoadHUDTexture(TEXT("UI/Main_Panel")))
	{
		Panel->SetBrushFromTexture(PanelTexture, false);
	}
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel))
	{
		PlaceBottom(PanelSlot, 960.f, 930.f, 1030.f, 250.f);
	}

	// 프로필 (엄마 (550,875) / 아기 (550,985), Paper_Frame 110)
	UTexture2D* PaperFrame = LoadHUDTexture(TEXT("UI/Paper_Frame"));
	const struct { const TCHAR* Frame; const TCHAR* Portrait; float Y; } Profiles[] = {
		{ TEXT("MomFrame"), TEXT("UI/mom_profile1"), 875.f },
		{ TEXT("BabyFrame"), TEXT("UI/baby_profile1"), 985.f },
	};
	for (const auto& Profile : Profiles)
	{
		UImage* Frame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Profile.Frame);
		if (PaperFrame)
		{
			Frame->SetBrushFromTexture(PaperFrame, false);
		}
		if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Frame))
		{
			PlaceBottom(PanelSlot, 550.f, Profile.Y, 110.f, 110.f);
		}

		UImage* Portrait = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
			*FString::Printf(TEXT("%s_Portrait"), Profile.Frame));
		if (UTexture2D* PortraitTexture = LoadHUDTexture(Profile.Portrait))
		{
			Portrait->SetBrushFromTexture(PortraitTexture, false);
		}
		if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Portrait))
		{
			PlaceBottom(PanelSlot, 550.f, Profile.Y, 90.f, 90.f);
		}
	}

	// 포만 게이지 (원작 hungryGauge (770,880) — gauge_frame + 게이지)
	UImage* GaugeFrame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("GaugeFrame"));
	if (UTexture2D* GaugeTexture = LoadHUDTexture(TEXT("UI/gauge_frame")))
	{
		GaugeFrame->SetBrushFromTexture(GaugeTexture, false);
	}
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(GaugeFrame))
	{
		PlaceBottom(PanelSlot, 770.f, 880.f, 300.f, 56.f);
	}

	FullnessBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("FullnessBar"));
	FullnessBar->SetPercent(1.f);
	FullnessBar->SetFillColorAndOpacity(FLinearColor(1.f, 0.55f, 0.1f));
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(FullnessBar))
	{
		PlaceBottom(PanelSlot, 770.f, 880.f, 260.f, 26.f);
	}

	// 하트 x3 (원작 (685/770/855, 980) 72x65)
	UTexture2D* HeartTexture = LoadHUDTexture(TEXT("UI/Icon_Heart"));
	TObjectPtr<UImage>* Hearts[] = { &Heart1, &Heart2, &Heart3 };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UImage* Heart = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
			*FString::Printf(TEXT("Heart%d"), Index + 1));
		if (HeartTexture)
		{
			Heart->SetBrushFromTexture(HeartTexture, false);
		}
		else
		{
			Heart->SetColorAndOpacity(FLinearColor::Red);
		}
		if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Heart))
		{
			PlaceBottom(PanelSlot, 685.f + Index * 85.f, 980.f, 72.f, 65.f);
		}
		*Hearts[Index] = Heart;
	}

	// 인벤토리 3슬롯 (원작 (1010/1190/1370, 930) 160x160 + 카운트 텍스트)
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const float SlotX = 1010.f + Index * 180.f;

		UImage* Frame = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
			*FString::Printf(TEXT("SlotFrame%d"), Index + 1));
		if (FrameTexture)
		{
			Frame->SetBrushFromTexture(FrameTexture, false);
		}
		if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Frame))
		{
			PlaceBottom(PanelSlot, SlotX, 930.f, 160.f, 160.f);
		}

		UImage* Item = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),
			*FString::Printf(TEXT("SlotItem%d"), Index + 1));
		Item->SetVisibility(ESlateVisibility::Hidden);
		if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Item))
		{
			PlaceBottom(PanelSlot, SlotX, 930.f, 100.f, 100.f);
		}

		UTextBlock* Count = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			*FString::Printf(TEXT("SlotCount%d"), Index + 1));
		SetFontSize(Count, 24);
		if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Count))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 1.f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetPosition(FVector2D(SlotX - 960.f + 50.f, 930.f - 1080.f + 50.f));
			PanelSlot->SetAutoSize(true);
		}

		SlotWidgets[Index] = { Item ? Frame : nullptr, Item, Count };
		SlotWidgets[Index].Frame = Frame;
	}
}

void ULSRatHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ALSRatGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSRatGameMode>())
	{
		GameMode->OnScoreChanged.AddDynamic(this, &ULSRatHUDWidget::HandleScoreChanged);
		HandleScoreChanged(GameMode->GetTotalScore(), 0);
	}

	if (ALSRatPlayer* Player = Cast<ALSRatPlayer>(GetOwningPlayerPawn()))
	{
		Player->OnHpChanged.AddDynamic(this, &ULSRatHUDWidget::HandleHpChanged);
		Player->OnFullnessChanged.AddDynamic(this, &ULSRatHUDWidget::HandleFullnessChanged);
		HandleHpChanged(Player->GetHp());
		HandleFullnessChanged(Player->GetFullness(), Player->GetMaxFullness());

		if (ULSRatInventoryComponent* Inventory = Player->GetInventory())
		{
			Inventory->OnInventoryChanged.AddDynamic(this, &ULSRatHUDWidget::HandleInventoryChanged);
			HandleInventoryChanged();
		}
	}
}

void ULSRatHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const ALSRatGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSRatGameMode>();
	if (TimerText && GameMode)
	{
		const int32 Remaining = FMath::CeilToInt(GameMode->GetRemainingTime());
		TimerText->SetText(FText::Format(
			LOCTEXT("TimerFormat", "{0}:{1}"),
			FText::AsNumber(Remaining / 60),
			FText::FromString(FString::Printf(TEXT("%02d"), Remaining % 60))));
	}
}

void ULSRatHUDWidget::HandleScoreChanged(int32 TotalScore, int32 DeltaScore)
{
	if (ScoreText)
	{
		ScoreText->SetText(FText::Format(LOCTEXT("ScoreFormat", "Score : {0}"), FText::AsNumber(TotalScore)));
	}
}

void ULSRatHUDWidget::HandleHpChanged(int32 NewHp)
{
	const TObjectPtr<UImage> Hearts[] = { Heart1, Heart2, Heart3 };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Hearts); ++Index)
	{
		if (Hearts[Index])
		{
			Hearts[Index]->SetVisibility(Index < NewHp ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		}
	}
}

void ULSRatHUDWidget::HandleFullnessChanged(float Fullness, float MaxFullness)
{
	if (FullnessBar && MaxFullness > 0.f)
	{
		FullnessBar->SetPercent(Fullness / MaxFullness);
	}
}

UTexture2D* ULSRatHUDWidget::GetItemTexture(ELSRatCropType Type) const
{
	switch (Type)
	{
	case ELSRatCropType::Eggplant: return LoadHUDTexture(TEXT("Crops/eggplant_item"));
	case ELSRatCropType::Potato:   return LoadHUDTexture(TEXT("Crops/potato_item"));
	case ELSRatCropType::Pumpkin:  return LoadHUDTexture(TEXT("Crops/pumpkin_item"));
	default:                       return nullptr;
	}
}

void ULSRatHUDWidget::HandleInventoryChanged()
{
	const ALSRatPlayer* Player = Cast<ALSRatPlayer>(GetOwningPlayerPawn());
	const ULSRatInventoryComponent* Inventory = Player ? Player->GetInventory() : nullptr;
	if (!Inventory)
	{
		return;
	}

	const TArray<FLSRatSlotData>& Slots = Inventory->GetSlots();
	const int32 CurrentIndex = Inventory->GetCurrentSlotIndex();

	for (int32 Index = 0; Index < 3; ++Index)
	{
		FSlotWidgets& Widgets = SlotWidgets[Index];
		if (!Widgets.Frame || !Widgets.Item || !Widgets.Count)
		{
			continue; // WBP 바인딩 모드에서는 폴백 슬롯이 없음
		}

		// 선택 슬롯 하이라이트 (원작 Icon_Frame_Selected)
		UTexture2D* FrameTex = (Index == CurrentIndex && FrameSelectedTexture) ? FrameSelectedTexture : FrameTexture;
		if (FrameTex)
		{
			Widgets.Frame->SetBrushFromTexture(FrameTex, false);
		}

		const bool bHasItem = Slots.IsValidIndex(Index) && !Slots[Index].IsEmpty();
		if (bHasItem)
		{
			if (UTexture2D* ItemTexture = GetItemTexture(Slots[Index].Type))
			{
				Widgets.Item->SetBrushFromTexture(ItemTexture, false);
			}
			Widgets.Item->SetVisibility(ESlateVisibility::Visible);
			Widgets.Count->SetText(FText::AsNumber(Slots[Index].Count));
		}
		else
		{
			Widgets.Item->SetVisibility(ESlateVisibility::Hidden);
			Widgets.Count->SetText(FText::GetEmpty());
		}
	}
}

#undef LOCTEXT_NAMESPACE
