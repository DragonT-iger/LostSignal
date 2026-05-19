#include "Skills/LSPlayerSkillComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Characters/LSEnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GAS/Abilities/Character1/LSGA_Bypass.h"
#include "GAS/Abilities/Character1/LSGA_Execution.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"
#include "Skills/LSSkillDataAsset.h"
#include "Skills/Preview/LSSkillPreviewComponent.h"
#include "TimerManager.h"

ULSPlayerSkillComponent::ULSPlayerSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool ULSPlayerSkillComponent::BeginSkillPreview(ELSPlayerSkillSlot Slot)
{
	if (!CanUseLocalPreview())
	{
		return false;
	}

	ULSSkillDataAsset* SkillData = GetSkillData(Slot);
	ULSSkillPreviewComponent* PreviewComponent = ResolvePreviewComponent();
	if (!SkillData || !PreviewComponent)
	{
		return false;
	}

	if (IsSkillCooldownActive(SkillData))
	{
		LogSkillCooldownBlocked(SkillData, TEXT("Preview"));
		return false;
	}

	if (ActiveSkillData)
	{
		PreviewComponent->EndAreaPreview();
	}

	ActiveSkillData = nullptr;
	ActiveSlot = Slot;

	if (!PreviewComponent->BeginAreaPreview(SkillData->BuildPreviewSpec()))
	{
		return false;
	}

	ActiveSkillData = SkillData;
	return true;
}

void ULSPlayerSkillComponent::UpdateActiveSkillPreview(const FVector& WorldLocation, const FRotator& WorldRotation)
{
	if (!ActiveSkillData)
	{
		return;
	}

	if (ULSSkillPreviewComponent* PreviewComponent = ResolvePreviewComponent())
	{
		PreviewComponent->UpdateAreaPreview(WorldLocation, WorldRotation);
	}
}

bool ULSPlayerSkillComponent::ConfirmActiveSkillPreview(ELSPlayerSkillSlot Slot)
{
	if (!ActiveSkillData || ActiveSlot != Slot)
	{
		return false;
	}

	if (ULSSkillPreviewComponent* PreviewComponent = ResolvePreviewComponent())
	{
		PreviewComponent->EndAreaPreview();
	}

	ActiveSkillData = nullptr;
	return true;
}

bool ULSPlayerSkillComponent::ConfirmAnyActiveSkillPreview(const FVector& TargetLocation, const FRotator& AimRotation)
{
	if (!ActiveSkillData)
	{
		return false;
	}

	const ELSPlayerSkillSlot SlotToActivate = ActiveSlot;
	const bool bConfirmed = ConfirmActiveSkillPreview(SlotToActivate);
	if (!bConfirmed)
	{
		return false;
	}

	if (const AActor* OwnerActor = GetOwner())
	{
		if (OwnerActor->HasAuthority())
		{
			return ActivateSkillOnServer(SlotToActivate, TargetLocation, AimRotation.Yaw);
		}

		TryPredictFastMovementSkill(GetSkillData(SlotToActivate), TargetLocation, AimRotation.Yaw);
		ServerRequestActivateSkill(SlotToActivate, TargetLocation, AimRotation.Yaw);
	}

	return true;
}

void ULSPlayerSkillComponent::CancelActiveSkillPreview(ELSPlayerSkillSlot Slot)
{
	if (!ActiveSkillData || ActiveSlot != Slot)
	{
		return;
	}

	CancelAnyActiveSkillPreview();
}

void ULSPlayerSkillComponent::CancelAnyActiveSkillPreview()
{
	if (ULSSkillPreviewComponent* PreviewComponent = ResolvePreviewComponent())
	{
		PreviewComponent->EndAreaPreview();
	}

	ActiveSkillData = nullptr;
}

ULSSkillDataAsset* ULSPlayerSkillComponent::GetSkillData(ELSPlayerSkillSlot Slot) const
{
	const FLSPlayerSkillSlotSpec* SlotSpec = SkillSlots.Find(Slot);
	return SlotSpec ? SlotSpec->SkillData.Get() : nullptr;
}

