#include "UI/Combat/LSDamageNumberWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"

void ULSDamageNumberWidget::ShowDamageNumber(const FLSDamageNumberPayload& Payload)
{
	if (Payload.DamageAmount <= 0.0f)
	{
		HideDamageNumber();
		return;
	}

	ActiveWorldLocation = Payload.WorldLocation;
	ActiveDurationSeconds = FMath::Max(DurationSeconds, KINDA_SMALL_NUMBER);
	ActiveElapsedSeconds = 0.0f;
	RandomScreenOffset = FVector2D(FMath::FRandRange(-RandomHorizontalSpread, RandomHorizontalSpread), 0.0f);
	bActive = true;

	if (DamageText)
	{
		DamageText->SetText(FText::AsNumber(FMath::RoundToInt(Payload.DamageAmount)));
		DamageText->SetColorAndOpacity(Payload.bCritical ? CriticalDamageColor : NormalDamageColor);
	}

	SetDesiredSizeInViewport(WidgetDesiredSize);
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	RefreshDamageNumber(1.0f);
}

void ULSDamageNumberWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!DamageText)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required damage number binding: DamageText."), *GetNameSafe(this));
	}

	HideDamageNumber();
}

void ULSDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bActive)
	{
		return;
	}

	ActiveElapsedSeconds += InDeltaTime;
	const float LifeAlpha = 1.0f - FMath::Clamp(ActiveElapsedSeconds / ActiveDurationSeconds, 0.0f, 1.0f);
	if (LifeAlpha <= 0.0f)
	{
		HideDamageNumber();
		return;
	}

	RefreshDamageNumber(LifeAlpha);
}

float ULSDamageNumberWidget::GetRemainingLifeSeconds() const
{
	return bActive ? FMath::Max(ActiveDurationSeconds - ActiveElapsedSeconds, 0.0f) : 0.0f;
}

void ULSDamageNumberWidget::HideDamageNumber()
{
	bActive = false;
	ActiveElapsedSeconds = 0.0f;
	SetRenderOpacity(0.0f);
	SetVisibility(ESlateVisibility::Collapsed);
}

void ULSDamageNumberWidget::RefreshDamageNumber(const float LifeAlpha)
{
	FVector2D WidgetPosition = FVector2D::ZeroVector;
	if (!ProjectDamageLocation(WidgetPosition))
	{
		SetRenderOpacity(0.0f);
		return;
	}

	const float RiseAlpha = 1.0f - LifeAlpha;
	WidgetPosition += RandomScreenOffset;
	WidgetPosition.Y += FMath::Lerp(StartVerticalOffset, EndVerticalOffset, RiseAlpha);

	SetPositionInViewport(WidgetPosition, false);
	SetRenderOpacity(LifeAlpha);
}

bool ULSDamageNumberWidget::ProjectDamageLocation(FVector2D& OutWidgetPosition) const
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return false;
	}

	if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, ActiveWorldLocation, OutWidgetPosition, true))
	{
		return false;
	}

	FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	const float ViewportScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), KINDA_SMALL_NUMBER);
	ViewportSize /= ViewportScale;
	if (ViewportSize.X <= KINDA_SMALL_NUMBER || ViewportSize.Y <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OutWidgetPosition.X = FMath::Clamp(OutWidgetPosition.X, ScreenPadding, ViewportSize.X - ScreenPadding);
	OutWidgetPosition.Y = FMath::Clamp(OutWidgetPosition.Y, ScreenPadding, ViewportSize.Y - ScreenPadding);
	return true;
}
