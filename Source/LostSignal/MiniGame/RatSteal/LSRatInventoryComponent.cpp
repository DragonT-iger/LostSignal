#include "MiniGame/RatSteal/LSRatInventoryComponent.h"

ULSRatInventoryComponent::ULSRatInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Slots.SetNum(3);
	ResetFixedSlots();
}

void ULSRatInventoryComponent::AddCrop(ELSRatCropType Type, ELSRatCropSize Size)
{
	if (Type == ELSRatCropType::None || Size == ELSRatCropSize::Born)
	{
		return;
	}

	ResetFixedSlots();

	const int32 SlotIndex = GetSlotIndexForType(Type);
	if (!Slots.IsValidIndex(SlotIndex))
	{
		return;
	}

	Slots[SlotIndex].Count += LSRat::GetCountForSize(Size);
	OnInventoryChanged.Broadcast();
}

void ULSRatInventoryComponent::ChangeSlot()
{
	ResetFixedSlots();
	CurrentSlotIndex = (CurrentSlotIndex + 1) % FMath::Max(1, Slots.Num());
	OnInventoryChanged.Broadcast();
}

bool ULSRatInventoryComponent::ThrowItem()
{
	ResetFixedSlots();
	if (!Slots.IsValidIndex(CurrentSlotIndex))
	{
		return false;
	}

	FLSRatSlotData& Slot = Slots[CurrentSlotIndex];
	if (Slot.Count <= 0)
	{
		return false;
	}

	Slot.Count--;
	OnInventoryChanged.Broadcast();
	return true;
}

TArray<FLSRatSlotData> ULSRatInventoryComponent::SubmitAll()
{
	ResetFixedSlots();
	TArray<FLSRatSlotData> Datas = Slots;

	for (FLSRatSlotData& Slot : Slots)
	{
		Slot.Count = 0;
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
		if (Slot.Count <= 0)
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

ELSRatCropType ULSRatInventoryComponent::GetSlotCropType(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0:  return ELSRatCropType::Eggplant;
	case 1:  return ELSRatCropType::Potato;
	case 2:  return ELSRatCropType::Pumpkin;
	default: return ELSRatCropType::None;
	}
}

void ULSRatInventoryComponent::ResetFixedSlots()
{
	SlotNum = 3;
	if (Slots.Num() != SlotNum)
	{
		Slots.SetNum(SlotNum);
	}

	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		Slots[Index].Type = GetSlotCropType(Index);
		Slots[Index].Count = FMath::Max(0, Slots[Index].Count);
	}

	CurrentSlotIndex = FMath::Clamp(CurrentSlotIndex, 0, FMath::Max(0, Slots.Num() - 1));
}

int32 ULSRatInventoryComponent::GetSlotIndexForType(ELSRatCropType Type) const
{
	switch (Type)
	{
	case ELSRatCropType::Eggplant: return 0;
	case ELSRatCropType::Potato:   return 1;
	case ELSRatCropType::Pumpkin:  return 2;
	default:                       return INDEX_NONE;
	}
}
