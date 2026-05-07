#include "AI/LSMonsterCombatComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Data/LSMonsterArchetypeRow.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GAS/Effects/LSGE_MonsterBasicDamage.h"
#include "GAS/LSGameplayTags.h"
#include "GameplayEffect.h"
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

	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(AbilityTag);
	return ASC->TryActivateAbilitiesByTag(AbilityTags);
}

void ULSMonsterCombatComponent::PerformMeleeHit()
{
	ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		UE_LOG(LogLS, Warning, TEXT("%s: OwnerCharacter Missing"), *GetNameSafe(GetOwner()));
		return;
	}

	UAbilitySystemComponent* SourceASC = OwnerCharacter->GetAbilitySystemComponent();
	if (!SourceASC || !DamageEffectClass)
	{
		UE_LOG(LogLS, Warning, TEXT("%s: PerformMeleeHit skipped, ASC or DamageEffectClass missing."), *GetNameSafe(GetOwner()));
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

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
		if (!TargetASC)
		{
			UE_LOG(
				LogLS,
				Log,
				TEXT("%s melee overlap ignored %s because it has no ASC."),
				*GetNameSafe(OwnerCharacter),
				*GetNameSafe(HitActor)
			);
			continue;
		}

		UniqueTargets.Add(HitActor);

		FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, DamageEffectLevel, EffectContext);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(LSGameplayTags::Data_Damage_Base, MeleeBaseDamage);
			SpecHandle.Data->SetSetByCallerMagnitude(LSGameplayTags::Data_Damage_AttackCoefficient, MeleeAttackCoefficient);
			SpecHandle.Data->SetSetByCallerMagnitude(LSGameplayTags::Data_Damage_CanCrit, bMeleeCanCrit ? 1.0f : 0.0f);

			const float BeforeHealth = TargetASC->GetNumericAttribute(ULSCombatAttributeSet::GetCurrentHealthAttribute());
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			const float AfterHealth = TargetASC->GetNumericAttribute(ULSCombatAttributeSet::GetCurrentHealthAttribute());

			UE_LOG(
				LogLS,
				Log,
				TEXT("%s melee hit %s. HP Before: %.1f -> After: %.1f"),
				*GetNameSafe(OwnerCharacter),
				*GetNameSafe(HitActor),
				BeforeHealth,
				AfterHealth
			);
		}
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
