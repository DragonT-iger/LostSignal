#include "UI/Inventory/LSItemTooltipWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Data/LSArmorRow.h"
#include "Data/LSChipRow.h"
#include "Data/LSDropSettings.h"
#include "Data/LSItemRow.h"
#include "Data/LSWeaponRow.h"
#include "Engine/DataTable.h"
#include "Core/LSPlayerControllerBase.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
#include "UI/Inventory/LSItemTooltipExtraInfoRowWidget.h"
#include "UI/Inventory/LSItemTooltipStatRowWidget.h"

#define LOCTEXT_NAMESPACE "LSItemTooltipWidget"

void ULSItemTooltipWidget::SetItem(const FName ItemRowName, const int32 HoveredSlotAmount)
{
	ClearStats();
	ClearExtraInfos();

	if (ItemRowName.IsNone())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set item tooltip because ItemRowName is none on %s."), *GetNameSafe(this));
		return;
	}

	const FString RowNameString = ItemRowName.ToString();
	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		PopulateChipTooltip(ItemRowName);
	}
	else if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		PopulateWeaponTooltip(ItemRowName);
	}
	else if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		PopulateArmorTooltip(ItemRowName);
	}
	else if (RowNameString.StartsWith(TEXT("Item_")))
	{
		PopulateItemTooltip(ItemRowName, HoveredSlotAmount);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set item tooltip because row '%s' has an unknown prefix."), *ItemRowName.ToString());
	}
}

