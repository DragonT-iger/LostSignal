#include "UI/Protocol/LSProtocolWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Widget.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "UI/LSUILayer.h"
#include "UI/Protocol/LSProtocolStageWidget.h"
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

	ApplyProtocolBorderColor(ProtocolBorderColor);
}

void ULSProtocolWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ProtocolBorder)
	{
		UE_LOG(LogLS, Warning, TEXT("ProtocolBorder is not bound on %s."), *GetNameSafe(this));
	}

	if (!ProtocolNameTexture)
	{
		UE_LOG(LogLS, Warning, TEXT("ProtocolNameTexture is not set on %s."), *GetNameSafe(this));
	}

	const TArray<ULSProtocolStageWidget*> StageWidgets = GetProtocolStageWidgets();
	for (int32 Index = 0; Index < StageWidgets.Num(); ++Index)
	{
		if (!StageWidgets[Index])
		{
			UE_LOG(LogLS, Warning, TEXT("ProtocolStage_%d is not bound on %s."), Index + 1, *GetNameSafe(this));
		}
	}

	ApplyProtocolBorderColor(ProtocolBorderColor);
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

	// 툴팁만 뜨고 상호작용 피드백이 없어, 호버 시 한 줄 전체를 강조한다.
	// 위젯 ColorAndOpacity 는 자식 색에 곱해지므로, RGB 에 균일한 배수만 넣어 원래 색조를 유지한다.
	SetColorAndOpacity(FLinearColor(HoveredColorMultiplier, HoveredColorMultiplier, HoveredColorMultiplier, 1.0f));
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
	RefreshProtocolStages(SynergyStage);

	CurrentProtocolLevel = CurrentLevel;
	PreviousProtocolLevel = PreviousLevel;
}

void ULSProtocolWidget::SetProtocolType(const ELSProtocolType InProtocolType)
{
	ProtocolType = InProtocolType;
}

void ULSProtocolWidget::SetProtocolBorderColor(const FLinearColor& InColor)
{
	ProtocolBorderColor = InColor;
	ApplyProtocolBorderColor(ProtocolBorderColor);
}

TArray<ULSProtocolStageWidget*> ULSProtocolWidget::GetProtocolStageWidgets() const
{
	return {
		ProtocolStage_1.Get(),
		ProtocolStage_2.Get(),
		ProtocolStage_3.Get(),
		ProtocolStage_4.Get(),
		ProtocolStage_5.Get(),
		ProtocolStage_6.Get(),
		ProtocolStage_7.Get()};
}

void ULSProtocolWidget::RefreshProtocolStages(const int32 UnlockedStageCount) const
{
	const TArray<ULSProtocolStageWidget*> StageWidgets = GetProtocolStageWidgets();
	for (int32 Index = 0; Index < StageWidgets.Num(); ++Index)
	{
		if (ULSProtocolStageWidget* StageWidget = StageWidgets[Index])
		{
			const int32 StageOrder = Index + 1;
			StageWidget->SetProtocolStage(StageOrder, StageOrder <= UnlockedStageCount);
		}
	}
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

	// 툴팁 아이콘은 프로토콜 이름 이미지와 같은 텍스처를 쓴다.
	ProtocolTooltipWidget->SetProtocolTooltipLevels(ProtocolType, ProtocolNameTexture, CurrentProtocolLevel, PreviousProtocolLevel);
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

void ULSProtocolWidget::ApplyProtocolBorderColor(const FLinearColor& InColor) const
{
	if (ProtocolBorder)
	{
		ProtocolBorder->SetBrushColor(InColor);
	}
}
