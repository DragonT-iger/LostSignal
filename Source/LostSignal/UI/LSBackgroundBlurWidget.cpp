// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/LSBackgroundBlurWidget.h"

#include "Components/BackgroundBlur.h"
#include "LostSignal.h"

void ULSBackgroundBlurWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!BlurEffect)
	{
		UE_LOG(LogLS, Warning, TEXT("BlurEffect is not bound on %s."), *GetNameSafe(this));
	}
}