bool ULSPlayerSkillComponent::SetSkillData(ELSPlayerSkillSlot Slot, ULSSkillDataAsset* NewSkillData)
{
	if (!NewSkillData)
	{
		return false;
	}

	if (ActiveSkillData && ActiveSlot == Slot)
	{
		CancelAnyActiveSkillPreview();
	}

	FLSPlayerSkillSlotSpec& SlotSpec = SkillSlots.FindOrAdd(Slot);
	SlotSpec.SkillData = NewSkillData;

	UE_LOG(LogLS, Log, TEXT("%s changed skill slot data. Slot=%d Skill=%s"),
		*GetNameSafe(GetOwner()),
		static_cast<int32>(Slot),
		*GetNameSafe(NewSkillData));

	return true;
}

bool ULSPlayerSkillComponent::ApplySkillEnhancementByIndex(ELSPlayerSkillSlot Slot, int32 EnhancementIndex)
{
	ULSSkillDataAsset* CurrentSkillData = GetSkillData(Slot);
	ULSSkillDataAsset* EnhancedSkillData = CurrentSkillData ? CurrentSkillData->GetEnhancementVariant(EnhancementIndex) : nullptr;
	if (!EnhancedSkillData)
	{
		UE_LOG(LogLS, Warning, TEXT("%s failed to apply skill enhancement. Slot=%d Index=%d CurrentSkill=%s"),
			*GetNameSafe(GetOwner()),
			static_cast<int32>(Slot),
			EnhancementIndex,
			*GetNameSafe(CurrentSkillData));
		return false;
	}

	return SetSkillData(Slot, EnhancedSkillData);
}

bool ULSPlayerSkillComponent::GetActivePreviewSpec(FLSSkillAreaPreviewSpec& OutPreviewSpec) const
{
	if (!ActiveSkillData)
	{
		return false;
	}

	OutPreviewSpec = ActiveSkillData->BuildPreviewSpec();
	return true;
}

void ULSPlayerSkillComponent::HandleBasicAttackHit(int32 ComboIndex, int32 ValidHitCount)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || ValidHitCount <= 0)
	{
		return;
	}

	for (ULSSkillDataAsset* PassiveSkillData : PassiveSkills)
	{
		if (!PassiveSkillData)
		{
			continue;
		}

		TrySendPassiveGameplayEvent(PassiveSkillData, ComboIndex);
	}
}

bool ULSPlayerSkillComponent::ConsumePendingAbilityContext(TSubclassOf<UGameplayAbility> AbilityClass, FLSSkillActivationContext& OutContext)
{
	if (!AbilityClass || PendingAbilityClass != AbilityClass || !PendingAbilityContext.SkillData)
	{
		return false;
	}

	OutContext = PendingAbilityContext;
	PendingAbilityContext = FLSSkillActivationContext();
	PendingAbilityClass = nullptr;
	return true;
}

bool ULSPlayerSkillComponent::ApplySkillCooldown(const ULSSkillDataAsset* SkillData) const
{
	AActor* OwnerActor = GetOwner();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	if (!OwnerActor || !OwnerActor->HasAuthority() || !ASC || !SkillData)
	{
		return false;
	}

	const FGameplayTag CooldownTag = SkillData->GetCooldownTag();
	const float BaseDuration = SkillData->GetCooldownDuration();
	if (!CooldownTag.IsValid() || BaseDuration <= 0.0f || !SkillData->CooldownEffectClass)
	{
		return false;
	}

	const float CooldownReduction = ASC->GetNumericAttribute(ULSCharacterAttributeSet::GetCooldownReductionAttribute());
	const float ReductionRatio = CooldownReduction > 1.0f ? CooldownReduction * 0.01f : CooldownReduction;
	const float FinalDuration = BaseDuration * FMath::Clamp(1.0f - ReductionRatio, 0.0f, 1.0f);
	if (FinalDuration <= 0.0f)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(SkillData);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(SkillData->CooldownEffectClass, 1.0f, EffectContext);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	SpecHandle.Data->SetDuration(FinalDuration, true);
	SpecHandle.Data->DynamicGrantedTags.AddTag(CooldownTag);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	UE_LOG(LogLS, Log, TEXT("%s applied skill cooldown. Skill=%s Tag=%s Duration=%.2f Base=%.2f Reduction=%.2f"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(SkillData),
		*CooldownTag.ToString(),
		FinalDuration,
		BaseDuration,
		CooldownReduction);

	return true;
}

bool ULSPlayerSkillComponent::IsSkillCooldownActive(const ULSSkillDataAsset* SkillData) const
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	const FGameplayTag CooldownTag = SkillData ? SkillData->GetCooldownTag() : FGameplayTag();
	return ASC && CooldownTag.IsValid() && ASC->HasMatchingGameplayTag(CooldownTag);
}

