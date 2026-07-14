#include "Combat/LSCharacterCombatComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "AI/LSAIController.h"
#include "AI/LSMonsterSenseComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Characters/LSCharacterHitAudioData.h"
#include "Characters/Enemys/LSEnemyCharacter.h"
#include "Characters/LSPlayerCharacter.h"
#include "Combat/LSCombatStateComponent.h"
#include "Combat/LSStatusEffectComponent.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSCharacterSkillRow.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "LostSignal.h"
#include "Sound/SoundBase.h"
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

void ULSCharacterCombatComponent::RecordDamageExecutionResult(float DamageAmount, bool bCriticalHit) const
{
	LastDamageExecutionAmount = FMath::Max(0.0f, DamageAmount);
	bLastDamageExecutionCritical = bCriticalHit;
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

	ULSCharacterCombatComponent* TargetCombatComponent = TargetActor->FindComponentByClass<ULSCharacterCombatComponent>();
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
	LastDamageExecutionAmount = 0.0f;
	bLastDamageExecutionCritical = false;
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	const float AfterHealth = TargetASC->GetNumericAttribute(ULSCombatAttributeSet::GetCurrentHealthAttribute());
	const float ActualDamage = FMath::Max(0.0f, BeforeHealth - AfterHealth);
	const float DisplayDamage = LastDamageExecutionAmount > 0.0f ? LastDamageExecutionAmount : ActualDamage;
	const bool bCriticalHit = bLastDamageExecutionCritical;

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

	if (ALSEnemyCharacter* EnemyTarget = Cast<ALSEnemyCharacter>(TargetActor))
	{
		if (ALSPlayerCharacter* PlayerSource = Cast<ALSPlayerCharacter>(GetOwner()))
		{
			if (ULSMonsterSenseComponent* SenseComponent = EnemyTarget->GetMonsterSenseComponent())
			{
				SenseComponent->SetCurrentTargetFromDamage(PlayerSource);
			}
		}
	}

	if (TargetCombatComponent && DisplayDamage > 0.0f)
	{
		FLSDamageNumberPayload Payload;
		Payload.DamageAmount = DisplayDamage;
		Payload.WorldLocation = FVector_NetQuantize(TargetActor->GetActorLocation() + TargetCombatComponent->DamageNumberWorldOffset);
		Payload.bCritical = bCriticalHit;
		TargetCombatComponent->BroadcastDamageNumberToPlayers(Payload);
	}

	return true;
}

bool ULSCharacterCombatComponent::ApplyStatusEffectFromRow(int32 StatusID, ELSCharacterSkillEffectTarget EffectTarget, float Duration, AActor* HitTarget) const
{
	if (StatusID <= 0)
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	AActor* EffectActor = nullptr;
	switch (EffectTarget)
	{
	case ELSCharacterSkillEffectTarget::Self:
		EffectActor = OwnerActor;
		break;
	case ELSCharacterSkillEffectTarget::Target:
		EffectActor = HitTarget;
		break;
	case ELSCharacterSkillEffectTarget::Ally:
		// 아군 타겟팅 정책이 정해지면 확장한다.
		UE_LOG(LogLS, Verbose, TEXT("StatusEffect: Effect_Target=Ally는 아직 미지원 (StatusID=%d)."), StatusID);
		return false;
	default:
		return false; // None
	}

	ALSCharacterBase* EffectCharacter = Cast<ALSCharacterBase>(EffectActor);
	ULSStatusEffectComponent* StatusEffectComponent = EffectCharacter ? EffectCharacter->GetStatusEffectComponent() : nullptr;
	if (!StatusEffectComponent)
	{
		return false;
	}

	return StatusEffectComponent->ApplyStatusEffectByID(StatusID, OwnerActor, Duration);
}

void ULSCharacterCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	BindHealthDelegates();
	BindStateTagDelegates();
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

void ULSCharacterCombatComponent::BindStateTagDelegates()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	ASC->RegisterGameplayTagEvent(LSGameplayTags::State_Stunned, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &ULSCharacterCombatComponent::HandleStunnedTagChanged);
}

void ULSCharacterCombatComponent::HandleCurrentHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshDeathState();

	// 데미지로 체력이 줄었으면 피격 오디오(재질 임팩트음 + 보이스). 치사타도 일단 피격 처리(사망 보이스 미보유).
	if (ChangeData.NewValue < ChangeData.OldValue)
	{
		PlayHitAudio();
	}
}

