#include "Combat/LSPlayerCombatComponent.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Characters/LSCharacterBase.h"
#include "Combat/LSAimComponent.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Combat/LSCombatStateComponent.h"
#include "Combat/LSCombatTypes.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GAS/Abilities/LSGA_Dash.h"
#include "GAS/Abilities/LSGA_PlayerBasicAttack.h"
#include "GAS/Effects/LSGE_PlayerBasicDamage.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "TimerManager.h"

ULSPlayerCombatComponent::ULSPlayerCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	BasicAttackAbilityClass = ULSGA_PlayerBasicAttack::StaticClass();
	DashAbilityClass = ULSGA_Dash::StaticClass();
	BasicAttackDamageEffectClass = ULSGE_PlayerBasicDamage::StaticClass();
}

bool ULSPlayerCombatComponent::RequestBasicAttack()
{
	ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent();
	ULSCombatStateComponent* CombatStateComponent = ResolveCombatStateComponent();
	if (!OwnerCharacter || OwnerCharacter->IsTemplate() || !SharedCombatComponent || !CombatStateComponent || SharedCombatComponent->IsDead())
	{
		UE_LOG(LogLS, Warning, TEXT("%s basic attack rejected. Owner=%s SharedCombat=%s CombatState=%s Dead=%d"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(OwnerCharacter),
			*GetNameSafe(SharedCombatComponent),
			*GetNameSafe(CombatStateComponent),
			SharedCombatComponent && SharedCombatComponent->IsDead() ? 1 : 0);
		return false;
	}

	if (!AttackMontage)
	{
		UE_LOG(LogLS, Warning, TEXT("%s basic attack rejected because AttackMontage is not set."),
			*GetNameSafe(OwnerCharacter));
		return false;
	}

	if (!OwnerCharacter->HasAuthority())
	{
		return true;
	}

	if (ULSGA_PlayerBasicAttack* ActiveAttackAbility = FindActiveBasicAttackAbility())
	{
		CombatStateComponent->ClearBufferedCommand();
		ActiveAttackAbility->QueueComboInput();
		return true;
	}

	if (!CombatStateComponent->TrySubmitCommand(ELSCombatCommandType::BasicAttack))
	{
		UE_LOG(LogLS, Warning, TEXT("%s basic attack rejected by combat state. State=%d Phase=%d"),
			*GetNameSafe(OwnerCharacter),
			static_cast<int32>(CombatStateComponent->GetCurrentState()),
			static_cast<int32>(CombatStateComponent->GetCurrentPhase()));
		return false;
	}

	UAbilitySystemComponent* ASC = SharedCombatComponent->GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogLS, Warning, TEXT("%s basic attack rejected because ASC is missing."),
			*GetNameSafe(OwnerCharacter));
		return false;
	}

	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(LSGameplayTags::Ability_PlayerBasicAttack);
	const bool bActivated = ASC->TryActivateAbilitiesByTag(AbilityTags);
	if (!bActivated)
	{
		UE_LOG(LogLS, Warning, TEXT("%s basic attack ability activation failed. Tag=%s"),
			*GetNameSafe(OwnerCharacter),
			*LSGameplayTags::Ability_PlayerBasicAttack.GetTag().ToString());
	}

	return bActivated;
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
	ULSCombatStateComponent* CombatStateComponent = ResolveCombatStateComponent();
	if (!OwnerCharacter || !SharedCombatComponent || !CombatStateComponent || SharedCombatComponent->IsDead())
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

	if (!CombatStateComponent->TrySubmitCommand(ELSCombatCommandType::Dash))
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
	const bool bActivated = ASC->TryActivateAbilitiesByTag(AbilityTags);
	if (bActivated)
	{
		CancelAttackForDash();
		CombatStateComponent->BeginAction(ELSCombatActionState::Dash, ELSCombatActionPhase::Active);
	}

	return bActivated;
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
	CancelAttackForDash();

	if (ULSCombatStateComponent* CombatStateComponent = ResolveCombatStateComponent())
	{
		CombatStateComponent->BeginAction(ELSCombatActionState::Dash, ELSCombatActionPhase::Active);
	}

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
	const ULSCombatStateComponent* CombatStateComponent = ResolveCombatStateComponent();
	return OwnerCharacter && !OwnerCharacter->IsTemplate() && SharedCombatComponent && CombatStateComponent &&
		!SharedCombatComponent->IsDead() && !IsDashCooldownActive() && CombatStateComponent->CanExecuteCommand(ELSCombatCommandType::Dash);
}