float ULSPlayerSkillComponent::GetSkillCooldownRemaining(const ULSSkillDataAsset* SkillData) const
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	const FGameplayTag CooldownTag = SkillData ? SkillData->GetCooldownTag() : FGameplayTag();
	if (!ASC || !CooldownTag.IsValid())
	{
		return 0.0f;
	}

	FGameplayTagContainer QueryTags;
	QueryTags.AddTag(CooldownTag);

	float RemainingTime = 0.0f;
	for (const TPair<float, float>& TimePair : ASC->GetActiveEffectsTimeRemainingAndDuration(FGameplayEffectQuery::MakeQuery_MatchAllOwningTags(QueryTags)))
	{
		RemainingTime = FMath::Max(RemainingTime, TimePair.Key);
	}

	return RemainingTime;
}

void ULSPlayerSkillComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FinishPredictedFastMovementSkill();
	Super::EndPlay(EndPlayReason);
}

bool ULSPlayerSkillComponent::CanUseLocalPreview() const
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}

ULSSkillPreviewComponent* ULSPlayerSkillComponent::ResolvePreviewComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<ULSSkillPreviewComponent>() : nullptr;
}

void ULSPlayerSkillComponent::ServerRequestActivateSkill_Implementation(ELSPlayerSkillSlot Slot, FVector_NetQuantize TargetLocation, float AimYaw)
{
	ActivateSkillOnServer(Slot, FVector(TargetLocation), AimYaw);
}

bool ULSPlayerSkillComponent::ActivateSkillOnServer(ELSPlayerSkillSlot Slot, const FVector& TargetLocation, float AimYaw)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	ULSSkillDataAsset* SkillData = GetSkillData(Slot);
	if (!SkillData)
	{
		return false;
	}

	if (IsSkillCooldownActive(SkillData))
	{
		LogSkillCooldownBlocked(SkillData, TEXT("ServerActivate"));
		return false;
	}

	FLSSkillActivationContext Context;
	Context.SourceActor = OwnerActor;
	Context.SkillData = SkillData;
	Context.TargetLocation = TargetLocation;
	Context.AimYaw = AimYaw;

	if (SkillData->GetAbilityClass())
	{
		return TryActivateGameplayAbility(SkillData, Context);
	}

	UE_LOG(LogLS, Warning, TEXT("%s skill activation rejected because %s has no GAS AbilityClass."),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(SkillData));
	return false;
}

void ULSPlayerSkillComponent::LogSkillCooldownBlocked(const ULSSkillDataAsset* SkillData, const TCHAR* Phase) const
{
	const AActor* OwnerActor = GetOwner();
	const FGameplayTag CooldownTag = SkillData ? SkillData->GetCooldownTag() : FGameplayTag();
	UE_LOG(LogLS, Log, TEXT("%s skill blocked by cooldown. Phase=%s Skill=%s Tag=%s Remaining=%.2f"),
		*GetNameSafe(OwnerActor),
		Phase ? Phase : TEXT("Unknown"),
		*GetNameSafe(SkillData),
		CooldownTag.IsValid() ? *CooldownTag.ToString() : TEXT("None"),
		GetSkillCooldownRemaining(SkillData));
}

bool ULSPlayerSkillComponent::TryActivateGameplayAbility(ULSSkillDataAsset* SkillData, const FLSSkillActivationContext& Context)
{
	AActor* OwnerActor = GetOwner();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	const TSubclassOf<UGameplayAbility> AbilityClass = SkillData ? SkillData->GetAbilityClass() : nullptr;
	if (!OwnerActor || !OwnerActor->HasAuthority() || !ASC || !AbilityClass)
	{
		return false;
	}

	bool bHasAbility = false;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetClass() == AbilityClass)
		{
			bHasAbility = true;
			break;
		}
	}

	if (!bHasAbility)
	{
		ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
	}

	PendingAbilityContext = Context;
	PendingAbilityClass = AbilityClass;

	const bool bActivated = ASC->TryActivateAbilityByClass(AbilityClass);
	if (!bActivated)
	{
		PendingAbilityContext = FLSSkillActivationContext();
		PendingAbilityClass = nullptr;
	}

	return bActivated;
}

