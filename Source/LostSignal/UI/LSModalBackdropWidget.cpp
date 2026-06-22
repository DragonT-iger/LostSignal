// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/LSModalBackdropWidget.h"

#include "Components/BackgroundBlur.h"
#include "LostSignal.h"

void ULSModalBackdropWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!BackdropBlur)
	{
		UE_LOG(LogLS, Warning, TEXT("BackdropBlur is not bound on %s."), *GetNameSafe(this));
	}
}
