#include "GAS/Abilities/LSGA_PlayerBasicAttack.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/LSCharacterBase.h"
#include "Combat/LSCombatStateComponent.h"
#include "Combat/LSPlayerCombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"
#include "TimerManager.h"

ULSGA_PlayerBasicAttack::ULSGA_PlayerBasicAttack()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(LSGameplayTags::Ability_PlayerBasicAttack);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_Attacking);
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dead);

	ComboSections = { TEXT("Attack_1"), TEXT("Attack_2"), TEXT("Attack_3") };

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

ULSGA_PlayerBasicAttack* ULSGA_PlayerBasicAttack::FindActiveBasicAttackAbility(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return nullptr;
	}

	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		ULSGA_PlayerBasicAttack* Ability = Cast<ULSGA_PlayerBasicAttack>(Spec.GetPrimaryInstance());
		if (Ability && Ability->IsActive())
		{
			return Ability;
		}
	}

	return nullptr;
}

void ULSGA_PlayerBasicAttack::QueueComboInput()
{
	if (!IsActive() || !bWaitingForPostComboInput)
	{
		return;
	}

	if (bComboInputBuffered)
	{
		return;
	}

	const int32 NextSectionIndex = CurrentSectionIndex + 1;
	if (!ComboSections.IsValidIndex(NextSectionIndex))
	{
		return;
	}

	bComboInputBuffered = true;
	bWaitingForPostComboInput = false;
	SetComboWindowTagActive(false);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PostComboInputWindowTimerHandle);
		World->GetTimerManager().SetTimerForNextTick(this, &ULSGA_PlayerBasicAttack::ConsumePostComboInput);
	}

	UE_LOG(LogLS, Log, TEXT("%s queued post-combo basic attack input."), *GetNameSafe(GetAvatarActorFromActorInfo()));
}

void ULSGA_PlayerBasicAttack::OpenComboWindow()
{
	if (!IsActive())
	{
		return;
	}

	bComboWindowOpen = true;
	SetComboWindowTagActive(true);
}

void ULSGA_PlayerBasicAttack::CloseComboWindow()
{
	if (!IsActive())
	{
		return;
	}

	bComboWindowOpen = false;
	SetComboWindowTagActive(false);
}

void ULSGA_PlayerBasicAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bEndingAbility = false;
	bComboInputBuffered = false;
	bComboWindowOpen = false;
	bWaitingForPostComboInput = false;
	bComboWindowTagActive = false;
	CurrentSectionIndex = INDEX_NONE;

	ALSCharacterBase* Character = Cast<ALSCharacterBase>(GetAvatarActorFromActorInfo());
	ULSPlayerCombatComponent* PlayerCombatComponent = Character ? Character->FindComponentByClass<ULSPlayerCombatComponent>() : nullptr;
	if (!Character || !PlayerCombatComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveAttackMontage = PlayerCombatComponent->GetBasicAttackMontage();
	if (!ActiveAttackMontage || ComboSections.Num() == 0)
	{
		UE_LOG(LogLS, Warning, TEXT("%s basic attack ability missing montage or combo sections."), *GetNameSafe(Character));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ULSCombatStateComponent* CombatStateComponent = Character->FindComponentByClass<ULSCombatStateComponent>())
	{
		CombatStateComponent->ClearBufferedCommand();
		CombatStateComponent->BeginAction(ELSCombatActionState::BasicAttack, ELSCombatActionPhase::Startup);
	}

	PlayComboSection(0);
}

