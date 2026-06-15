#include "UI/Combat/LSEnemyHealthBarWidget.h"

#include "Components/ProgressBar.h"
#include "LostSignal.h"

void ULSEnemyHealthBarWidget::SetHealthPercent(const float Percent)
{
	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	}
}

void ULSEnemyHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!HealthProgressBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required enemy health bar binding: HealthProgressBar."), *GetNameSafe(this));
		return;
	}

	SetHealthPercent(0.0f);
}
