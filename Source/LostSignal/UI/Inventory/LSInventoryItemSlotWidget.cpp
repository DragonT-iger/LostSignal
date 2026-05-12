#include "UI/Inventory/LSInventoryItemSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/LSArmorRow.h"
#include "Data/LSChipRow.h"
#include "Data/LSDropSettings.h"
#include "Data/LSItemRow.h"
#include "Data/LSWeaponRow.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "LostSignal.h"
#include "UI/Inventory/LSInventoryDragDropOperation.h"
#include "UI/Inventory/LSInventoryWidget.h"

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
		IconTexture = LoadDefaultIconTexture();
		UE_LOG(LogLS, Warning, TEXT("Using default inventory icon for row '%s' on %s."), *ItemRowName.ToString(), *GetNameSafe(this));
	}

	if (IconTexture)
	{
		ItemIconImage->SetBrushFromTexture(IconTexture);
	}

	ItemIconImage->SetColorAndOpacity(FLinearColor::White);
	ItemIconImage->SetVisibility(ESlateVisibility::Visible);
	AmountText->SetText(FText::AsNumber(Amount));
	AmountText->SetVisibility(Amount > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	SetTooltipItem(ItemRowName, Amount);
	bHasItem = true;
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

	if (UTexture2D* DefaultIconTexture = LoadDefaultIconTexture())
	{
		ItemIconImage->SetBrushFromTexture(DefaultIconTexture);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("ClearItem could not load default icon on %s."), *GetNameSafe(this));
	}

	ItemIconImage->SetColorAndOpacity(FLinearColor::White);
	ItemIconImage->SetVisibility(ESlateVisibility::Visible);
	AmountText->SetText(FText::GetEmpty());
	AmountText->SetVisibility(ESlateVisibility::Collapsed);
	bHasItem = false;
	ClearTooltipItem();
}

void ULSInventoryItemSlotWidget::SetSlotContext(ULSInventoryWidget* InInventoryWidget, const ELSInventorySlotArea InSlotArea, const int32 InSlotIndex, const bool bInHasItem)
{
	InventoryWidget = InInventoryWidget;
	SlotArea = InSlotArea;
	SlotIndex = InSlotIndex;
	bHasItem = bInHasItem;
}

FReply ULSInventoryItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bHasItem)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}

void ULSInventoryItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (!bHasItem || !InventoryWidget.IsValid() || SlotIndex == INDEX_NONE)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot start inventory drag. Widget=%s SlotIndex=%d HasItem=%s"),
			*GetNameSafe(this), SlotIndex, bHasItem ? TEXT("true") : TEXT("false"));
		return;
	}

	ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(
		UWidgetBlueprintLibrary::CreateDragDropOperation(ULSInventoryDragDropOperation::StaticClass()));
	if (!DragOperation)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to create inventory drag operation on %s."), *GetNameSafe(this));
		return;
	}

	DragOperation->SourceInventoryWidget = InventoryWidget.Get();
	DragOperation->SourceSlotWidget = this;
	DragOperation->SourceSlotIndex = SlotIndex;
	DragOperation->SourceSlotArea = SlotArea;
	DragOperation->DefaultDragVisual = this;
	DragOperation->Pivot = EDragPivot::MouseDown;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(0.25f);
	OutOperation = DragOperation;
}

bool ULSInventoryItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	ULSInventoryWidget* TargetInventoryWidget = InventoryWidget.Get();
	if (!TargetInventoryWidget || DragOperation->SourceInventoryWidget != TargetInventoryWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot because source/target inventory widget does not match."));
		return false;
	}

	return TargetInventoryWidget->HandleInventorySlotDrop(
		DragOperation->SourceSlotArea,
		DragOperation->SourceSlotIndex,
		SlotArea,
		SlotIndex);
}

void ULSInventoryItemSlotWidget::RestoreDragSourceVisual()
{
	SetVisibility(ESlateVisibility::Visible);
	SetRenderOpacity(1.0f);
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

UTexture2D* ULSInventoryItemSlotWidget::LoadDefaultIconTexture() const
{
	static const TCHAR* DefaultIconObjectPath = TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture");
	UTexture2D* DefaultIconTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, DefaultIconObjectPath));
	if (!DefaultIconTexture)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to load default inventory icon '%s'."), DefaultIconObjectPath);
	}

	return DefaultIconTexture;
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
