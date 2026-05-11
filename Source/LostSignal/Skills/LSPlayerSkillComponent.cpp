#include "Skills/LSPlayerSkillComponent.h"

#include "GameFramework/Pawn.h"
#include "Skills/LSSkillDataAsset.h"
#include "Skills/Preview/LSSkillPreviewComponent.h"

ULSPlayerSkillComponent::ULSPlayerSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
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

	ActiveSkillData = nullptr;
	ActiveSlot = Slot;

	if (!PreviewComponent->BeginAreaPreview(SkillData->BuildPreviewSpec()))
	{
		return false;
	}

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

bool ULSPlayerSkillComponent::ConfirmAnyActiveSkillPreview(const FVector& TargetLocation, const FRotator& AimRotation)
{
	if (!ActiveSkillData)
	{
		return false;
	}

	const ELSPlayerSkillSlot SlotToActivate = ActiveSlot;
	const bool bConfirmed = ConfirmActiveSkillPreview(SlotToActivate);
	if (!bConfirmed)
	{
		return false;
	}

	if (const AActor* OwnerActor = GetOwner())
	{
		if (OwnerActor->HasAuthority())
		{
			ActivateSkillOnServer(SlotToActivate, TargetLocation, AimRotation.Yaw);
		}
		else
		{
			ServerRequestActivateSkill(SlotToActivate, TargetLocation, AimRotation.Yaw);
		}
	}

	return true;
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

	OutPreviewSpec = ActiveSkillData->BuildPreviewSpec();
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

void ULSPlayerSkillComponent::ServerRequestActivateSkill_Implementation(ELSPlayerSkillSlot Slot, FVector_NetQuantize TargetLocation, float AimYaw)
{
	ActivateSkillOnServer(Slot, FVector(TargetLocation), AimYaw);
}

bool ULSPlayerSkillComponent::ActivateSkillOnServer(ELSPlayerSkillSlot Slot, const FVector& TargetLocation, float AimYaw)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	ULSSkillDataAsset* SkillData = GetSkillData(Slot);
	if (!SkillData)
	{
		return false;
	}

	FLSSkillActivationContext Context;
	Context.SourceActor = OwnerActor;
	Context.SkillData = SkillData;
	Context.TargetLocation = TargetLocation;
	Context.AimYaw = AimYaw;
	return SkillData->ActivateSkill(Context);
}