bool ULSPlayerSkillComponent::TrySendPassiveGameplayEvent(ULSSkillDataAsset* SkillData, int32 ComboIndex) const
{
	AActor* OwnerActor = GetOwner();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	const TSubclassOf<UGameplayAbility> AbilityClass = SkillData ? SkillData->GetAbilityClass() : nullptr;
	if (!OwnerActor || !OwnerActor->HasAuthority() || !ASC || !AbilityClass)
	{
		return false;
	}

	bool bHasAbility = false;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetClass() == AbilityClass)
		{
			bHasAbility = true;
			break;
		}
	}

	if (!bHasAbility)
	{
		ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
	}

	FGameplayEventData EventData;
	EventData.EventTag = LSGameplayTags::Event_Combat_BasicAttackHit;
	EventData.Instigator = OwnerActor;
	EventData.Target = OwnerActor;
	EventData.OptionalObject = SkillData;
	EventData.EventMagnitude = static_cast<float>(ComboIndex);
	ASC->HandleGameplayEvent(LSGameplayTags::Event_Combat_BasicAttackHit, &EventData);

	return true;
}

bool ULSPlayerSkillComponent::TryPredictFastMovementSkill(ULSSkillDataAsset* SkillData, const FVector& TargetLocation, float AimYaw)
{
	const TSubclassOf<UGameplayAbility> AbilityClass = SkillData ? SkillData->GetAbilityClass() : nullptr;
	if (!AbilityClass || bPredictedFastMovementInProgress || IsSkillCooldownActive(SkillData))
	{
		return false;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || OwnerCharacter->HasAuthority() || !OwnerCharacter->IsLocallyControlled())
	{
		return false;
	}

	float Distance = 0.0f;
	float Duration = 0.0f;
	if (!ResolvePredictedFastMovementParams(SkillData, Distance, Duration))
	{
		return false;
	}

	FVector AimDirection = TargetLocation - OwnerCharacter->GetActorLocation();
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = FRotator(0.0f, AimYaw, 0.0f).Vector();
	}

	AimDirection = AimDirection.GetSafeNormal2D();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	}

	UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	if (AimDirection.IsNearlyZero() || !MovementComponent)
	{
		return false;
	}

	IgnoreEnemiesForPredictedFastMovement(OwnerCharacter, OwnerCharacter->GetActorLocation(), AimDirection, Distance);

	TSharedPtr<FRootMotionSource_ConstantForce> RootMotion = MakeShared<FRootMotionSource_ConstantForce>();
	RootMotion->InstanceName = FName("PredictedFastMovementSkill");
	RootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	RootMotion->Priority = 5;
	RootMotion->Force = AimDirection * (Distance / Duration);
	RootMotion->Duration = Duration;
	RootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	PredictedFastMovementRootMotionSourceID = MovementComponent->ApplyRootMotionSource(RootMotion);
	bPredictedFastMovementInProgress = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PredictedFastMovementTimerHandle);
		World->GetTimerManager().SetTimer(PredictedFastMovementTimerHandle, this, &ULSPlayerSkillComponent::FinishPredictedFastMovementSkill, Duration, false);
	}

	return true;
}

bool ULSPlayerSkillComponent::ResolvePredictedFastMovementParams(ULSSkillDataAsset* SkillData, float& OutDistance, float& OutDuration) const
{
	const TSubclassOf<UGameplayAbility> AbilityClass = SkillData ? SkillData->GetAbilityClass() : nullptr;
	if (!AbilityClass)
	{
		return false;
	}

	if (AbilityClass->IsChildOf(ULSGA_Bypass::StaticClass()))
	{
		const ULSGA_Bypass* BypassCDO = AbilityClass->GetDefaultObject<ULSGA_Bypass>();
		return BypassCDO && BypassCDO->ResolveMovementParams(SkillData, OutDistance, OutDuration);
	}

	if (AbilityClass->IsChildOf(ULSGA_Execution::StaticClass()))
	{
		const ULSGA_Execution* ExecutionCDO = AbilityClass->GetDefaultObject<ULSGA_Execution>();
		return ExecutionCDO && ExecutionCDO->ResolveMovementParams(SkillData, OutDistance, OutDuration);
	}

	return false;
}

