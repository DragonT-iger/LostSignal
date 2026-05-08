#include "UI/Inventory/LSInventoryItemSlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/LSArmorRow.h"
#include "Data/LSChipRow.h"
#include "Data/LSDropSettings.h"
#include "Data/LSItemRow.h"
#include "Data/LSWeaponRow.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "LostSignal.h"

void ULSInventoryItemSlotWidget::SetItem(const FName ItemRowName, const int32 Amount)
{
	if (!ItemIconImage)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemIconImage is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (!AmountText)
	{
		UE_LOG(LogLS, Warning, TEXT("AmountText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (ItemRowName.IsNone())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set item slot because ItemRowName is none on %s."), *GetNameSafe(this));
		ClearItem();
		return;
	}

	UTexture2D* IconTexture = LoadIconTextureByRowName(ItemRowName);
	if (!IconTexture)
	{
		ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	ItemIconImage->SetBrushFromTexture(IconTexture);
	ItemIconImage->SetVisibility(ESlateVisibility::Visible);
	AmountText->SetText(FText::AsNumber(Amount));
	AmountText->SetVisibility(Amount > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void ULSInventoryItemSlotWidget::ClearItem()
{
	if (!ItemIconImage)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemIconImage is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (!AmountText)
	{
		UE_LOG(LogLS, Warning, TEXT("AmountText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	ItemIconImage->SetBrushFromTexture(nullptr);
	ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
	AmountText->SetText(FText::GetEmpty());
	AmountText->SetVisibility(ESlateVisibility::Collapsed);
}

UTexture2D* ULSInventoryItemSlotWidget::LoadIconTextureByRowName(const FName ItemRowName) const
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (!Settings)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot load item icon because LS Drop Settings is missing."));
		return nullptr;
	}

	const FString RowNameString = ItemRowName.ToString();
	FString IconNameOrPath;

	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		UDataTable* ChipTable = Settings->ChipTable.LoadSynchronous();
		if (!ChipTable)
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot load chip icon because ChipTable is not set."));
			return nullptr;
		}

		const FLSChipRow* Row = ChipTable->FindRow<FLSChipRow>(ItemRowName, TEXT("LoadIconTextureByRowName"));
		if (!Row)
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot load chip icon because row '%s' is missing."), *ItemRowName.ToString());
			return nullptr;
		}

		IconNameOrPath = Row->Icon_Path;
	}
	else if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		UDataTable* WeaponTable = Settings->WeaponTable.LoadSynchronous();
		if (!WeaponTable)
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot load weapon icon because WeaponTable is not set."));
			return nullptr;
		}

		const FLSWeaponRow* Row = WeaponTable->FindRow<FLSWeaponRow>(ItemRowName, TEXT("LoadIconTextureByRowName"));
		if (!Row)
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot load weapon icon because row '%s' is missing."), *ItemRowName.ToString());
			return nullptr;
		}

		IconNameOrPath = Row->Icon_Path;
	}
	else if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		UDataTable* ArmorTable = Settings->ArmorTable.LoadSynchronous();
		if (!ArmorTable)
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot load armor icon because ArmorTable is not set."));
			return nullptr;
		}

		const FLSArmorRow* Row = ArmorTable->FindRow<FLSArmorRow>(ItemRowName, TEXT("LoadIconTextureByRowName"));
		if (!Row)
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot load armor icon because row '%s' is missing."), *ItemRowName.ToString());
			return nullptr;
		}

		IconNameOrPath = Row->Icon_Path;
	}
	else if (RowNameString.StartsWith(TEXT("Item_")))
	{
		UDataTable* ItemTable = Settings->ItemTable.LoadSynchronous();
		if (!ItemTable)
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot load item icon because ItemTable is not set."));
			return nullptr;
		}

		const FLSItemRow* Row = ItemTable->FindRow<FLSItemRow>(ItemRowName, TEXT("LoadIconTextureByRowName"));
		if (!Row)
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot load item icon because row '%s' is missing."), *ItemRowName.ToString());
			return nullptr;
		}

		IconNameOrPath = Row->Icon_Path;
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot load item icon because row '%s' has an unknown prefix."), *ItemRowName.ToString());
		return nullptr;
	}

	if (IconNameOrPath.IsEmpty())
	{
		IconNameOrPath = ItemRowName.ToString();
		UE_LOG(LogLS, Warning, TEXT("Icon_Path is empty for row '%s'. Falling back to row name as icon asset name."), *ItemRowName.ToString());
	}

	const FString IconObjectPath = BuildIconObjectPath(IconNameOrPath, GetIconBaseFolderByRowName(ItemRowName));
	UTexture2D* IconTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *IconObjectPath));
	if (!IconTexture)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to load item icon '%s' for row '%s'."), *IconObjectPath, *ItemRowName.ToString());
	}

	return IconTexture;
}

FString ULSInventoryItemSlotWidget::BuildIconObjectPath(const FString& IconNameOrPath, const FString& BaseFolder)
{
	if (IconNameOrPath.StartsWith(TEXT("/Game/")))
	{
		if (IconNameOrPath.Contains(TEXT(".")))
		{
			return IconNameOrPath;
		}

		FString AssetName;
		IconNameOrPath.Split(TEXT("/"), nullptr, &AssetName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		return FString::Printf(TEXT("%s.%s"), *IconNameOrPath, *AssetName);
	}

	return FString::Printf(TEXT("%s%s.%s"), *BaseFolder, *IconNameOrPath, *IconNameOrPath);
}

FString ULSInventoryItemSlotWidget::GetIconBaseFolderByRowName(const FName ItemRowName)
{
	const FString RowNameString = ItemRowName.ToString();
	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		return TEXT("/Game/LostSignal/UI/Icons/Chips/");
	}

	if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		return TEXT("/Game/LostSignal/UI/Icons/Weapons/");
	}

	if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		return TEXT("/Game/LostSignal/UI/Icons/Armors/");
	}

	return TEXT("/Game/LostSignal/UI/Icons/Items/");
}
