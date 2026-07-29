#include "UI/Storage/LSStorageButtonWidget.h"

#include "Components/Button.h"
#include "LostSignal.h"

void ULSStorageButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// WBP 디자이너 프리뷰에서도 실제 색으로 보이게 한다.
	ApplyButtonColors();
}

void ULSStorageButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!Button)
	{
		UE_LOG(LogLS, Warning, TEXT("Button is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		Button->OnClicked.AddDynamic(this, &ULSStorageButtonWidget::HandleButtonClicked);
	}

	ApplyButtonColors();
}

void ULSStorageButtonWidget::NativeDestruct()
{
	if (Button)
	{
		Button->OnClicked.RemoveDynamic(this, &ULSStorageButtonWidget::HandleButtonClicked);
	}

	Super::NativeDestruct();
}

void ULSStorageButtonWidget::SetSelected(const bool bInSelected)
{
	if (bIsSelected == bInSelected)
	{
		return;
	}

	bIsSelected = bInSelected;
	ApplyButtonColors();
}

void ULSStorageButtonWidget::HandleButtonClicked()
{
	OnClicked.Broadcast();
}

void ULSStorageButtonWidget::ApplyButtonColors() const
{
	if (!Button)
	{
		return;
	}

	// 아트가 WBP에서 잡은 브러시(이미지·라운드박스·모서리)는 유지하고 틴트만 상태별로 덮어쓴다.
	FButtonStyle ButtonStyle = Button->GetStyle();
	ButtonStyle.Normal.TintColor = FSlateColor(bIsSelected ? SelectedColor : NormalColor);
	// 선택된 탭은 호버해도 색이 흔들리지 않도록 선택색으로 고정한다.
	ButtonStyle.Hovered.TintColor = FSlateColor(bIsSelected ? SelectedColor : HoveredColor);
	ButtonStyle.Pressed.TintColor = FSlateColor(PressedColor);
	ButtonStyle.Disabled.TintColor = FSlateColor(NormalColor);
	Button->SetStyle(ButtonStyle);
}