void ULSPlayerSkillComponent::IgnoreEnemiesForPredictedFastMovement(ACharacter* OwnerCharacter, const FVector& StartLocation, const FVector& Direction, float Distance)
{
	if (!OwnerCharacter || OwnerCharacter->HasAuthority() || Distance <= 0.0f || Direction.IsNearlyZero())
	{
		return;
	}

	UCapsuleComponent* OwnerCapsule = OwnerCharacter->GetCapsuleComponent();
	if (!OwnerCapsule)
	{
		return;
	}

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	const float PathHalfLength = Distance * 0.5f;
	const float PathHalfWidth = OwnerCapsule->GetScaledCapsuleRadius() * 2.0f;
	const FVector AreaCenter = StartLocation + (Direction * PathHalfLength);
	const float QueryRadius = FMath::Sqrt(FMath::Square(PathHalfLength) + FMath::Square(PathHalfWidth));

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		AreaCenter,
		QueryRadius,
		ObjectTypes,
		ALSEnemyCharacter::StaticClass(),
		ActorsToIgnore,
		OverlappedActors);

	const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal2D();
	for (AActor* EnemyActor : OverlappedActors)
	{
		if (!EnemyActor)
		{
			continue;
		}

		const FVector ToEnemy = EnemyActor->GetActorLocation() - StartLocation;
		const float ForwardDistance = FVector::DotProduct(ToEnemy, Direction);
		const float LateralDistance = FMath::Abs(FVector::DotProduct(ToEnemy, RightDirection));
		if (ForwardDistance < 0.0f || ForwardDistance > Distance || LateralDistance > PathHalfWidth)
		{
			continue;
		}

		OwnerCapsule->IgnoreActorWhenMoving(EnemyActor, true);
		OwnerCharacter->MoveIgnoreActorAdd(EnemyActor);

		if (ACharacter* EnemyCharacter = Cast<ACharacter>(EnemyActor))
		{
			if (UCapsuleComponent* EnemyCapsule = EnemyCharacter->GetCapsuleComponent())
			{
				EnemyCapsule->IgnoreActorWhenMoving(OwnerCharacter, true);
			}
			EnemyCharacter->MoveIgnoreActorAdd(OwnerCharacter);
		}

		PredictedFastMovementIgnoredEnemies.AddUnique(EnemyActor);
	}
}

void ULSPlayerSkillComponent::ClearIgnoredEnemiesForPredictedFastMovement(ACharacter* OwnerCharacter)
{
	if (!OwnerCharacter)
	{
		PredictedFastMovementIgnoredEnemies.Reset();
		return;
	}

	if (UCapsuleComponent* OwnerCapsule = OwnerCharacter->GetCapsuleComponent())
	{
		for (const TWeakObjectPtr<AActor>& IgnoredActor : PredictedFastMovementIgnoredEnemies)
		{
			if (AActor* EnemyActor = IgnoredActor.Get())
			{
				OwnerCapsule->IgnoreActorWhenMoving(EnemyActor, false);
			}
		}
	}

	for (const TWeakObjectPtr<AActor>& IgnoredActor : PredictedFastMovementIgnoredEnemies)
	{
		AActor* EnemyActor = IgnoredActor.Get();
		if (!EnemyActor)
		{
			continue;
		}

		OwnerCharacter->MoveIgnoreActorRemove(EnemyActor);
		if (ACharacter* EnemyCharacter = Cast<ACharacter>(EnemyActor))
		{
			if (UCapsuleComponent* EnemyCapsule = EnemyCharacter->GetCapsuleComponent())
			{
				EnemyCapsule->IgnoreActorWhenMoving(OwnerCharacter, false);
			}
			EnemyCharacter->MoveIgnoreActorRemove(OwnerCharacter);
		}
	}

	PredictedFastMovementIgnoredEnemies.Reset();
}

void ULSPlayerSkillComponent::FinishPredictedFastMovementSkill()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PredictedFastMovementTimerHandle);
	}

	if (bPredictedFastMovementInProgress)
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
		{
			ClearIgnoredEnemiesForPredictedFastMovement(OwnerCharacter);

			if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
			{
				MovementComponent->RemoveRootMotionSourceByID(PredictedFastMovementRootMotionSourceID);
			}
		}
	}

	PredictedFastMovementRootMotionSourceID = 0;
	PredictedFastMovementIgnoredEnemies.Reset();
	bPredictedFastMovementInProgress = false;
}
