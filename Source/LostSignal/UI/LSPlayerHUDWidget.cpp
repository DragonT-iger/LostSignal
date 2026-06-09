#include "UI/LSPlayerHUDWidget.h"

#include "LostSignal.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "UI/Minimap/LSMinimapWidget.h"
#include "UI/Skill/LSSkillBarWidget.h"
#include "UI/Survival/LSSurvivalStatusWidget.h"

void ULSPlayerHUDWidget::InitializeHUDForPawn(APawn* InPawn)
{
	if (!SkillBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize HUD because SkillBar is not bound."), *GetNameSafe(this));
	}

	if (!Minimap)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize HUD because Minimap is not bound."), *GetNameSafe(this));
	}
	else
	{
		Minimap->InitializeMinimapForPawn(InPawn);
	}

	if (!SurvivalStatus)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize HUD because SurvivalStatus is not bound."), *GetNameSafe(this));
	}
	else
	{
		SurvivalStatus->InitializeSurvivalStatusForPawn(InPawn);
	}

	ULSPlayerSkillComponent* SkillComponent = InPawn ? InPawn->FindComponentByClass<ULSPlayerSkillComponent>() : nullptr;
	if (!SkillComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize skill bar because pawn skill component is missing. Pawn=%s"),
			*GetNameSafe(this),
			*GetNameSafe(InPawn));
		return;
	}

	if (SkillBar)
	{
		SkillBar->InitializeSkillBar(SkillComponent);
	}
}

void ULSPlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!SkillBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required HUD widget binding: SkillBar."), *GetNameSafe(this));
	}
	if (!Minimap)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required HUD widget binding: Minimap."), *GetNameSafe(this));
	}
	if (!SurvivalStatus)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required HUD widget binding: SurvivalStatus."), *GetNameSafe(this));
	}
}
