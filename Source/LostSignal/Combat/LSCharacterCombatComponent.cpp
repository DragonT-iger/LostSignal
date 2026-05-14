#include "Combat/LSCharacterCombatComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Characters/LSEnemyCharacter.h"
#include "Characters/LSPlayerCharacter.h"
#include "Combat/LSCombatStateComponent.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "LostSignal.h"
#include "TimerManager.h"

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

ELSTenacityTier ULSCharacterCombatComponent::GetCurrentTenacityTier() const
{
	if (HasCombatTag(LSGameplayTags::State_Invincible))
	{
		return ELSTenacityTier::Invincible;
	}

	if (HasCombatTag(LSGameplayTags::State_SuperArmor))
	{
		return ELSTenacityTier::SuperArmor;
	}

	return ELSTenacityTier::Normal;
}

FLSImpactResolution ULSCharacterCombatComponent::ResolveIncomingImpact(ELSBreakPowerTier BreakPowerTier) const
{
	FLSImpactResolution Resolution;
	Resolution.TargetTenacity = GetCurrentTenacityTier();
	Resolution.IncomingBreakPower = BreakPowerTier;
	Resolution.bDamageBlocked = Resolution.TargetTenacity == ELSTenacityTier::Invincible;
	Resolution.bCrowdControlBlocked = static_cast<int32>(BreakPowerTier) < static_cast<int32>(Resolution.TargetTenacity);
	Resolution.bImpactAllowed = !Resolution.bDamageBlocked && !Resolution.bCrowdControlBlocked;
	return Resolution;
}

bool ULSCharacterCombatComponent::CanApplyCrowdControl(ELSBreakPowerTier BreakPowerTier) const
{
	return !ResolveIncomingImpact(BreakPowerTier).bCrowdControlBlocked;
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

bool ULSCharacterCombatComponent::ApplyKnockback(const FVector& Direction, float Speed, float Duration, float UpSpeed)
{
	ALSCharacterBase* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || IsDead() || Speed <= 0.0f || Duration <= 0.0f)
	{
		return false;
	}

	FVector KnockbackDirection = Direction.GetSafeNormal2D();
	if (KnockbackDirection.IsNearlyZero())
	{
		KnockbackDirection = -OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	}

	UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	if (KnockbackDirection.IsNearlyZero() || !MovementComponent)
	{
		return false;
	}

	ClearKnockback();
	SetCombatTagActive(LSGameplayTags::State_Knockback, true);
	bKnockbackActive = true;

	if (ULSCombatStateComponent* CombatStateComponent = OwnerCharacter->GetCombatStateComponent())
	{
		CombatStateComponent->BeginAction(ELSCombatActionState::HitReaction, ELSCombatActionPhase::Active);
	}

	TSharedPtr<FRootMotionSource_ConstantForce> RootMotion = MakeShared<FRootMotionSource_ConstantForce>();
	RootMotion->InstanceName = FName("Knockback");
	RootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	RootMotion->Priority = 8;
	RootMotion->Force = (KnockbackDirection * Speed) + FVector(0.0f, 0.0f, UpSpeed);
	RootMotion->Duration = Duration;
	RootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
	KnockbackRootMotionSourceID = MovementComponent->ApplyRootMotionSource(RootMotion);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(KnockbackTimerHandle, this, &ULSCharacterCombatComponent::FinishKnockback, Duration, false);
	}

	return true;
}

