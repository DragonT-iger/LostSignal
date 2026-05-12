#include "Animation/LSCharacterAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimMontage.h"
#include "GAS/LSGameplayTags.h"
#include "GameFramework/Pawn.h"

void ULSCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CacheAbilitySystemComponent();
	RefreshAnimationState();
}

void ULSCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!CachedAbilitySystemComponent.IsValid())
	{
		CacheAbilitySystemComponent();
	}

	RefreshAnimationState();
}

void ULSCharacterAnimInstance::RefreshAnimationState()
{
	const UAbilitySystemComponent* AbilitySystemComponent = CachedAbilitySystemComponent.Get();
	bIsDead = AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(LSGameplayTags::State_Dead);

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

void ULSCharacterAnimInstance::CacheAbilitySystemComponent()
{
	CachedAbilitySystemComponent.Reset();

	const APawn* PawnOwner = TryGetPawnOwner();
	if (!PawnOwner)
	{
		return;
	}

	const IAbilitySystemInterface* AbilitySystemOwner = Cast<IAbilitySystemInterface>(PawnOwner);
	if (!AbilitySystemOwner)
	{
		return;
	}

	CachedAbilitySystemComponent = AbilitySystemOwner->GetAbilitySystemComponent();
}
