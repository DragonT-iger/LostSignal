#include "Animation/LSAnimInstanceBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GAS/LSCharacterAttributeSet.h"
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
	UpdateMoveSpeedMultiplier();
}

void ULSAnimInstanceBase::UpdateMoveSpeedMultiplier()
{
	const UAbilitySystemComponent* AbilitySystemComponent = CachedAbilitySystemComponent.Get();
	const float RawMoveSpeed = AbilitySystemComponent
		? AbilitySystemComponent->GetNumericAttribute(ULSCharacterAttributeSet::GetMoveSpeedAttribute())
		: 0.0f;

	// MoveSpeed 어트리뷰트가 없거나(다른 어트리뷰트셋) 미초기화면 0이 잡히므로 1.0으로 취급한다.
	MoveSpeedMultiplier = RawMoveSpeed > 0.0f ? RawMoveSpeed : 1.0f;
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
