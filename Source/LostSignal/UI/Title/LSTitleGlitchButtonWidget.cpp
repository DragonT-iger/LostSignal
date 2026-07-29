#include "UI/Title/LSTitleGlitchButtonWidget.h"

#include "Components/Button.h"
#include "Components/RetainerBox.h"
#include "LostSignal.h"
#include "Materials/MaterialInstanceDynamic.h"

void ULSTitleGlitchButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitializeGlitchMaterial();
	if (Button)
	{
		Button->OnHovered.AddDynamic(this, &ULSTitleGlitchButtonWidget::HandleGlitchHovered);
		Button->OnUnhovered.AddDynamic(this, &ULSTitleGlitchButtonWidget::HandleGlitchUnhovered);
	}

	UpdateGlitchInteractionState();
}

void ULSTitleGlitchButtonWidget::NativeDestruct()
{
	if (Button)
	{
		Button->OnHovered.RemoveDynamic(this, &ULSTitleGlitchButtonWidget::HandleGlitchHovered);
		Button->OnUnhovered.RemoveDynamic(this, &ULSTitleGlitchButtonWidget::HandleGlitchUnhovered);
	}

	bMouseHovered = false;
	bFocusWithin = false;
	GlitchMaterialInstance = nullptr;
	Super::NativeDestruct();
}

void ULSTitleGlitchButtonWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);

	bFocusWithin = true;
	UpdateGlitchInteractionState();
}

void ULSTitleGlitchButtonWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);

	bFocusWithin = false;
	UpdateGlitchInteractionState();
}

void ULSTitleGlitchButtonWidget::HandleGlitchHovered()
{
	bMouseHovered = true;
	UpdateGlitchInteractionState();
}

void ULSTitleGlitchButtonWidget::HandleGlitchUnhovered()
{
	bMouseHovered = false;
	UpdateGlitchInteractionState();
}

void ULSTitleGlitchButtonWidget::InitializeGlitchMaterial()
{
	if (!GlitchRetainer)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] GlitchRetainer is not bound on %s."), *GetNameSafe(this));
		return;
	}

	GlitchMaterialInstance = GlitchRetainer->GetEffectMaterial();
	if (!GlitchMaterialInstance)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] Glitch effect material is not set on %s."), *GetNameSafe(this));
		return;
	}

	const uint32 WidgetNameHash = GetTypeHash(GetFName());
	const float RandomSeed = 1.0f + static_cast<float>(WidgetNameHash % 10000u) / 137.0f;
	GlitchMaterialInstance->SetScalarParameterValue(TEXT("RandomSeed"), RandomSeed);
}

void ULSTitleGlitchButtonWidget::UpdateGlitchInteractionState()
{
	if (!GlitchMaterialInstance)
	{
		return;
	}

	const bool bInteractionActive = GetIsEnabled() && (bMouseHovered || bFocusWithin);
	GlitchMaterialInstance->SetScalarParameterValue(
		HoverAmountParameterName,
		bInteractionActive ? 1.0f : 0.0f);

	if (GlitchRetainer)
	{
		GlitchRetainer->RequestRender();
	}
}
