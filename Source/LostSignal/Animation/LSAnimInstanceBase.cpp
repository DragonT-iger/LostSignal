#include "Animation/LSAnimInstanceBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GAS/LSGameplayTags.h"
#include "GameFramework/Pawn.h"

void ULSAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CacheAbilitySystemComponent();
	UpdateDeathState();
}

void ULSAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!CachedAbilitySystemComponent.IsValid())
	{
		CacheAbilitySystemComponent();
	}

	UpdateDeathState();
}

void ULSAnimInstanceBase::UpdateDeathState()
{
	const UAbilitySystemComponent* AbilitySystemComponent = CachedAbilitySystemComponent.Get();
	bIsDead = AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(LSGameplayTags::State_Dead);
}

void ULSAnimInstanceBase::CacheAbilitySystemComponent()
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
