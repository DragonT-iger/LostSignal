#include "Combat/LSPlayerCombatComponent.h"

#include "AbilitySystemComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Combat/LSAimComponent.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GAS/Abilities/LSGA_Dash.h"
#include "GAS/Effects/LSGE_PlayerBasicDamage.h"
#include "GAS/LSGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"
#include "TimerManager.h"

ULSPlayerCombatComponent::ULSPlayerCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	DashAbilityClass = ULSGA_Dash::StaticClass();
	BasicAttackDamageEffectClass = ULSGE_PlayerBasicDamage::StaticClass();
}

bool ULSPlayerCombatComponent::RequestBasicAttack()
{
	ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent();
	if (!OwnerCharacter || !SharedCombatComponent || !SharedCombatComponent->CanStartAttack())
	{
		return false;
	}

	if (!AttackMontage)
	{
		UE_LOG(LogLS, Warning, TEXT("%s basic attack rejected: AttackMontage is not configured."), *GetNameSafe(OwnerCharacter));
		return false;
	}

	const float AttackDuration = OwnerCharacter->PlayAnimMontage(AttackMontage);
	if (AttackDuration <= 0.0f)
	{
		UE_LOG(LogLS, Warning, TEXT("%s basic attack rejected: failed to play montage %s."), *GetNameSafe(OwnerCharacter), *GetNameSafe(AttackMontage));
		return false;
	}

	bAttackHitConsumed = false;
	SharedCombatComponent->SetCombatTagActive(LSGameplayTags::Combat_Attacking, true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackHitTimerHandle);
		World->GetTimerManager().ClearTimer(AttackRecoveryTimerHandle);
		World->GetTimerManager().SetTimer(AttackHitTimerHandle, this, &ULSPlayerCombatComponent::PerformMeleeHit, BasicAttackHitDelay, false);
		World->GetTimerManager().SetTimer(AttackRecoveryTimerHandle, this, &ULSPlayerCombatComponent::FinishAttack, AttackDuration, false);
	}

	return true;
}

bool ULSPlayerCombatComponent::RequestDash() const
{
	const ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent();
	if (!OwnerCharacter || !SharedCombatComponent || SharedCombatComponent->IsDead())
	{
		return false;
	}

	UAbilitySystemComponent* ASC = SharedCombatComponent->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(LSGameplayTags::Ability_Dash);
	return ASC->TryActivateAbilitiesByTag(AbilityTags);
}

void ULSPlayerCombatComponent::PerformMeleeHit()
{
	if (bAttackHitConsumed)
	{
		return;
	}

	ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent();
	ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	if (!SharedCombatComponent || !OwnerCharacter)
	{
		return;
	}

	bAttackHitConsumed = true;
	SharedCombatComponent->SetCombatTagActive(LSGameplayTags::Combat_AttackActive, true);

	FVector AttackDirection = OwnerCharacter->GetActorForwardVector();
	if (const ULSAimComponent* AimComponent = ResolveAimComponent())
	{
		AttackDirection = AimComponent->GetAimDirection();
	}

	AttackDirection.Z = 0.0f;
	if (AttackDirection.IsNearlyZero())
	{
		AttackDirection = OwnerCharacter->GetActorForwardVector();
	}
	else
	{
		AttackDirection = AttackDirection.GetSafeNormal();
	}
	ExecuteMeleeHit(AttackDirection);

	SharedCombatComponent->SetCombatTagActive(LSGameplayTags::Combat_AttackActive, false);
}

bool ULSPlayerCombatComponent::IsAttackInProgress() const
{
	const ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent();
	return SharedCombatComponent && SharedCombatComponent->HasCombatTag(LSGameplayTags::Combat_Attacking);
}

void ULSPlayerCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter(); OwnerCharacter && OwnerCharacter->HasAuthority() && DashAbilityClass)
	{
		OwnerCharacter->GrantAbility(DashAbilityClass);
	}
}

ULSAimComponent* ULSPlayerCombatComponent::ResolveAimComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<ULSAimComponent>() : nullptr;
}

ULSCharacterCombatComponent* ULSPlayerCombatComponent::ResolveSharedCombatComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<ULSCharacterCombatComponent>() : nullptr;
}

ALSCharacterBase* ULSPlayerCombatComponent::ResolveOwnerCharacter() const
{
	return Cast<ALSCharacterBase>(GetOwner());
}

void ULSPlayerCombatComponent::FinishAttack()
{
	if (ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent())
	{
		SharedCombatComponent->SetCombatTagActive(LSGameplayTags::Combat_Attacking, false);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AttackHitTimerHandle);
		World->GetTimerManager().ClearTimer(AttackRecoveryTimerHandle);
	}
}

void ULSPlayerCombatComponent::ExecuteMeleeHit(const FVector& AttackDirection)
{
	ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent();
	if (!OwnerCharacter || !SharedCombatComponent || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	const FVector TraceCenter = OwnerCharacter->GetActorLocation() + (AttackDirection * BasicAttackForwardOffset);

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		TraceCenter,
		BasicAttackRadius,
		ObjectTypes,
		nullptr,
		ActorsToIgnore,
		OverlappedActors);

	TSet<AActor*> UniqueTargets;
	for (AActor* HitActor : OverlappedActors)
	{
		if (!HitActor || UniqueTargets.Contains(HitActor))
		{
			continue;
		}

		if (SharedCombatComponent->ApplyDamageEffectToTarget(HitActor, BasicAttackDamageEffectClass, DamageEffectLevel))
		{
			UniqueTargets.Add(HitActor);
		}
	}

	UE_LOG(
		LogLS,
		Log,
		TEXT("%s player melee hit. Center=(%.1f, %.1f, %.1f) Radius=%.1f ValidTargets=%d"),
		*GetNameSafe(OwnerCharacter),
		TraceCenter.X,
		TraceCenter.Y,
		TraceCenter.Z,
		BasicAttackRadius,
		UniqueTargets.Num());
}