bool ULSPlayerCombatComponent::SubmitDashInput(const FVector& DashDirection, bool& bOutShouldExecuteImmediately)
{
	bOutShouldExecuteImmediately = false;

	const ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	const ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent();
	ULSCombatStateComponent* CombatStateComponent = ResolveCombatStateComponent();
	if (!OwnerCharacter || OwnerCharacter->IsTemplate() || !SharedCombatComponent || !CombatStateComponent || SharedCombatComponent->IsDead() || IsDashCooldownActive())
	{
		return false;
	}

	PendingDashDirection = DashDirection.GetSafeNormal2D();
	if (PendingDashDirection.IsNearlyZero())
	{
		PendingDashDirection = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	}

	if (CombatStateComponent->CanExecuteCommand(ELSCombatCommandType::Dash))
	{
		bOutShouldExecuteImmediately = true;
		return true;
	}

	CombatStateComponent->TrySubmitCommand(ELSCombatCommandType::Dash);

	ELSCombatCommandType BufferedCommand = ELSCombatCommandType::BasicAttack;
	return CombatStateComponent->PeekBufferedCommand(BufferedCommand) && BufferedCommand == ELSCombatCommandType::Dash;
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
	if (!SharedCombatComponent || !OwnerCharacter || SharedCombatComponent->IsDead())
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
	const int32 ValidHitCount = ExecuteMeleeHit(AttackDirection);
	if (ValidHitCount > 0)
	{
		const int32 ComboIndex = FindActiveBasicAttackAbility()
			? FindActiveBasicAttackAbility()->GetCurrentComboIndex()
			: INDEX_NONE;

		if (ULSPlayerSkillComponent* SkillComponent = OwnerCharacter->FindComponentByClass<ULSPlayerSkillComponent>())
		{
			SkillComponent->HandleBasicAttackHit(ComboIndex, ValidHitCount);
		}
	}

	SharedCombatComponent->SetCombatTagActive(LSGameplayTags::Combat_AttackActive, false);
}

void ULSPlayerCombatComponent::HandleCombatActionEnd(ELSCombatActionState ExpectedState)
{
	const ULSCombatStateComponent* CombatStateComponent = ResolveCombatStateComponent();
	if (!CombatStateComponent || CombatStateComponent->GetCurrentState() != ExpectedState)
	{
		return;
	}

	if (ExpectedState == ELSCombatActionState::BasicAttack)
	{
		FinishAttack();
	}
}

bool ULSPlayerCombatComponent::IsAttackInProgress() const
{
	const ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent();
	return SharedCombatComponent && SharedCombatComponent->HasCombatTag(LSGameplayTags::Combat_Attacking);
}

void ULSPlayerCombatComponent::ResetBasicAttackHit()
{
	bAttackHitConsumed = false;
}

void ULSPlayerCombatComponent::SetPendingBasicAttackComboIndexOverride(int32 ComboIndex, float ExpireSeconds)
{
	ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || ComboIndex == INDEX_NONE)
	{
		return;
	}

	PendingComboIndexOverride = ComboIndex;
	if (ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent())
	{
		SharedCombatComponent->SetCombatTagActive(LSGameplayTags::Combat_NextAttack_ComboIndexOverride, true);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PendingComboIndexOverrideTimerHandle);
		if (ExpireSeconds > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				PendingComboIndexOverrideTimerHandle,
				this,
				&ULSPlayerCombatComponent::ClearPendingBasicAttackComboIndexOverride,
				ExpireSeconds,
				false);
		}
	}

	UE_LOG(LogLS, Log, TEXT("%s reserved next basic attack combo index override. ComboIndex=%d Window=%.2f"),
		*GetNameSafe(OwnerCharacter),
		PendingComboIndexOverride,
		ExpireSeconds);
}

bool ULSPlayerCombatComponent::ConsumePendingBasicAttackComboIndexOverride(int32& OutComboIndex)
{
	if (PendingComboIndexOverride == INDEX_NONE)
	{
		return false;
	}

	OutComboIndex = PendingComboIndexOverride;
	ClearPendingBasicAttackComboIndexOverride();
	return true;
}

void ULSPlayerCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter(); OwnerCharacter && OwnerCharacter->HasAuthority())
	{
		if (BasicAttackAbilityClass)
		{
			OwnerCharacter->GrantAbility(BasicAttackAbilityClass);
		}

		if (DashAbilityClass)
		{
			OwnerCharacter->GrantAbility(DashAbilityClass);
		}
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

ULSCombatStateComponent* ULSPlayerCombatComponent::ResolveCombatStateComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<ULSCombatStateComponent>() : nullptr;
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

	if (ULSCombatStateComponent* CombatStateComponent = ResolveCombatStateComponent())
	{
		if (CombatStateComponent->GetCurrentState() == ELSCombatActionState::BasicAttack)
		{
			CombatStateComponent->EndAction();
		}
	}

	TryExecuteBufferedCommand();
}

void ULSPlayerCombatComponent::CancelAttackForDash()
{
	ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	if (OwnerCharacter && AttackMontage)
	{
		if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr)
		{
			if (AnimInstance->Montage_IsPlaying(AttackMontage))
			{
				AnimInstance->Montage_Stop(AttackCancelBlendOutTime, AttackMontage);
			}
		}
	}

	if (ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent())
	{
		SharedCombatComponent->SetCombatTagActive(LSGameplayTags::Combat_Attacking, false);
		SharedCombatComponent->SetCombatTagActive(LSGameplayTags::Combat_AttackActive, false);
	}

	bAttackHitConsumed = true;
}

ULSGA_PlayerBasicAttack* ULSPlayerCombatComponent::FindActiveBasicAttackAbility() const
{
	const ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent();
	UAbilitySystemComponent* ASC = SharedCombatComponent ? SharedCombatComponent->GetAbilitySystemComponent() : nullptr;
	return ULSGA_PlayerBasicAttack::FindActiveBasicAttackAbility(ASC);
}

void ULSPlayerCombatComponent::TryExecuteBufferedCommand()
{
	ULSCombatStateComponent* CombatStateComponent = ResolveCombatStateComponent();
	ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	if (!CombatStateComponent || !OwnerCharacter)
	{
		return;
	}

	ELSCombatCommandType BufferedCommand = ELSCombatCommandType::BasicAttack;
	if (!CombatStateComponent->ConsumeBufferedCommand(BufferedCommand))
	{
		return;
	}

	switch (BufferedCommand)
	{
	case ELSCombatCommandType::BasicAttack:
		// Basic-attack combo chaining is handled inside ULSGA_PlayerBasicAttack.
		// Replaying a buffered BasicAttack here restarts the combo from Attack_1.
		break;

	case ELSCombatCommandType::Dash:
	{
		FVector DashDirection = PendingDashDirection;
		if (DashDirection.IsNearlyZero())
		{
			DashDirection = OwnerCharacter->GetActorForwardVector();
		}

		if (OwnerCharacter->HasAuthority())
		{
			RequestDash(DashDirection);
		}
		else if (OwnerCharacter->IsLocallyControlled())
		{
			PredictDashMovement(DashDirection);
		}
		break;
	}

	default:
		break;
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

	if (ULSCombatStateComponent* CombatStateComponent = ResolveCombatStateComponent())
	{
		if (CombatStateComponent->GetCurrentState() == ELSCombatActionState::Dash)
		{
			CombatStateComponent->EndAction();
		}
	}

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

void ULSPlayerCombatComponent::ClearPendingBasicAttackComboIndexOverride()
{
	if (PendingComboIndexOverride == INDEX_NONE)
	{
		return;
	}

	PendingComboIndexOverride = INDEX_NONE;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PendingComboIndexOverrideTimerHandle);
	}

	if (ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent())
	{
		SharedCombatComponent->SetCombatTagActive(LSGameplayTags::Combat_NextAttack_ComboIndexOverride, false);
	}
}

int32 ULSPlayerCombatComponent::ExecuteMeleeHit(const FVector& AttackDirection)
{
	ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	ULSCharacterCombatComponent* SharedCombatComponent = ResolveSharedCombatComponent();
	if (!OwnerCharacter || !SharedCombatComponent || !OwnerCharacter->HasAuthority())
	{
		return 0;
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

		if (SharedCombatComponent->ApplyDamageEffectToTarget(
			HitActor,
			BasicAttackDamageEffectClass,
			DamageEffectLevel,
			BasicAttackFixedDamage,
			BasicAttackAttackCoefficient,
			bBasicAttackCanCrit,
			BasicAttackBreakPower))
		{
			UniqueTargets.Add(HitActor);
		}
	}

	return UniqueTargets.Num();
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
