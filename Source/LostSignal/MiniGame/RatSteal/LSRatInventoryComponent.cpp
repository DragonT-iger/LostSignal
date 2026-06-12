#include "MiniGame/RatSteal/LSRatInventoryComponent.h"

ULSRatInventoryComponent::ULSRatInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Slots.SetNum(3);
}

void ULSRatInventoryComponent::AddCrop(ELSRatCropType Type, ELSRatCropSize Size)
{
	if (Type == ELSRatCropType::None || Size == ELSRatCropSize::Born)
	{
		return;
	}

	if (Slots.Num() != SlotNum)
	{
		Slots.SetNum(SlotNum);
	}

	// 원작 Inventory::AddCrop — 순서대로 "빈 슬롯 또는 같은 종류" 첫 슬롯에 적재
	for (FLSRatSlotData& Slot : Slots)
	{
		if (Slot.IsEmpty() || Slot.Type == Type)
		{
			Slot.Type = Type;
			Slot.Count += LSRat::GetCountForSize(Size);
			OnInventoryChanged.Broadcast();
			return;
		}
	}
}

void ULSRatInventoryComponent::ChangeSlot()
{
	CurrentSlotIndex = (CurrentSlotIndex + 1) % FMath::Max(1, Slots.Num());
	OnInventoryChanged.Broadcast();
}

void ULSRatInventoryComponent::ThrowItem()
{
	if (!Slots.IsValidIndex(CurrentSlotIndex))
	{
		return;
	}

	FLSRatSlotData& Slot = Slots[CurrentSlotIndex];
	if (Slot.Count > 0)
	{
		Slot.Count--;
		if (Slot.Count == 0)
		{
			Slot.Reset();
		}
		OnInventoryChanged.Broadcast();
	}
}

TArray<FLSRatSlotData> ULSRatInventoryComponent::SubmitAll()
{
	TArray<FLSRatSlotData> Datas = Slots;

	for (FLSRatSlotData& Slot : Slots)
	{
		Slot.Reset();
	}
	OnInventoryChanged.Broadcast();

	return Datas;
}

float ULSRatInventoryComponent::GetSpeedMultiplier() const
{
	int32 Potato = 0;
	int32 Eggplant = 0;
	int32 Pumpkin = 0;

	for (const FLSRatSlotData& Slot : Slots)
	{
		if (Slot.IsEmpty())
		{
			continue;
		}

		switch (Slot.Type)
		{
		case ELSRatCropType::Potato:   Potato += Slot.Count;   break;
		case ELSRatCropType::Eggplant: Eggplant += Slot.Count; break;
		case ELSRatCropType::Pumpkin:  Pumpkin += Slot.Count;  break;
		default: break;
		}
	}

	double Mult = 1.0;
	Mult *= FMath::Pow(1.0 + PotatoBonus, static_cast<double>(Potato));
	Mult *= FMath::Pow(1.0 + EggplantBonus, static_cast<double>(Eggplant));
	Mult *= FMath::Pow(1.0 + PumpkinBonus, static_cast<double>(Pumpkin));

	return static_cast<float>(Mult);
}
