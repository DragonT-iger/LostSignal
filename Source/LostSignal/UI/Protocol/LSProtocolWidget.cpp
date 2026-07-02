#include "UI/Protocol/LSProtocolWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "UI/LSUILayer.h"
#include "UI/Protocol/LSProtocolTooltipWidget.h"

void ULSProtocolWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 디자이너에서도 인스턴스별 지정 텍스처가 바로 보이도록 PreConstruct 에서 적용한다.
	// 미지정이면 WBP 브러시를 그대로 둔다(크기는 WBP 브러시 설정 유지).
	if (ProtocolNameImage && ProtocolNameTexture)
	{
		ProtocolNameImage->SetBrushFromTexture(ProtocolNameTexture, /*bMatchSize=*/false);
	}
}

void ULSProtocolWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ProtocolNameTexture)
	{
		UE_LOG(LogLS, Warning, TEXT("ProtocolNameTexture is not set on %s."), *GetNameSafe(this));
	}

	EnsureHoverHitTestable();
}

void ULSProtocolWidget::NativeDestruct()
{
	HideProtocolTooltip();

	Super::NativeDestruct();
}

void ULSProtocolWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	// 툴팁만 뜨고 상호작용 피드백이 없어, 호버 시 한 줄 전체에 틴트를 입혀 강조한다.
	SetColorAndOpacity(HoveredTint);
	ShowProtocolTooltip();
}

void ULSProtocolWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	SetColorAndOpacity(FLinearColor::White);
	HideProtocolTooltip();
}

FReply ULSProtocolWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 툴팁이 커서를 따라가도록 위치를 갱신한다.
	UpdateTooltipPosition();

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void ULSProtocolWidget::SetProtocol(int32 Level, int32 SynergyStage)
{
	SetProtocolLevels(Level, Level, SynergyStage);
}

void ULSProtocolWidget::SetProtocolLevels(const int32 CurrentLevel, const int32 PreviousLevel, const int32 SynergyStage)
{
	if (LevelText)
	{
		LevelText->SetText(FText::AsNumber(CurrentLevel));
	}
	if (SynergyStageText)
	{
		SynergyStageText->SetText(FText::FromString(BuildSynergyMarkup(SynergyStage)));
	}

	CurrentProtocolLevel = CurrentLevel;
	PreviousProtocolLevel = PreviousLevel;
}

void ULSProtocolWidget::SetProtocolStageCount(const int32 InSynergyStageCount)
{
	SynergyStageCount = FMath::Max(0, InSynergyStageCount);
	SynergyStageLevels.Reset();
}

void ULSProtocolWidget::SetProtocolStageLevels(const TArray<int32>& InSynergyStageLevels)
{
	SynergyStageLevels.Reset();
	for (const int32 StageLevel : InSynergyStageLevels)
	{
		if (StageLevel > 0)
		{
			SynergyStageLevels.AddUnique(StageLevel);
		}
	}

	SynergyStageLevels.Sort();
	SynergyStageCount = SynergyStageLevels.Num();
}

void ULSProtocolWidget::SetProtocolType(const ELSProtocolType InProtocolType)
{
	ProtocolType = InProtocolType;
}

FString ULSProtocolWidget::BuildSynergyMarkup(int32 ActiveStage) const
{
	const int32 Count = FMath::Max(0, SynergyStageCount);
	const int32 Active = FMath::Clamp(ActiveStage, 0, SynergyStageLevels.IsEmpty() ? Count : MAX_int32);

	FString Markup;

	TArray<int32> DisplayLevels;
	if (SynergyStageLevels.IsEmpty())
	{
		for (int32 i = 1; i <= Count; ++i)
		{
			DisplayLevels.Add(i);
		}
	}
	else
	{
		DisplayLevels = SynergyStageLevels;
	}

	TArray<int32> ActiveLevels;
	TArray<int32> InactiveLevels;
	for (const int32 DisplayLevel : DisplayLevels)
	{
		if (DisplayLevel <= Active)
		{
			ActiveLevels.Add(DisplayLevel);
		}
		else
		{
			InactiveLevels.Add(DisplayLevel);
		}
	}

	if (!ActiveLevels.IsEmpty())
	{
		Markup += TEXT("<Bold>");
		for (int32 Index = 0; Index < ActiveLevels.Num(); ++Index)
		{
			if (Index > 0)
			{
				Markup += TEXT("/");
			}
			Markup += FString::FromInt(ActiveLevels[Index]);
		}
		Markup += TEXT("</>");
	}

	if (!InactiveLevels.IsEmpty())
	{
		Markup += TEXT("<Light>");
		for (int32 Index = 0; Index < InactiveLevels.Num(); ++Index)
		{
			if (!ActiveLevels.IsEmpty() || Index > 0)
			{
				Markup += TEXT("/");
			}
			Markup += FString::FromInt(InactiveLevels[Index]);
		}
		Markup += TEXT("</>");
	}

	return Markup;
}