void ULSGA_PlayerBasicAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bEndingAbility = true;
	SetComboWindowTagActive(false);
	bWaitingForPostComboInput = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PostComboInputWindowTimerHandle);
	}

	if (ALSCharacterBase* Character = Cast<ALSCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		if (bWasCancelled && ActiveAttackMontage)
		{
			Character->MulticastStopLSMontage(ActiveAttackMontage, CancelBlendOutTime);
		}

		if (ULSPlayerCombatComponent* PlayerCombatComponent = Character->FindComponentByClass<ULSPlayerCombatComponent>())
		{
			PlayerCombatComponent->HandleCombatActionEnd(ELSCombatActionState::BasicAttack);
		}
		else if (ULSCombatStateComponent* CombatStateComponent = Character->FindComponentByClass<ULSCombatStateComponent>())
		{
			if (CombatStateComponent->GetCurrentState() == ELSCombatActionState::BasicAttack)
			{
				CombatStateComponent->EndAction();
			}
		}
	}

	ActiveAttackMontage = nullptr;
	CurrentSectionIndex = INDEX_NONE;
	bComboInputBuffered = false;
	bComboWindowOpen = false;
	bWaitingForPostComboInput = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULSGA_PlayerBasicAttack::PlayComboSection(int32 SectionIndex)
{
	if (!ComboSections.IsValidIndex(SectionIndex) || !ActiveAttackMontage)
	{
		return;
	}

	ALSCharacterBase* Character = Cast<ALSCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return;
	}

	CurrentSectionIndex = SectionIndex;
	bComboInputBuffered = false;
	bComboWindowOpen = false;
	bWaitingForPostComboInput = false;
	SetComboWindowTagActive(false);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PostComboInputWindowTimerHandle);
	}

	if (ULSPlayerCombatComponent* PlayerCombatComponent = Character->FindComponentByClass<ULSPlayerCombatComponent>())
	{
		PlayerCombatComponent->ResetBasicAttackHit();
	}

	Character->MulticastPlayLSMontage(ActiveAttackMontage, ComboSections[SectionIndex]);
	Character->MulticastSetLSMontageNextSection(ActiveAttackMontage, ComboSections[SectionIndex], NAME_None);

	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !AnimInstance->Montage_IsPlaying(ActiveAttackMontage))
	{
		UE_LOG(LogLS, Warning, TEXT("%s basic attack failed to play montage %s."), *GetNameSafe(Character), *GetNameSafe(ActiveAttackMontage));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &ULSGA_PlayerBasicAttack::HandleAttackMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveAttackMontage);

	UE_LOG(
		LogLS,
		Log,
		TEXT("%s playing basic attack combo section %s (%d/%d)."),
		*GetNameSafe(Character),
		*ComboSections[SectionIndex].ToString(),
		SectionIndex + 1,
		ComboSections.Num());
}

void ULSGA_PlayerBasicAttack::OpenPostComboInputWindow()
{
	if (!IsActive())
	{
		return;
	}

	bWaitingForPostComboInput = true;
	SetComboWindowTagActive(true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PostComboInputWindowTimerHandle);
		World->GetTimerManager().SetTimer(
			PostComboInputWindowTimerHandle,
			this,
			&ULSGA_PlayerBasicAttack::ClosePostComboInputWindow,
			PostComboInputWindowSeconds,
			false);
	}

	UE_LOG(LogLS, Log, TEXT("%s opened post-combo input window for %.2f seconds."), *GetNameSafe(GetAvatarActorFromActorInfo()), PostComboInputWindowSeconds);
}

void ULSGA_PlayerBasicAttack::ClosePostComboInputWindow()
{
	if (!IsActive())
	{
		return;
	}

	bWaitingForPostComboInput = false;
	SetComboWindowTagActive(false);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void ULSGA_PlayerBasicAttack::ConsumePostComboInput()
{
	if (!IsActive() || !bComboInputBuffered)
	{
		return;
	}

	bComboInputBuffered = false;

	const int32 NextSectionIndex = CurrentSectionIndex + 1;
	if (!ComboSections.IsValidIndex(NextSectionIndex))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UE_LOG(LogLS, Log, TEXT("%s accepted post-combo basic attack input."), *GetNameSafe(GetAvatarActorFromActorInfo()));
	PlayComboSection(NextSectionIndex);
}

void ULSGA_PlayerBasicAttack::HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bEndingAbility)
	{
		return;
	}

	if (IsActive())
	{
		const int32 NextSectionIndex = CurrentSectionIndex + 1;
		if (!bInterrupted && ComboSections.IsValidIndex(NextSectionIndex) && PostComboInputWindowSeconds > 0.0f)
		{
			OpenPostComboInputWindow();
			return;
		}

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
	}
}

void ULSGA_PlayerBasicAttack::SetComboWindowTagActive(bool bActive)
{
	if (bComboWindowTagActive == bActive)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	bComboWindowTagActive = bActive;
	if (bActive)
	{
		ASC->AddLooseGameplayTag(LSGameplayTags::Combat_ComboWindow);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(LSGameplayTags::Combat_ComboWindow);
	}
}