bool ULSCharacterCombatComponent::ApplyDamageEffectToTarget(
	AActor* TargetActor,
	TSubclassOf<UGameplayEffect> DamageEffectClass,
	float EffectLevel,
	float FixedDamage,
	float AttackCoefficient,
	bool bCanCrit,
	ELSBreakPowerTier BreakPowerTier) const
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!SourceASC || !TargetASC || !DamageEffectClass || !CanDamageTarget(TargetActor))
	{
		return false;
	}

	const ULSCharacterCombatComponent* TargetCombatComponent = TargetActor->FindComponentByClass<ULSCharacterCombatComponent>();
	const FLSImpactResolution ImpactResolution = TargetCombatComponent
		? TargetCombatComponent->ResolveIncomingImpact(BreakPowerTier)
		: FLSImpactResolution();

	if (ImpactResolution.bDamageBlocked)
	{
		UE_LOG(
			LogLS,
			Log,
			TEXT("DamageBlocked %s -> %s | BreakPower=%d TargetTenacity=%d"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(TargetActor),
			static_cast<int32>(BreakPowerTier),
			static_cast<int32>(ImpactResolution.TargetTenacity));
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, EffectLevel, EffectContext);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(LSGameplayTags::Data_Damage_Fixed, FixedDamage);
	SpecHandle.Data->SetSetByCallerMagnitude(LSGameplayTags::Data_Damage_AttackCoefficient, AttackCoefficient);
	SpecHandle.Data->SetSetByCallerMagnitude(LSGameplayTags::Data_Damage_CanCrit, bCanCrit ? 1.0f : 0.0f);

	const float BeforeHealth = TargetASC->GetNumericAttribute(ULSCombatAttributeSet::GetCurrentHealthAttribute());
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	const float AfterHealth = TargetASC->GetNumericAttribute(ULSCombatAttributeSet::GetCurrentHealthAttribute());

	UE_LOG(
		LogLS,
		Log,
		TEXT("DamageApply %s -> %s | GE=%s Level=%.1f Fixed=%.2f Coef=%.2f CanCrit=%d BreakPower=%d TargetTenacity=%d CCBlocked=%d | HP %.1f -> %.1f (Delta %.1f)"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(TargetActor),
		*GetNameSafe(DamageEffectClass),
		EffectLevel,
		FixedDamage,
		AttackCoefficient,
		bCanCrit ? 1 : 0,
		static_cast<int32>(BreakPowerTier),
		static_cast<int32>(ImpactResolution.TargetTenacity),
		ImpactResolution.bCrowdControlBlocked ? 1 : 0,
		BeforeHealth,
		AfterHealth,
		AfterHealth - BeforeHealth);

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

	if (bCachedIsDead == bShouldBeDead)
	{
		return;
	}

	bCachedIsDead = bShouldBeDead;
	SetCombatTagActive(LSGameplayTags::State_Dead, bShouldBeDead);
	HandleDeathStateChanged(bShouldBeDead);
}

void ULSCharacterCombatComponent::HandleDeathStateChanged(bool bIsDead)
{
	ALSCharacterBase* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return;
	}

	if (ULSCombatStateComponent* CombatStateComponent = OwnerCharacter->GetCombatStateComponent())
	{
		if (bIsDead)
		{
			CombatStateComponent->BeginAction(ELSCombatActionState::Dead, ELSCombatActionPhase::None);
		}
		else if (CombatStateComponent->GetCurrentState() == ELSCombatActionState::Dead)
		{
			CombatStateComponent->EndAction();
		}
	}

	if (!bIsDead)
	{
		if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
		{
			MovementComponent->SetMovementMode(MOVE_Walking);
		}
		return;
	}

	ClearKnockback();

	if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	if (OwnerCharacter->HasAuthority())
	{
		if (AController* Controller = OwnerCharacter->GetController())
		{
			Controller->StopMovement();
		}
	}
}

void ULSCharacterCombatComponent::FinishKnockback()
{
	ClearKnockback();
}

void ULSCharacterCombatComponent::ClearKnockback()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(KnockbackTimerHandle);
	}

	if (bKnockbackActive)
	{
		SetCombatTagActive(LSGameplayTags::State_Knockback, false);
		bKnockbackActive = false;

		if (ALSCharacterBase* OwnerCharacter = GetOwnerCharacter())
		{
			if (ULSCombatStateComponent* CombatStateComponent = OwnerCharacter->GetCombatStateComponent())
			{
				if (CombatStateComponent->GetCurrentState() == ELSCombatActionState::HitReaction)
				{
					CombatStateComponent->EndAction();
				}
			}
		}
	}

	if (KnockbackRootMotionSourceID != 0)
	{
		if (ALSCharacterBase* OwnerCharacter = GetOwnerCharacter())
		{
			if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
			{
				MovementComponent->RemoveRootMotionSourceByID(KnockbackRootMotionSourceID);
			}
		}

		KnockbackRootMotionSourceID = 0;
	}
}

bool ULSCharacterCombatComponent::CanDamageTarget(AActor* TargetActor) const
{
	if (!TargetActor || TargetActor == GetOwner())
	{
		return false;
	}

	if (IsFriendlyTarget(TargetActor))
	{
		return false;
	}

	if (const UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
	{
		return !TargetASC->HasMatchingGameplayTag(LSGameplayTags::State_Dead);
	}

	return false;
}

bool ULSCharacterCombatComponent::IsFriendlyTarget(AActor* TargetActor) const
{
	const AActor* SourceActor = GetOwner();
	if (!SourceActor || !TargetActor)
	{
		return false;
	}

	const bool bSourceIsPlayer = SourceActor->IsA<ALSPlayerCharacter>();
	const bool bTargetIsPlayer = TargetActor->IsA<ALSPlayerCharacter>();
	if (bSourceIsPlayer || bTargetIsPlayer)
	{
		return bSourceIsPlayer == bTargetIsPlayer;
	}

	const bool bSourceIsEnemy = SourceActor->IsA<ALSEnemyCharacter>();
	const bool bTargetIsEnemy = TargetActor->IsA<ALSEnemyCharacter>();
	if (bSourceIsEnemy || bTargetIsEnemy)
	{
		return bSourceIsEnemy == bTargetIsEnemy;
	}

	return false;
}
