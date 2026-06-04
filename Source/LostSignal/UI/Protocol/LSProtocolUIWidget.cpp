#include "UI/Protocol/LSProtocolUIWidget.h"

#include "Data/LSChipStats.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/Protocol/LSProtocolWidget.h"

void ULSProtocolUIWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshProtocolUI();
}

void ULSProtocolUIWidget::RefreshProtocolUI()
{
	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot refresh protocol UI because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return;
	}

	const FLSChipProtocolTotals ProtocolTotals = LSChipStats::AggregateChipProtocolTotals(SaveSubsystem->GetChipEquipmentSlots(), this);
	if (Protocol_Survival)
	{
		Protocol_Survival->SetProtocol(ProtocolTotals.Survival, ProtocolTotals.Survival);
	}
	if (Protocol_Carrying)
	{
		Protocol_Carrying->SetProtocol(ProtocolTotals.Carrying, ProtocolTotals.Carrying);
	}
	if (Protocol_Battle)
	{
		Protocol_Battle->SetProtocol(ProtocolTotals.Battle, ProtocolTotals.Battle);
	}
	if (Protocol_Navigation)
	{
		Protocol_Navigation->SetProtocol(ProtocolTotals.Navigation, ProtocolTotals.Navigation);
	}
}
