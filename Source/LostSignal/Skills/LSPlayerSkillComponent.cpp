#include "Skills/LSPlayerSkillComponent.h"

#include "GameFramework/Pawn.h"
#include "Skills/LSSkillDataAsset.h"
#include "Skills/LSSkillPreviewComponent.h"

ULSPlayerSkillComponent::ULSPlayerSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool ULSPlayerSkillComponent::BeginSkillPreview(ELSPlayerSkillSlot Slot)
{
	if (!CanUseLocalPreview())
	{
		return false;
	}

	ULSSkillDataAsset* SkillData = GetSkillData(Slot);
	ULSSkillPreviewComponent* PreviewComponent = ResolvePreviewComponent();
	if (!SkillData || !PreviewComponent)
	{
		return false;
	}

	if (ActiveSkillData)
	{
		PreviewComponent->EndAreaPreview();
	}

	if (!PreviewComponent->BeginAreaPreview(SkillData->GetPreviewSpec()))
	{
		return false;
	}

	ActiveSlot = Slot;
	ActiveSkillData = SkillData;
	return true;
}

void ULSPlayerSkillComponent::UpdateActiveSkillPreview(const FVector& WorldLocation, const FRotator& WorldRotation)
{
	if (!ActiveSkillData)
	{
		return;
	}

	if (ULSSkillPreviewComponent* PreviewComponent = ResolvePreviewComponent())
	{
		PreviewComponent->UpdateAreaPreview(WorldLocation, WorldRotation);
	}
}

bool ULSPlayerSkillComponent::ConfirmActiveSkillPreview(ELSPlayerSkillSlot Slot)
{
	if (!ActiveSkillData || ActiveSlot != Slot)
	{
		return false;
	}

	if (ULSSkillPreviewComponent* PreviewComponent = ResolvePreviewComponent())
	{
		PreviewComponent->EndAreaPreview();
	}

	ActiveSkillData = nullptr;
	return true;
}

bool ULSPlayerSkillComponent::ConfirmAnyActiveSkillPreview()
{
	if (!ActiveSkillData)
	{
		return false;
	}

	return ConfirmActiveSkillPreview(ActiveSlot);
}

void ULSPlayerSkillComponent::CancelActiveSkillPreview(ELSPlayerSkillSlot Slot)
{
	if (!ActiveSkillData || ActiveSlot != Slot)
	{
		return;
	}

	CancelAnyActiveSkillPreview();
}

void ULSPlayerSkillComponent::CancelAnyActiveSkillPreview()
{
	if (ULSSkillPreviewComponent* PreviewComponent = ResolvePreviewComponent())
	{
		PreviewComponent->EndAreaPreview();
	}

	ActiveSkillData = nullptr;
}

ULSSkillDataAsset* ULSPlayerSkillComponent::GetSkillData(ELSPlayerSkillSlot Slot) const
{
	const FLSPlayerSkillSlotSpec* SlotSpec = SkillSlots.Find(Slot);
	return SlotSpec ? SlotSpec->SkillData.Get() : nullptr;
}

bool ULSPlayerSkillComponent::GetActivePreviewSpec(FLSSkillAreaPreviewSpec& OutPreviewSpec) const
{
	if (!ActiveSkillData)
	{
		return false;
	}

	OutPreviewSpec = ActiveSkillData->GetPreviewSpec();
	return true;
}

bool ULSPlayerSkillComponent::CanUseLocalPreview() const
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}

ULSSkillPreviewComponent* ULSPlayerSkillComponent::ResolvePreviewComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<ULSSkillPreviewComponent>() : nullptr;
}
