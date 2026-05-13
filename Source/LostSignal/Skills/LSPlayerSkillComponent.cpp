#include "Skills/LSPlayerSkillComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GAS/Abilities/LSGA_Bypass.h"
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

		TryPredictBypassMovement(GetSkillData(SlotToActivate), TargetLocation, AimRotation.Yaw);
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

		FLSBasicAttackHitContext Context;
		Context.SourceActor = OwnerActor;
		Context.SkillData = PassiveSkillData;
		Context.ComboIndex = ComboIndex;
		Context.ValidHitCount = ValidHitCount;
		PassiveSkillData->HandleBasicAttackHit(Context);
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

void ULSPlayerSkillComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FinishPredictedBypass();
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

	FLSSkillActivationContext Context;
	Context.SourceActor = OwnerActor;
	Context.SkillData = SkillData;
	Context.TargetLocation = TargetLocation;
	Context.AimYaw = AimYaw;

	if (SkillData->GetAbilityClass())
	{
		return TryActivateGameplayAbility(SkillData, Context);
	}

	return SkillData->ActivateSkill(Context);
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

bool ULSPlayerSkillComponent::TryPredictBypassMovement(ULSSkillDataAsset* SkillData, const FVector& TargetLocation, float AimYaw)
{
	const TSubclassOf<UGameplayAbility> AbilityClass = SkillData ? SkillData->GetAbilityClass() : nullptr;
	if (!AbilityClass || !AbilityClass->IsChildOf(ULSGA_Bypass::StaticClass()) || bPredictedBypassInProgress)
	{
		return false;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || OwnerCharacter->HasAuthority() || !OwnerCharacter->IsLocallyControlled())
	{
		return false;
	}

	const ULSGA_Bypass* BypassCDO = AbilityClass->GetDefaultObject<ULSGA_Bypass>();
	float Distance = 0.0f;
	float Duration = 0.0f;
	if (!BypassCDO || !BypassCDO->ResolveMovementParams(SkillData, Distance, Duration))
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

	TSharedPtr<FRootMotionSource_ConstantForce> RootMotion = MakeShared<FRootMotionSource_ConstantForce>();
	RootMotion->InstanceName = FName("PredictedBypass");
	RootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	RootMotion->Priority = 5;
	RootMotion->Force = AimDirection * (Distance / Duration);
	RootMotion->Duration = Duration;
	RootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	PredictedBypassRootMotionSourceID = MovementComponent->ApplyRootMotionSource(RootMotion);
	bPredictedBypassInProgress = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PredictedBypassTimerHandle);
		World->GetTimerManager().SetTimer(PredictedBypassTimerHandle, this, &ULSPlayerSkillComponent::FinishPredictedBypass, Duration, false);
	}

	return true;
}

void ULSPlayerSkillComponent::FinishPredictedBypass()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PredictedBypassTimerHandle);
	}

	if (bPredictedBypassInProgress)
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
		{
			if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
			{
				MovementComponent->RemoveRootMotionSourceByID(PredictedBypassRootMotionSourceID);
			}
		}
	}

	PredictedBypassRootMotionSourceID = 0;
	bPredictedBypassInProgress = false;
}
