#include "Combat/LSPlayerCombatComponent.h"

#include "AbilitySystemComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Combat/LSAimComponent.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GAS/Abilities/LSGA_Dash.h"
#include "GAS/Effects/LSGE_PlayerBasicDamage.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
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
	if (!OwnerCharacter || OwnerCharacter->IsTemplate() || !SharedCombatComponent || !SharedCombatComponent->CanStartAttack())
	{
		return false;
	}

	if (!AttackMontage)
	{
		UE_LOG(
			LogLS,
			Warning,
			TEXT("%s basic attack rejected: AttackMontage is not configured. OwnerClass=%s Component=%s ComponentOuter=%s IsTemplate=%d"),
			*GetNameSafe(OwnerCharacter),
			*GetNameSafe(OwnerCharacter->GetClass()),
			*GetNameSafe(this),
			*GetNameSafe(GetOuter()),
			IsTemplate() ? 1 : 0);
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

bool ULSPlayerCombatComponent::RequestDash()
{
	ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	FVector DashDirection = OwnerCharacter ? OwnerCharacter->GetActorForwardVector() : FVector::ForwardVector;
	return RequestDash(DashDirection);
}

bool ULSPlayerCombatComponent::RequestDash(const FVector& DashDirection)
{
	const ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent();
	if (!OwnerCharacter || !SharedCombatComponent || SharedCombatComponent->IsDead())
	{
		return false;
	}

	if (!OwnerCharacter->HasAuthority())
	{
		return false;
	}

	PendingDashDirection = DashDirection.GetSafeNormal2D();
	if (PendingDashDirection.IsNearlyZero())
	{
		PendingDashDirection = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
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

bool ULSPlayerCombatComponent::PredictDashMovement(const FVector& DashDirection)
{
	ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent();
	if (!OwnerCharacter || OwnerCharacter->HasAuthority() || !OwnerCharacter->IsLocallyControlled() || !SharedCombatComponent || SharedCombatComponent->IsDead())
	{
		return false;
	}

	if (bPredictedDashInProgress || IsDashCooldownActive())
	{
		return false;
	}

	uint16 NewRootMotionSourceID = 0;
	if (!ApplyDashRootMotion(DashDirection, NewRootMotionSourceID))
	{
		return false;
	}

	bPredictedDashInProgress = true;
	bPredictedDashCooldownActive = true;
	PredictedDashRootMotionSourceID = NewRootMotionSourceID;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PredictedDashTimerHandle);
		World->GetTimerManager().ClearTimer(PredictedDashCooldownTimerHandle);
		World->GetTimerManager().SetTimer(PredictedDashTimerHandle, this, &ULSPlayerCombatComponent::FinishPredictedDash, GetDashDuration(), false);
		World->GetTimerManager().SetTimer(PredictedDashCooldownTimerHandle, this, &ULSPlayerCombatComponent::FinishPredictedDashCooldown, GetDashCooldown(), false);
	}

	return true;
}

bool ULSPlayerCombatComponent::CanRequestDashLocally() const
{
	const ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	const ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent();
	return OwnerCharacter && !OwnerCharacter->IsTemplate() && SharedCombatComponent && !SharedCombatComponent->IsDead() && !IsDashCooldownActive();
}

bool ULSPlayerCombatComponent::GetPendingDashDirection(FVector& OutDashDirection) const
{
	if (PendingDashDirection.IsNearlyZero())
	{
		return false;
	}

	OutDashDirection = PendingDashDirection;
	return true;
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

void ULSPlayerCombatComponent::FinishPredictedDash()
{
	if (ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter())
	{
		if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
		{
			MovementComponent->RemoveRootMotionSourceByID(PredictedDashRootMotionSourceID);
		}
	}

	PredictedDashRootMotionSourceID = 0;
	bPredictedDashInProgress = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PredictedDashTimerHandle);
	}
}

void ULSPlayerCombatComponent::FinishPredictedDashCooldown()
{
	bPredictedDashCooldownActive = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PredictedDashCooldownTimerHandle);
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

bool ULSPlayerCombatComponent::ApplyDashRootMotion(const FVector& DashDirection, uint16& OutRootMotionSourceID) const
{
	ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent();
	if (!OwnerCharacter || !SharedCombatComponent)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = SharedCombatComponent->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	FVector NormalizedDashDirection = DashDirection.GetSafeNormal2D();
	if (NormalizedDashDirection.IsNearlyZero())
	{
		NormalizedDashDirection = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	}

	const float DashSpeed = ASC->GetNumericAttribute(ULSCharacterAttributeSet::GetDashSpeedAttribute());
	const float DashDuration = GetDashDuration();

	TSharedPtr<FRootMotionSource_ConstantForce> RootMotion = MakeShared<FRootMotionSource_ConstantForce>();
	RootMotion->InstanceName = FName("PredictedDash");
	RootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	RootMotion->Priority = 5;
	RootMotion->Force = NormalizedDashDirection * DashSpeed;
	RootMotion->Duration = DashDuration;
	RootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
	{
		OutRootMotionSourceID = MovementComponent->ApplyRootMotionSource(RootMotion);
		return true;
	}

	return false;
}

float ULSPlayerCombatComponent::GetDashDuration() const
{
	if (const ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent())
	{
		if (const UAbilitySystemComponent* ASC = SharedCombatComponent->GetAbilitySystemComponent())
		{
			const float DashDuration = ASC->GetNumericAttribute(ULSCharacterAttributeSet::GetDashDurationAttribute());
			return DashDuration > 0.0f ? DashDuration : 0.3f;
		}
	}

	return 0.3f;
}

float ULSPlayerCombatComponent::GetDashCooldown() const
{
	if (const ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent())
	{
		if (const UAbilitySystemComponent* ASC = SharedCombatComponent->GetAbilitySystemComponent())
		{
			const float DashCooldown = ASC->GetNumericAttribute(ULSCharacterAttributeSet::GetDashCooldownAttribute());
			return DashCooldown > 0.0f ? DashCooldown : 1.0f;
		}
	}

	return 1.0f;
}

bool ULSPlayerCombatComponent::IsDashCooldownActive() const
{
	if (bPredictedDashCooldownActive)
	{
		return true;
	}

	if (const ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent())
	{
		if (const UAbilitySystemComponent* ASC = SharedCombatComponent->GetAbilitySystemComponent())
		{
			return ASC->HasMatchingGameplayTag(LSGameplayTags::Cooldown_Dash);
		}
	}

	return false;
}
