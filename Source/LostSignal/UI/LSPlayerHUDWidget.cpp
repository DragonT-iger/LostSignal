#include "UI/LSPlayerHUDWidget.h"

#include "LostSignal.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "UI/Skill/LSSkillBarWidget.h"

void ULSPlayerHUDWidget::InitializeHUDForPawn(APawn* InPawn)
{
	if (!SkillBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize HUD because SkillBar is not bound."), *GetNameSafe(this));
		return;
	}

	ULSPlayerSkillComponent* SkillComponent = InPawn ? InPawn->FindComponentByClass<ULSPlayerSkillComponent>() : nullptr;
	if (!SkillComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize skill bar because pawn skill component is missing. Pawn=%s"),
			*GetNameSafe(this),
			*GetNameSafe(InPawn));
		return;
	}

	SkillBar->InitializeSkillBar(SkillComponent);
}

void ULSPlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!SkillBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required HUD widget binding: SkillBar."), *GetNameSafe(this));
	}
}
