#include "UI/Protocol/LSProtocolUIWidget.h"

#include "Core/LSPlayerControllerBase.h"
#include "Data/LSChipStats.h"
#include "Data/LSGameDataSubsystem.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/Protocol/LSProtocolWidget.h"

namespace
{
void SetProtocolWidgetFromData(
	ULSProtocolWidget* ProtocolWidget,
	const ELSProtocolType ProtocolType,
	const int32 CurrentLevel,
	const int32 PreviousLevel,
	UGameInstance* GameInstance)
{
	if (!ProtocolWidget)
	{
		return;
	}

	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	TArray<int32> StageLevels;
	if (GameDataSubsystem)
	{
		GameDataSubsystem->GetProtocolRequiredLevels(ProtocolType, StageLevels, TEXT("ProtocolUI"));
	}

	if (!StageLevels.IsEmpty())
	{
		ProtocolWidget->SetProtocolStageLevels(StageLevels);
	}
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

	SetProtocolWidgetFromData(Protocol_Survival, ELSProtocolType::Survival, ProtocolTotals.Survival, ProtocolTotals.Survival, GameInstance);
	SetProtocolWidgetFromData(Protocol_Carrying, ELSProtocolType::Carrying, ProtocolTotals.Carrying, ProtocolTotals.Carrying, GameInstance);
	SetProtocolWidgetFromData(Protocol_Battle, ELSProtocolType::Battle, ProtocolTotals.Battle, ProtocolTotals.Battle, GameInstance);
	SetProtocolWidgetFromData(Protocol_Navigation, ELSProtocolType::Navigation, ProtocolTotals.Navigation, ProtocolTotals.Navigation, GameInstance);
}
