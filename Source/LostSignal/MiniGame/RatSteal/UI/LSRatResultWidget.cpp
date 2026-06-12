#include "MiniGame/RatSteal/UI/LSRatResultWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "MiniGame/RatSteal/LSRatStealSubsystem.h"

#define LOCTEXT_NAMESPACE "RatSteal"

namespace
{
	UTextBlock* AddCenteredText(UWidgetTree* Tree, UCanvasPanel* Canvas, const FName Name, float OffsetY, int32 FontSize)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		Text->SetJustification(ETextJustify::Center);
		if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Text))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetPosition(FVector2D(0.f, OffsetY));
			PanelSlot->SetAutoSize(true);
		}
		return Text;
	}
}

void ULSRatResultWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!ReasonText && !ScoreText && WidgetTree)
	{
		BuildFallbackLayout();
	}
}

void ULSRatResultWidget::BuildFallbackLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Canvas;

	// 반투명 배경 딤
	UImage* Dim = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DimBackground"));
	Dim->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));
	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Dim))
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		PanelSlot->SetOffsets(FMargin(0.f));
	}

	ReasonText = AddCenteredText(WidgetTree, Canvas, TEXT("ReasonText"), -180.f, 64);
	ScoreText = AddCenteredText(WidgetTree, Canvas, TEXT("ScoreText"), -60.f, 48);
	GradeText = AddCenteredText(WidgetTree, Canvas, TEXT("GradeText"), 30.f, 56);
	CountsText = AddCenteredText(WidgetTree, Canvas, TEXT("CountsText"), 130.f, 36);

	UTextBlock* Hint = AddCenteredText(WidgetTree, Canvas, TEXT("HintText"), 260.f, 28);
	Hint->SetText(LOCTEXT("ResultHint", "Enter / Space : 돌아가기"));
}

void ULSRatResultWidget::SetResult(const FLSRatResult& InResult)
{
	Result = InResult;

	if (!ReasonText || !ScoreText || !GradeText || !CountsText)
	{
		UE_LOG(LogLS, Warning, TEXT("[RatSteal] Result: BindWidget 누락 항목 있음 (WBP 위젯 이름 확인)"));
	}

	if (ReasonText)
	{
		FText Reason;
		switch (Result.EndReason)
		{
		case ELSRatEndReason::TimeUp:      Reason = LOCTEXT("ReasonTimeUp", "3분 생존 성공!"); break;
		case ELSRatEndReason::BabyStarved: Reason = LOCTEXT("ReasonBabyStarved", "아기쥐가 배고파요..."); break;
		case ELSRatEndReason::PlayerDead:  Reason = LOCTEXT("ReasonPlayerDead", "농부에게 잡혔다!"); break;
		default:                           Reason = FText::GetEmpty(); break;
		}
		ReasonText->SetText(Reason);
	}

	if (ScoreText)
	{
		ScoreText->SetText(FText::Format(LOCTEXT("ScoreFormat", "총점 {0}"), FText::AsNumber(Result.TotalScore)));
	}

	if (GradeText)
	{
		// 생존 종료에만 ★ 표기 (02_Progression — 패배 시 정책 미정)
		if (Result.EndReason == ELSRatEndReason::TimeUp)
		{
			FString Stars;
			for (int32 Index = 0; Index < 3; ++Index)
			{
				Stars += (Index < Result.Stars) ? TEXT("★") : TEXT("☆");
			}
			GradeText->SetText(FText::FromString(Stars));
		}
		else
		{
			GradeText->SetText(FText::GetEmpty());
		}
	}

	if (CountsText)
	{
		CountsText->SetText(FText::Format(
			LOCTEXT("CountsFormat", "가지 {0}  감자 {1}  호박 {2}"),
			FText::AsNumber(Result.EggplantCount),
			FText::AsNumber(Result.PotatoCount),
			FText::AsNumber(Result.PumpkinCount)));
	}

	OnResultSet(Result);
}

void ULSRatResultWidget::ReturnToMainWorld()
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