ULSProtocolTooltipWidget* ULSProtocolWidget::CreateProtocolTooltipWidget()
{
	if (!ProtocolTooltipWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("ProtocolTooltipWidgetClass is not set on %s."), *GetNameSafe(this));
		return nullptr;
	}

	ULSProtocolTooltipWidget* ProtocolTooltipWidget = nullptr;
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		ProtocolTooltipWidget = CreateWidget<ULSProtocolTooltipWidget>(OwningPlayer, ProtocolTooltipWidgetClass);
	}
	else if (UWorld* World = GetWorld())
	{
		ProtocolTooltipWidget = CreateWidget<ULSProtocolTooltipWidget>(World, ProtocolTooltipWidgetClass);
	}

	if (!ProtocolTooltipWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to create protocol tooltip widget on %s."), *GetNameSafe(this));
		return nullptr;
	}

	ProtocolTooltipWidget->SetProtocolTooltipLevels(ProtocolType, TooltipIconTexture, CurrentProtocolLevel, PreviousProtocolLevel);
	return ProtocolTooltipWidget;
}

void ULSProtocolWidget::EnsureHoverHitTestable()
{
	// 호버(틴트/툴팁)가 동작하려면 루트가 히트테스트 가능해야 한다.
	UWidget* RootWidget = WidgetTree ? WidgetTree->RootWidget : nullptr;
	if (!RootWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("RootWidget is not set on %s."), *GetNameSafe(this));
		return;
	}

	if (RootWidget->GetVisibility() == ESlateVisibility::HitTestInvisible ||
		RootWidget->GetVisibility() == ESlateVisibility::SelfHitTestInvisible)
	{
		RootWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void ULSProtocolWidget::ShowProtocolTooltip()
{
	// 잔여 툴팁 정리 후 새로 띄운다(현재 레벨 기준으로 매 호버마다 생성).
	HideProtocolTooltip();

	ActiveTooltipWidget = CreateProtocolTooltipWidget();
	if (!ActiveTooltipWidget)
	{
		return;
	}

	// 마우스 입력을 가로채지 않게 하고, 모달 패널 위(툴팁 레이어)에 올린다.
	ActiveTooltipWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	ActiveTooltipWidget->AddToViewport(LSUILayer::Tooltip);
	UpdateTooltipPosition();
}

void ULSProtocolWidget::HideProtocolTooltip()
{
	if (ActiveTooltipWidget)
	{
		ActiveTooltipWidget->RemoveFromParent();
		ActiveTooltipWidget = nullptr;
	}
}

void ULSProtocolWidget::UpdateTooltipPosition()
{
	if (!ActiveTooltipWidget)
	{
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!OwningPlayer || !OwningPlayer->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	// 툴팁 크기를 확정한다. 갓 생성된 위젯은 desired size가 0일 수 있어 레이아웃을 강제로 한 번 돌린다.
	ActiveTooltipWidget->ForceLayoutPrepass();

	// 모든 계산은 픽셀 공간에서 한다. GetMousePosition·GetViewportSize는 픽셀, desired size는 레이아웃 단위라 DPI를 곱해 픽셀로 맞춘다.
	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	const FVector2D TooltipPixelSize = ActiveTooltipWidget->GetDesiredSize() * ViewportScale;

	// 마우스 + 오프셋(커서 오른쪽)으로 두되, 오른쪽/아래로 넘치면 화면 안으로 끌어들인다.
	FVector2D PositionPx = FVector2D(MouseX, MouseY) + TooltipCursorOffset;
	const float MaxX = FMath::Max(0.0f, ViewportSize.X - TooltipPixelSize.X);
	const float MaxY = FMath::Max(0.0f, ViewportSize.Y - TooltipPixelSize.Y);
	PositionPx.X = FMath::Clamp(PositionPx.X, 0.0f, MaxX);
	PositionPx.Y = FMath::Clamp(PositionPx.Y, 0.0f, MaxY);

	// bRemoveDPIScale=true로 픽셀→레이아웃 단위 변환.
	ActiveTooltipWidget->SetPositionInViewport(PositionPx, true);
}
