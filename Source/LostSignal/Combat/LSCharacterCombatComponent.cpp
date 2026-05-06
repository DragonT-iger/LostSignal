#include "Combat/LSCharacterCombatComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/LSCharacterBase.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

ULSCharacterCombatComponent::ULSCharacterCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

ALSCharacterBase* ULSCharacterCombatComponent::GetOwnerCharacter() const
{
	return Cast<ALSCharacterBase>(GetOwner());
}

UAbilitySystemComponent* ULSCharacterCombatComponent::GetAbilitySystemComponent() const
{
	const ALSCharacterBase* OwnerCharacter = GetOwnerCharacter();
	return OwnerCharacter ? OwnerCharacter->GetAbilitySystemComponent() : nullptr;
}

bool ULSCharacterCombatComponent::HasCombatTag(FGameplayTag Tag) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return ASC && Tag.IsValid() && ASC->HasMatchingGameplayTag(Tag);
}

bool ULSCharacterCombatComponent::IsDead() const
{
	return HasCombatTag(LSGameplayTags::State_Dead);
}

bool ULSCharacterCombatComponent::CanStartAttack() const
{
	return !IsDead() && !HasCombatTag(LSGameplayTags::Combat_Attacking);
}

void ULSCharacterCombatComponent::SetCombatTagActive(FGameplayTag Tag, bool bActive)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !Tag.IsValid())
	{
		return;
	}

	if (bActive)
	{
		int32& RefCount = LooseTagRefCounts.FindOrAdd(Tag);
		if (RefCount == 0)
		{
			ASC->AddLooseGameplayTag(Tag);
		}

		++RefCount;
	}
	else
	{
		int32* RefCount = LooseTagRefCounts.Find(Tag);
		if (!RefCount)
		{
			return;
		}

		--(*RefCount);
		if (*RefCount <= 0)
		{
			LooseTagRefCounts.Remove(Tag);
			ASC->RemoveLooseGameplayTag(Tag);
		}
	}
}

bool ULSCharacterCombatComponent::ApplyDamageEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> DamageEffectClass, float EffectLevel) const
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!SourceASC || !TargetASC || !DamageEffectClass)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, EffectLevel, EffectContext);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	return true;
}

void ULSCharacterCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	BindHealthDelegates();
	RefreshDeathState();
}

void ULSCharacterCombatComponent::BindHealthDelegates()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	ASC->GetGameplayAttributeValueChangeDelegate(ULSCombatAttributeSet::GetCurrentHealthAttribute())
		.AddUObject(this, &ULSCharacterCombatComponent::HandleCurrentHealthChanged);
}

void ULSCharacterCombatComponent::HandleCurrentHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshDeathState();
}

void ULSCharacterCombatComponent::RefreshDeathState()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	const float CurrentHealth = ASC->GetNumericAttribute(ULSCombatAttributeSet::GetCurrentHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(ULSCombatAttributeSet::GetMaxHealthAttribute());
	const bool bShouldBeDead = MaxHealth > 0.0f && CurrentHealth <= 0.0f;
	SetCombatTagActive(LSGameplayTags::State_Dead, bShouldBeDead);
}
