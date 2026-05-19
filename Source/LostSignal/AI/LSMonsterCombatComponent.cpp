#include "AI/LSMonsterCombatComponent.h"

#include "AbilitySystemComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Data/LSMonsterArchetypeRow.h"
#include "GAS/Effects/LSGE_MonsterBasicDamage.h"
#include "GAS/LSGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"

ULSMonsterCombatComponent::ULSMonsterCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	DefaultAttackAbilityTag = LSGameplayTags::Ability_MonsterMelee;
	DamageEffectClass = ULSGE_MonsterBasicDamage::StaticClass();
}

void ULSMonsterCombatComponent::ApplyArchetype(const FLSMonsterArchetypeRow& Row)
{
	DefaultAttackAbilityTag = Row.DefaultAttackAbilityTag;
	LeashDistance = Row.LeashDistance;
	AlertMoveSpeedMultiplier = Row.AlertMoveSpeedMultiplier;
}

bool ULSMonsterCombatComponent::RequestAbilityByTag(FGameplayTag AbilityTag) const
{
	if (!AbilityTag.IsValid())
	{
		return false;
	}

	const ALSCharacterBase* Character = Cast<ALSCharacterBase>(GetOwner());
	if (!Character)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	if (ASC->HasMatchingGameplayTag(LSGameplayTags::State_Dead) ||
		ASC->HasMatchingGameplayTag(LSGameplayTags::State_Stunned))
	{
		return false;
	}

	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(AbilityTag);
	return ASC->TryActivateAbilitiesByTag(AbilityTags);
}

void ULSMonsterCombatComponent::CancelAbilityByTag(FGameplayTag AbilityTag) const
{
	if (!AbilityTag.IsValid())
	{
		return;
	}

	const ALSCharacterBase* Character = Cast<ALSCharacterBase>(GetOwner());
	UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(AbilityTag);
	ASC->CancelAbilities(&AbilityTags);
}

bool ULSMonsterCombatComponent::IsAbilityActiveByTag(FGameplayTag AbilityTag) const
{
	if (!AbilityTag.IsValid())
	{
		return false;
	}

	const ALSCharacterBase* Character = Cast<ALSCharacterBase>(GetOwner());
	UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return false;
	}

	TArray<FGameplayAbilitySpec*> MatchingSpecs;
	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(AbilityTag);
	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(AbilityTags, MatchingSpecs, false);
	for (const FGameplayAbilitySpec* Spec : MatchingSpecs)
	{
		if (Spec && Spec->IsActive())
		{
			return true;
		}
	}

	return false;
}

void ULSMonsterCombatComponent::PerformMeleeHit()
{
	ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		UE_LOG(LogLS, Warning, TEXT("%s: OwnerCharacter Missing"), *GetNameSafe(GetOwner()));
		return;
	}

	ULSCharacterCombatComponent* SharedCombatComponent = OwnerCharacter->FindComponentByClass<ULSCharacterCombatComponent>();
	if (!SharedCombatComponent || !DamageEffectClass)
	{
		UE_LOG(LogLS, Warning, TEXT("%s: PerformMeleeHit skipped, combat component or DamageEffectClass missing."), *GetNameSafe(GetOwner()));
		return;
	}

	if (SharedCombatComponent->IsDead())
	{
		UE_LOG(LogLS, Log, TEXT("%s: PerformMeleeHit skipped because owner is dead."), *GetNameSafe(OwnerCharacter));
		return;
	}

	const FVector TraceCenter = OwnerCharacter->GetActorLocation() + (OwnerCharacter->GetActorForwardVector() * MeleeHitForwardOffset);

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		TraceCenter,
		MeleeHitRadius,
		ObjectTypes,
		nullptr,
		ActorsToIgnore,
		OverlappedActors
	);

	UE_LOG(
		LogLS,
		Log,
		TEXT("%s melee overlap. Center=(%.1f, %.1f, %.1f) Radius=%.1f RawHits=%d"),
		*GetNameSafe(OwnerCharacter),
		TraceCenter.X,
		TraceCenter.Y,
		TraceCenter.Z,
		MeleeHitRadius,
		OverlappedActors.Num()
	);

	TSet<AActor*> UniqueTargets;
	for (AActor* HitActor : OverlappedActors)
	{
		if (!HitActor || UniqueTargets.Contains(HitActor))
		{
			continue;
		}

		UE_LOG(
			LogLS,
			Log,
			TEXT("%s melee overlap candidate: %s"),
			*GetNameSafe(OwnerCharacter),
			*GetNameSafe(HitActor)
		);

		if (!SharedCombatComponent->ApplyDamageEffectToTarget(
			HitActor,
			DamageEffectClass,
			DamageEffectLevel,
			MeleeFixedDamage,
			MeleeAttackCoefficient,
			bMeleeCanCrit,
			MeleeBreakPower))
		{
			UE_LOG(
				LogLS,
				Log,
				TEXT("%s melee overlap ignored %s because damage application failed."),
				*GetNameSafe(OwnerCharacter),
				*GetNameSafe(HitActor));
			continue;
		}

		UniqueTargets.Add(HitActor);

		UE_LOG(
			LogLS,
			Log,
			TEXT("%s melee hit %s."),
			*GetNameSafe(OwnerCharacter),
			*GetNameSafe(HitActor));
	}

	if (UniqueTargets.Num() == 0)
	{
		UE_LOG(LogLS, Warning, TEXT("%s melee overlap found no valid damage target."), *GetNameSafe(OwnerCharacter));
	}
}

bool ULSMonsterCombatComponent::HasValidDamageEffect() const
{
	return DamageEffectClass != nullptr;
}
