#include "Animation/LSCharacterAnimInstance.h"

#include "Animation/AnimMontage.h"

void ULSCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	RefreshAnimationState();
}

void ULSCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	RefreshAnimationState();
}

void ULSCharacterAnimInstance::RefreshAnimationState()
{
	// bIsDead는 베이스(Super)에서 이미 갱신됨.
	CurrentActiveMontage = GetCurrentActiveMontage();
	bIsFullBodyMontagePlaying = IsMontageUsingSlot(CurrentActiveMontage, FullBodySlotName);

	bShouldBlockUpperBodySlot = bIsDead || bIsFullBodyMontagePlaying;
}

bool ULSCharacterAnimInstance::IsMontageUsingSlot(const UAnimMontage* Montage, FName SlotName) const
{
	if (!Montage || SlotName.IsNone())
	{
		return false;
	}

	for (const FSlotAnimationTrack& SlotTrack : Montage->SlotAnimTracks)
	{
		if (SlotTrack.SlotName == SlotName)
		{
			return true;
		}
	}

	return false;
}
