#include "UI/LootDrop/LSLootDropWidget.h"

#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Core/LSPlayerControllerBase.h"
#include "Gameplay/LSLootBox.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "UI/Inventory/LSItemSlotWidget.h"
#include "UI/Inventory/LSSlotWidgetSync.h"

void ULSLootDropWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!LootSourceNameText)
	{
		UE_LOG(LogLS, Warning, TEXT("LootSourceNameText is not bound on %s."), *GetNameSafe(this));
	}

	if (!LootItemWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("LootItemWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	LootItemWrapBox->ClearChildren();
	LootItems.Empty();
}

void ULSLootDropWidget::SetLootSourceName(const FText InSourceName)
{
	if (!LootSourceNameText)
	{
		UE_LOG(LogLS, Warning, TEXT("LootSourceNameText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	LootSourceNameText->SetText(InSourceName);
}

void ULSLootDropWidget::SetLootItems(const TArray<FLSDropResult>& InItems)
{
	LootItems = InItems;
	HoveredLootSlotIndex = INDEX_NONE;
	RebuildLootSlots();
}

void ULSLootDropWidget::SetSourceLootBox(ALSLootBox* InSourceLootBox)
{
	SourceLootBox = InSourceLootBox;
}

bool ULSLootDropWidget::IsShowingLootSource(const ALSLootBox* InSourceLootBox) const
{
	return SourceLootBox == InSourceLootBox;
}

void ULSLootDropWidget::RefreshLootItemsFromSource(ALSLootBox* InSourceLootBox, const TArray<FLSDropResult>& InItems)
{
	if (!IsShowingLootSource(InSourceLootBox))
	{
		return;
	}

	// 단계 공개로 슬롯이 늘어난 경우에만 등장 사운드를 재생한다.
	// (박스 재오픈은 ShowLootDropWidget → SetLootItems 직행이라 여기로 오지 않음)
	const int32 PreviousItemCount = LootItems.Num();
	SetLootItems(InItems);
	if (LootItems.Num() > PreviousItemCount)
	{
		PlayRevealSoundForNewItems(PreviousItemCount);
	}
}

void ULSLootDropWidget::PlayRevealSoundForNewItems(const int32 FirstNewItemIndex)
{
	for (int32 ItemIndex = FirstNewItemIndex; ItemIndex < LootItems.Num(); ++ItemIndex)
	{
		const FLSDropResult& Item = LootItems[ItemIndex];
		if (Item.ItemRowName.IsNone() || Item.Amount <= 0)
		{
			continue;
		}

		const FString Grade = LSInventorySlotUtils::ResolveItemGradeFromRowName(Item.ItemRowName);
		const TObjectPtr<USoundBase>* FoundSound = GradeRevealSounds.Find(FName(*Grade));
		if (!FoundSound || !*FoundSound)
		{
			UE_LOG(LogLS, Warning, TEXT("GradeRevealSounds has no sound mapped for grade '%s' on %s."), *Grade, *GetNameSafe(this));
			continue;
		}

		UGameplayStatics::PlaySound2D(this, *FoundSound);
	}
}

void ULSLootDropWidget::RebuildLootSlots()
{
	if (!LootItemWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("LootItemWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (!ItemSlotWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemSlotWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	// 단계 공개: 총 드랍 개수만큼 슬롯 프레임을 그린다.
	// - 공개된 인덱스: 실제 아이템(전환 시 스스로 pop-in).
	// - 바로 다음에 공개될 한 칸만: 스캔 placeholder(미확인 아이콘 + 펄스) — 연출은 이 칸 하나뿐.
	// - 그 이후 칸: 빈 기본 배경 프레임(연출 없음).
	const int32 TotalCount = SourceLootBox ? SourceLootBox->GetTotalLootCount() : LootItems.Num();
	const int32 SlotCount = FMath::Max(LootItems.Num(), TotalCount);
	const int32 NextRevealSlotIndex = LootItems.Num();

	LSSlotWidgetSync::SyncSlotWidgets(LootItemWrapBox, ItemSlotWidgetClass, GetOwningPlayer(), GetWorld(), SlotCount,
		[this, NextRevealSlotIndex](const int32 SlotIndex, ULSItemSlotWidget& SlotWidget)
		{
			if (LootItems.IsValidIndex(SlotIndex))
			{
				const FLSDropResult& Item = LootItems[SlotIndex];
				const bool bHasItem = !Item.ItemRowName.IsNone() && Item.Amount > 0;
				SlotWidget.SetLootSlotContext(this, SlotIndex, bHasItem);
				if (bHasItem)
				{
					SlotWidget.SetItem(Item.ItemRowName, Item.Amount, Item.ChipStats);
				}
				else
				{
					// 공개됐지만 looted되어 비워진 슬롯.
					SlotWidget.ClearItem();
				}
			}
			else if (SlotIndex == NextRevealSlotIndex)
			{
				// 다음에 공개될 한 칸만 스캔 연출.
				SlotWidget.SetLootSlotContext(this, SlotIndex, /*bHasItem*/ false);
				SlotWidget.SetPlaceholder();
			}
			else
			{
				// 아직 차례가 아닌 칸: 빈 기본 배경 프레임만(연출 없음).
				SlotWidget.SetLootSlotContext(this, SlotIndex, /*bHasItem*/ false);
				SlotWidget.ClearItem();
			}
		});
}

void ULSLootDropWidget::ClearLootItems()
{
	LootItems.Empty();
	HoveredLootSlotIndex = INDEX_NONE;

	if (!LootItemWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("LootItemWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	LootItemWrapBox->ClearChildren();
}

void ULSLootDropWidget::ClearLootSlotAt(const int32 SlotIndex)
{
	if (!LootItemWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("LootItemWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (SlotIndex < 0 || SlotIndex >= LootItemWrapBox->GetChildrenCount())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot clear loot slot because index %d is invalid on %s."), SlotIndex, *GetNameSafe(this));
		return;
	}

	if (LootItems.IsValidIndex(SlotIndex))
	{
		SetLootSlotFromSessionItem(SlotIndex, FLSSessionItem());
		RebuildLootSlots();
		return;
	}

	ULSItemSlotWidget* SlotWidget = Cast<ULSItemSlotWidget>(LootItemWrapBox->GetChildAt(SlotIndex));
	if (!SlotWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot clear loot slot because child %d is not an inventory item slot on %s."), SlotIndex, *GetNameSafe(this));
		return;
	}

	SlotWidget->ClearItem();
}

bool ULSLootDropWidget::TransferLootSlotToInventory(const int32 SlotIndex)
{
	if (!LootItems.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot because index %d is invalid on %s."), SlotIndex, *GetNameSafe(this));
		return false;
	}

	const FLSDropResult LootItem = LootItems[SlotIndex];
	if (LootItem.ItemRowName.IsNone() || LootItem.Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot because item data is invalid. Index=%d Row=%s Amount=%d"),
			SlotIndex,
			*LootItem.ItemRowName.ToString(),
			LootItem.Amount);
		return false;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot because owning player controller is invalid on %s."), *GetNameSafe(this));
		return false;
	}

	FLSSessionItem RemainingLootItem;
	if (!PlayerController->TransferLootDropSlotToSession(SourceLootBox, SlotIndex, LootItem.ItemRowName, LootItem.Amount, RemainingLootItem))
	{
		return false;
	}

	if (!PlayerController->HasAuthority())
	{
		return true;
	}

	SetLootSlotFromSessionItem(SlotIndex, RemainingLootItem);
	RebuildLootSlots();
	return true;
}

bool ULSLootDropWidget::TransferLootSlotToInventorySlot(const int32 SlotIndex, const ELSInventorySlotArea ToSlotArea, const int32 ToSlotIndex)
{
	if (!LootItems.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to inventory slot because index %d is invalid on %s."), SlotIndex, *GetNameSafe(this));
		return false;
	}

	const FLSDropResult LootItem = LootItems[SlotIndex];
	if (LootItem.ItemRowName.IsNone() || LootItem.Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to inventory slot because item data is invalid. Index=%d Row=%s Amount=%d"),
			SlotIndex,
			*LootItem.ItemRowName.ToString(),
			LootItem.Amount);
		return false;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer loot slot to inventory slot because owning player controller is invalid on %s."), *GetNameSafe(this));
		return false;
	}

	FLSSessionItem RemainingLootItem;
	if (!PlayerController->TransferLootDropSlotToSessionSlot(SourceLootBox, SlotIndex, LootItem.ItemRowName, LootItem.Amount, ToSlotArea, ToSlotIndex, RemainingLootItem))
	{
		return false;
	}

	if (!PlayerController->HasAuthority())
	{
		return true;
	}

	SetLootSlotFromSessionItem(SlotIndex, RemainingLootItem);
	RebuildLootSlots();
	return true;
}

bool ULSLootDropWidget::TransferHoveredLootSlotToInventory()
{
	if (!LootItems.IsValidIndex(HoveredLootSlotIndex))
	{
		return false;
	}

	const FLSDropResult& LootItem = LootItems[HoveredLootSlotIndex];
	if (LootItem.ItemRowName.IsNone() || LootItem.Amount <= 0)
	{
		return false;
	}

	return TransferLootSlotToInventory(HoveredLootSlotIndex);
}

bool ULSLootDropWidget::TransferInventorySlotToLootSlot(const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex, const int32 LootSlotIndex)
{
	if (!LootItems.IsValidIndex(LootSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer inventory slot to loot slot because loot index %d is invalid on %s."), LootSlotIndex, *GetNameSafe(this));
		return false;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot transfer inventory slot to loot slot because owning player controller is invalid on %s."), *GetNameSafe(this));
		return false;
	}

	FLSSessionItem NewLootItem;
	if (!PlayerController->TransferSessionSlotToLootDropSlot(SourceLootBox, FromSlotArea, FromSlotIndex, LootSlotIndex, LootItems[LootSlotIndex], NewLootItem))
	{
		return false;
	}

	if (!PlayerController->HasAuthority())
	{
		return true;
	}

	SetLootSlotFromSessionItem(LootSlotIndex, NewLootItem);
	RebuildLootSlots();
	return true;
}

bool ULSLootDropWidget::TransferInventorySlotToFirstEmptyLootSlot(const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex)
{
	for (int32 LootSlotIndex = 0; LootSlotIndex < LootItems.Num(); ++LootSlotIndex)
	{
		const FLSDropResult& LootItem = LootItems[LootSlotIndex];
		if (LootItem.ItemRowName.IsNone() || LootItem.Amount <= 0)
		{
			return TransferInventorySlotToLootSlot(FromSlotArea, FromSlotIndex, LootSlotIndex);
		}
	}

	return false;
}

bool ULSLootDropWidget::DropLootSlot(const int32 FromLootSlotIndex, const int32 ToLootSlotIndex)
{
	if (!LootItems.IsValidIndex(FromLootSlotIndex) || !LootItems.IsValidIndex(ToLootSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop loot slot because an index is invalid on %s. From=%d To=%d"),
			*GetNameSafe(this),
			FromLootSlotIndex,
			ToLootSlotIndex);
		return false;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop loot slot because owning player controller is invalid on %s."), *GetNameSafe(this));
		return false;
	}

	if (!PlayerController->DropLootDropSlot(SourceLootBox, FromLootSlotIndex, ToLootSlotIndex))
	{
		return false;
	}

	if (!PlayerController->HasAuthority())
	{
		return true;
	}

	if (SourceLootBox)
	{
		RefreshLootItemsFromSource(SourceLootBox, SourceLootBox->GetLootResults());
	}
	return true;
}

void ULSLootDropWidget::NotifyLootSlotHovered(const int32 SlotIndex)
{
	HoveredLootSlotIndex = SlotIndex;
}

void ULSLootDropWidget::NotifyLootSlotUnhovered(const int32 SlotIndex)
{
	if (HoveredLootSlotIndex == SlotIndex)
	{
		HoveredLootSlotIndex = INDEX_NONE;
	}
}

void ULSLootDropWidget::SetLootSlotFromSessionItem(const int32 SlotIndex, const FLSSessionItem& SessionItem)
{
	if (!LootItems.IsValidIndex(SlotIndex))
	{
		return;
	}

	LSInventorySlotUtils::SetDropResultFromSessionItem(LootItems[SlotIndex], SessionItem);
	if (HoveredLootSlotIndex == SlotIndex && (SessionItem.ItemRowName.IsNone() || SessionItem.Amount <= 0))
	{
		HoveredLootSlotIndex = INDEX_NONE;
	}
}

bool ULSLootDropWidget::HasLootItems() const
{
	for (const FLSDropResult& LootItem : LootItems)
	{
		if (!LootItem.ItemRowName.IsNone() && LootItem.Amount > 0)
		{
			return true;
		}
	}

	return false;
}