void ULSCharacterCombatComponent::PlayHitAudio()
{
	const ALSCharacterBase* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || !HitAudioData)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;

	// 재질 임팩트음 — 피격자 데이터가 단일 출처라 종류·재질별 분기가 데이터 교체로 끝난다.
	if (HitAudioData->HitImpactSound && (Now - LastImpactTime) >= HitAudioData->ImpactMinInterval)
	{
		LastImpactTime = Now;
		FireHitAudioCue(LSGameplayTags::GameplayCue_Combat_Hit, HitAudioData->HitImpactSound);
	}

	// 피격 보이스 — 서버에서 변주를 골라 파라미터로 넘겨 전 클라가 같은 클립을 듣게 한다.
	if (HitAudioData->HitVoices.Num() > 0 && (Now - LastVoiceTime) >= HitAudioData->VoiceMinInterval)
	{
		if (USoundBase* Voice = HitAudioData->HitVoices[FMath::RandHelper(HitAudioData->HitVoices.Num())].Get())
		{
			LastVoiceTime = Now;
			FireHitAudioCue(LSGameplayTags::GameplayCue_Voice, Voice);
		}
	}
}

void ULSCharacterCombatComponent::FireHitAudioCue(FGameplayTag CueTag, USoundBase* Sound) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	AActor* OwnerActor = GetOwner();
	if (!ASC || !OwnerActor || !Sound)
	{
		return;
	}

	FGameplayCueParameters CueParams;
	CueParams.SourceObject = Sound;
	CueParams.Location = OwnerActor->GetActorLocation();
	CueParams.Instigator = OwnerActor;

	ASC->ExecuteGameplayCue(CueTag, CueParams);
}

void ULSCharacterCombatComponent::HandleStunnedTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	UE_LOG(LogLS, Log, TEXT("%s stun tag changed. Tag=%s Count=%d"),
		*GetNameSafe(GetOwner()),
		*CallbackTag.ToString(),
		NewCount);

	HandleStunStateChanged(NewCount > 0);
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

	// 파생 클래스 사망 후처리 위임(몬스터: 콜리전 해제). 모든 머신에서 실행.
	OwnerCharacter->OnDeathStateChanged(bIsDead);

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

	// 진행 중인 어빌리티(공격 등)를 즉시 취소. 몽타주 정지·Combat_Attacking 해제는 각 어빌리티의 EndAbility가 정리한다.
	// 취소하지 않으면 Combat_Attacking이 남아 StateTree Attack→Dead 전이가 공격 몽타주 종료까지 지연된다.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->CancelAllAbilities();
	}

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

void ULSCharacterCombatComponent::HandleStunStateChanged(bool bIsStunned)
{
	ALSCharacterBase* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || IsDead())
	{
		return;
	}

	ULSCombatStateComponent* CombatStateComponent = OwnerCharacter->GetCombatStateComponent();
	if (bIsStunned)
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			FGameplayTagContainer CancelTags;
			CancelTags.AddTag(LSGameplayTags::Combat_Attacking);
			ASC->CancelAbilities(&CancelTags);
		}

		if (CombatStateComponent)
		{
			CombatStateComponent->BeginAction(ELSCombatActionState::Stunned, ELSCombatActionPhase::Active);
		}

		if (OwnerCharacter->HasAuthority())
		{
			if (AController* Controller = OwnerCharacter->GetController())
			{
				Controller->StopMovement();
			}

			if (AAIController* AIController = Cast<AAIController>(OwnerCharacter->GetController()))
			{
				if (AIController->BrainComponent)
				{
					AIController->BrainComponent->StopLogic(TEXT("Stunned"));
				}
			}
		}

		return;
	}

	if (CombatStateComponent && CombatStateComponent->GetCurrentState() == ELSCombatActionState::Stunned)
	{
		CombatStateComponent->EndAction();
	}

	if (OwnerCharacter->HasAuthority())
	{
		if (ALSAIController* LSAIController = Cast<ALSAIController>(OwnerCharacter->GetController()))
		{
			LSAIController->TryStartStateTreeLogic();
		}
		else if (AAIController* AIController = Cast<AAIController>(OwnerCharacter->GetController()))
		{
			if (AIController->BrainComponent && !AIController->BrainComponent->IsRunning())
			{
				AIController->BrainComponent->RestartLogic();
			}
		}
	}
}

void ULSCharacterCombatComponent::BroadcastDamageNumberToPlayers(const FLSDamageNumberPayload& Payload) const
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Payload.DamageAmount <= 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(It->Get());
		if (PlayerController)
		{
			PlayerController->ShowDamageNumber(Payload);
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
