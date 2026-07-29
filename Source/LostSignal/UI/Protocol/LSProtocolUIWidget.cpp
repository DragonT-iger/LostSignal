#include "UI/Protocol/LSProtocolUIWidget.h"

#include "Core/LSPlayerControllerBase.h"
#include "Data/LSChipStats.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/Protocol/LSProtocolWidget.h"

namespace
{
void ApplyProtocolUIWidgetLevels(ULSProtocolWidget* ProtocolWidget, const int32 CurrentLevel, const int32 PreviousLevel)
{
	if (!ProtocolWidget)
	{
		return;
	}

	// 단계 칸은 순번 고정(1~7)이라 현재 레벨 이하 순번까지 해금 표시한다.
	ProtocolWidget->SetProtocolLevels(CurrentLevel, PreviousLevel, CurrentLevel);
}
}

void ULSProtocolUIWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Protocol_Survival)
	{
		Protocol_Survival->SetProtocolType(ELSProtocolType::Survival);
	}
	if (Protocol_Carrying)
	{
		Protocol_Carrying->SetProtocolType(ELSProtocolType::Carrying);
	}
	if (Protocol_Battle)
	{
		Protocol_Battle->SetProtocolType(ELSProtocolType::Battle);
	}
	if (Protocol_Navigation)
	{
		Protocol_Navigation->SetProtocolType(ELSProtocolType::Navigation);
	}

	RefreshProtocolUI();
}

void ULSProtocolUIWidget::RefreshProtocolUI()
{
	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	FLSChipProtocolTotals ProtocolTotals;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot refresh protocol UI because SaveSubsystem is missing on %s."), *GetNameSafe(this));
	}
	else
	{
		ProtocolTotals = LSChipStats::AggregateChipProtocolTotals(SaveSubsystem->GetChipEquipmentSlots(), this);
	}

	if (const ALSPlayerControllerBase* PlayerController = GetOwningPlayer<ALSPlayerControllerBase>())
	{
		if (PlayerController->HasProtocolTestLevel(ELSProtocolType::Survival))
		{
			ProtocolTotals.Survival = PlayerController->GetProtocolTestLevel(ELSProtocolType::Survival);
		}
		if (PlayerController->HasProtocolTestLevel(ELSProtocolType::Carrying))
		{
			ProtocolTotals.Carrying = PlayerController->GetProtocolTestLevel(ELSProtocolType::Carrying);
		}
		if (PlayerController->HasProtocolTestLevel(ELSProtocolType::Battle))
		{
			ProtocolTotals.Battle = PlayerController->GetProtocolTestLevel(ELSProtocolType::Battle);
		}
		if (PlayerController->HasProtocolTestLevel(ELSProtocolType::Navigation))
		{
			ProtocolTotals.Navigation = PlayerController->GetProtocolTestLevel(ELSProtocolType::Navigation);
		}
	}

	ApplyProtocolUIWidgetLevels(Protocol_Survival, ProtocolTotals.Survival, ProtocolTotals.Survival);
	ApplyProtocolUIWidgetLevels(Protocol_Carrying, ProtocolTotals.Carrying, ProtocolTotals.Carrying);
	ApplyProtocolUIWidgetLevels(Protocol_Battle, ProtocolTotals.Battle, ProtocolTotals.Battle);
	ApplyProtocolUIWidgetLevels(Protocol_Navigation, ProtocolTotals.Navigation, ProtocolTotals.Navigation);
}
