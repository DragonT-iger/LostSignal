#include "MiniGame/RatSteal/UI/LSRatPauseWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "MiniGame/RatSteal/LSRatGameMode.h"
#include "MiniGame/RatSteal/LSRatStealSubsystem.h"

#define LOCTEXT_NAMESPACE "RatSteal"

void ULSRatPauseWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// WBP 없이 C++ 클래스로 직접 생성된 경우 기본 레이아웃 구성
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Canvas;

	UImage* Dim = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DimBackground"));
	Dim->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Dim))
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		PanelSlot->SetOffsets(FMargin(0.f));
	}

	auto AddText = [this, Canvas](const FName Name, const FText& Value, float OffsetY, int32 FontSize)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		Text->SetText(Value);
		Text->SetJustification(ETextJustify::Center);
		if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Text))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetPosition(FVector2D(0.f, OffsetY));
			PanelSlot->SetAutoSize(true);
		}
	};

	AddText(TEXT("PauseTitleText"), LOCTEXT("PauseTitle", "일시정지"), -60.f, 64);
	AddText(TEXT("PauseHintText"), LOCTEXT("PauseHint", "Esc : 계속하기"), 60.f, 32);
}

void ULSRatPauseWidget::ResumeGame()
{
	if (ALSRatGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSRatGameMode>())
	{
		GameMode->TogglePause();
	}
	RemoveFromParent();
}

void ULSRatPauseWidget::RestartGame()
{
	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

void ULSRatPauseWidget::QuitToMainWorld()
{
	UGameplayStatics::SetGamePaused(this, false);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULSRatStealSubsystem* Subsystem = GI->GetSubsystem<ULSRatStealSubsystem>())
		{
			Subsystem->ReturnToMainWorld();
		}
	}
}

#undef LOCTEXT_NAMESPACE