void ULSItemTooltipWidget::ClearStats()
{
	if (!StatsBox)
	{
		UE_LOG(LogLS, Warning, TEXT("StatsBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	StatsBox->ClearChildren();
}

void ULSItemTooltipWidget::ClearExtraInfos()
{
	if (!ExtraInfoBox)
	{
		UE_LOG(LogLS, Warning, TEXT("ExtraInfoBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	ExtraInfoBox->ClearChildren();
}

void ULSItemTooltipWidget::AddStat(const FText& StatName, const FText& StatValue)
{
	if (!StatsBox)
	{
		UE_LOG(LogLS, Warning, TEXT("StatsBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (!StatRowWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("StatRowWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot create stat row because world is missing on %s."), *GetNameSafe(this));
		return;
	}

	ULSItemTooltipStatRowWidget* StatRow = CreateWidget<ULSItemTooltipStatRowWidget>(World, StatRowWidgetClass);
	if (!StatRow)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to create tooltip stat row on %s."), *GetNameSafe(this));
		return;
	}

	StatRow->SetStat(StatName, StatValue);
	StatsBox->AddChild(StatRow);
}

void ULSItemTooltipWidget::AddStatIfNonZero(const FText& StatName, const float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}

	AddStat(StatName, FormatNumber(Value));
}

void ULSItemTooltipWidget::AddExtraInfo(const FText& ExtraInfoName, const FText& ExtraInfoValue)
{
	if (!ExtraInfoBox)
	{
		UE_LOG(LogLS, Warning, TEXT("ExtraInfoBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (!ExtraInfoRowWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("ExtraInfoRowWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot create extra info row because world is missing on %s."), *GetNameSafe(this));
		return;
	}

	ULSItemTooltipExtraInfoRowWidget* ExtraInfoRow = CreateWidget<ULSItemTooltipExtraInfoRowWidget>(World, ExtraInfoRowWidgetClass);
	if (!ExtraInfoRow)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to create tooltip extra info row on %s."), *GetNameSafe(this));
		return;
	}

	ExtraInfoRow->SetExtraInfo(ExtraInfoName, ExtraInfoValue);
	ExtraInfoBox->AddChild(ExtraInfoRow);
}

void ULSItemTooltipWidget::SetCommonTexts(const FText& TooltipType, const FText& ItemName, const FString& ItemGrade, const FText& Description, const int32 ItemCost)
{
	if (!TooltipTypeText)
	{
		UE_LOG(LogLS, Warning, TEXT("TooltipTypeText is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		TooltipTypeText->SetText(TooltipType);
	}

	if (!ItemNameText)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemNameText is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		ItemNameText->SetText(ItemName);
	}

	if (!GradeText)
	{
		UE_LOG(LogLS, Warning, TEXT("GradeText is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		GradeText->SetText(GetGradeText(ItemGrade));
	}

	if (!DescriptionText)
	{
		UE_LOG(LogLS, Warning, TEXT("DescriptionText is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		DescriptionText->SetText(NormalizeDescriptionText(Description));
	}

	if (!PriceText)
	{
		UE_LOG(LogLS, Warning, TEXT("PriceText is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		PriceText->SetText(FText::Format(LOCTEXT("PriceFormat", "C{0}"), FText::AsNumber(ItemCost)));
	}
}

void ULSItemTooltipWidget::PopulateChipTooltip(const FName ItemRowName)
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	UDataTable* ChipTable = Settings ? Settings->ChipTable.LoadSynchronous() : nullptr;
	if (!ChipTable)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set chip tooltip because ChipTable is not set."));
		return;
	}

	const FLSChipRow* Row = ChipTable->FindRow<FLSChipRow>(ItemRowName, TEXT("PopulateChipTooltip"));
	if (!Row)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set chip tooltip because row '%s' is missing."), *ItemRowName.ToString());
		return;
	}

	SetCommonTexts(LOCTEXT("ChipTooltipType", "칩 설명창"), Row->Item_Text, Row->Item_Grade, Row->Item_Description, Row->Item_Cost);
	AddExtraInfo(LOCTEXT("ChipMemoryCostExtraInfo", "메모리 할당량"), FText::AsNumber(Row->Item_MemoryCost));

	AddStatIfNonZero(LOCTEXT("ChipStatusCountStat", "전투 스탯 개수"), static_cast<float>(Row->Item_Chip_Status_Count));
}

void ULSItemTooltipWidget::PopulateWeaponTooltip(const FName ItemRowName)
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	UDataTable* WeaponTable = Settings ? Settings->WeaponTable.LoadSynchronous() : nullptr;
	if (!WeaponTable)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set weapon tooltip because WeaponTable is not set."));
		return;
	}

	const FLSWeaponRow* Row = WeaponTable->FindRow<FLSWeaponRow>(ItemRowName, TEXT("PopulateWeaponTooltip"));
	if (!Row)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set weapon tooltip because row '%s' is missing."), *ItemRowName.ToString());
		return;
	}

	SetCommonTexts(LOCTEXT("EquipmentTooltipType", "장비 설명창"), Row->Item_Text, Row->Item_Grade, Row->Item_Description, Row->Item_Cost);
	AddExtraInfo(LOCTEXT("EquipmentExtraInfo", "장착 가능"), GetEquipmentDisplayText(Row->Item_Equipment));
	AddStatIfNonZero(LOCTEXT("AttackStat", "공격력"), Row->Item_Attack);
	AddStatIfNonZero(LOCTEXT("AttackSpeedStat", "공격 속도"), Row->Item_Attack_Speed);
	AddStatIfNonZero(LOCTEXT("SkillHasteStat", "스킬 가속"), Row->Item_Skill_Haste);
	AddStatIfNonZero(LOCTEXT("CriticalRateStat", "치명타 확률"), Row->Item_Critical_Rate);
	AddStatIfNonZero(LOCTEXT("CriticalDamageStat", "치명타 피해"), Row->Item_Critical_Damage);
	AddStatIfNonZero(LOCTEXT("DefensePenetrationStat", "방어 관통"), Row->Item_Defense_Penetration);
}

void ULSItemTooltipWidget::PopulateArmorTooltip(const FName ItemRowName)
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	UDataTable* ArmorTable = Settings ? Settings->ArmorTable.LoadSynchronous() : nullptr;
	if (!ArmorTable)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set armor tooltip because ArmorTable is not set."));
		return;
	}

	const FLSArmorRow* Row = ArmorTable->FindRow<FLSArmorRow>(ItemRowName, TEXT("PopulateArmorTooltip"));
	if (!Row)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set armor tooltip because row '%s' is missing."), *ItemRowName.ToString());
		return;
	}

	SetCommonTexts(LOCTEXT("ArmorTooltipType", "장비 설명창"), Row->Item_Text, Row->Item_Grade, Row->Item_Description, Row->Item_Cost);
	AddExtraInfo(LOCTEXT("ArmorExtraInfo", "장착 가능"), GetEquipmentDisplayText(Row->Item_Equipment));
	AddStatIfNonZero(LOCTEXT("HealthStat", "체력"), Row->Item_Health);
	AddStatIfNonZero(LOCTEXT("DefenseStat", "방어력"), Row->Item_Defense);
	AddStatIfNonZero(LOCTEXT("RecoveryStat", "회복"), Row->Item_Recovery);
}

void ULSItemTooltipWidget::PopulateItemTooltip(const FName ItemRowName, const int32 HoveredSlotAmount)
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	UDataTable* ItemTable = Settings ? Settings->ItemTable.LoadSynchronous() : nullptr;
	if (!ItemTable)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set item tooltip because ItemTable is not set."));
		return;
	}

	const FLSItemRow* Row = ItemTable->FindRow<FLSItemRow>(ItemRowName, TEXT("PopulateItemTooltip"));
	if (!Row)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set item tooltip because row '%s' is missing."), *ItemRowName.ToString());
		return;
	}

	int32 CurrentCount = HoveredSlotAmount;
	if (const ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		if (const ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent())
		{
			CurrentCount = CountItems(RaidInventory->GetSessionInventory(), ItemRowName);
			CurrentCount += CountItems(RaidInventory->GetSessionSafeInventory(), ItemRowName);
		}
	}

	SetCommonTexts(LOCTEXT("ItemTooltipType", "일반 아이템 설명창"), Row->Item_Text, Row->Item_Grade, Row->Item_Description, Row->Item_Cost);
	AddExtraInfo(LOCTEXT("CurrentItemCountExtraInfo", "현재 아이템 개수"), FText::AsNumber(CurrentCount));
	AddExtraInfo(LOCTEXT("StashItemCountExtraInfo", "창고 아이템 개수"), FText::AsNumber(0));
}

FText ULSItemTooltipWidget::GetGradeText(const FString& ItemGrade)
{
	if (ItemGrade == TEXT("Supply"))       return LOCTEXT("GradeSupply", "보급");
	if (ItemGrade == TEXT("Standard"))     return LOCTEXT("GradeStandard", "표준");
	if (ItemGrade == TEXT("Presision"))    return LOCTEXT("GradePrecision", "정밀");
	if (ItemGrade == TEXT("Tuning"))       return LOCTEXT("GradeTuning", "튜닝");
	if (ItemGrade == TEXT("Prototype"))    return LOCTEXT("GradePrototype", "프로토타입");
	if (ItemGrade == TEXT("Masterpiece"))  return LOCTEXT("GradeMasterpiece", "마스터피스");

	UE_LOG(LogLS, Warning, TEXT("Unknown item grade '%s'."), *ItemGrade);
	return FText::FromString(ItemGrade);
}

FText ULSItemTooltipWidget::GetEquipmentDisplayText(const FString& EquipmentName)
{
	if (EquipmentName == TEXT("Weapon_1")) return LOCTEXT("EquipmentWeapon1", "1번 무기");
	if (EquipmentName == TEXT("Weapon_2")) return LOCTEXT("EquipmentWeapon2", "2번 무기");
	if (EquipmentName == TEXT("Weapon_3")) return LOCTEXT("EquipmentWeapon3", "방패");
	if (EquipmentName == TEXT("Processor")) return LOCTEXT("EquipmentProcessor", "프로세서");
	if (EquipmentName == TEXT("Core")) return LOCTEXT("EquipmentCore", "코어");
	if (EquipmentName == TEXT("Actuator")) return LOCTEXT("EquipmentActuator", "구동계");
	if (EquipmentName == TEXT("Frame")) return LOCTEXT("EquipmentFrame", "프레임");

	UE_LOG(LogLS, Warning, TEXT("Unknown equipment display name '%s'."), *EquipmentName);
	return FText::FromString(EquipmentName);
}

FText ULSItemTooltipWidget::NormalizeDescriptionText(const FText& Description)
{
	FString DescriptionString = Description.ToString();
	DescriptionString.ReplaceInline(TEXT("\\r\\n"), TEXT("\n"));
	DescriptionString.ReplaceInline(TEXT("\\n"), TEXT("\n"));
	return FText::FromString(DescriptionString);
}

FText ULSItemTooltipWidget::FormatNumber(const float Value)
{
	if (FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value)))
	{
		return FText::AsNumber(FMath::RoundToInt(Value));
	}

	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 0;
	Options.MaximumFractionalDigits = 2;
	return FText::AsNumber(Value, &Options);
}

int32 ULSItemTooltipWidget::CountItems(const TArray<FLSSessionItem>& Items, const FName ItemRowName)
{
	int32 Count = 0;
	for (const FLSSessionItem& Item : Items)
	{
		if (Item.ItemRowName == ItemRowName)
		{
			Count += Item.Amount;
		}
	}
	return Count;
}

#undef LOCTEXT_NAMESPACE
