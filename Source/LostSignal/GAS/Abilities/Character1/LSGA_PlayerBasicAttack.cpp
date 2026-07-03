#include "GAS/Abilities/Character1/LSGA_PlayerBasicAttack.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/LSCharacterBase.h"
#include "Combat/LSCombatStateComponent.h"
#include "Combat/LSPlayerCombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/LSComboAttackRow.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"
#include "TimerManager.h"

ULSGA_PlayerBasicAttack::ULSGA_PlayerBasicAttack()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(LSGameplayTags::Ability_PlayerBasicAttack);
	AssetTags.AddTag(LSGameplayTags::Combat_Attacking); // 스턴·사망 등 외부 CancelAbilities 매칭용 분류 태그(AssetTags 기준 매칭)
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_Attacking);
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Stunned);

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

void ULSGA_PlayerBasicAttack::QueueComboInput(bool bFromHold)
{
	if (!IsActive() || !bWaitingForPostComboInput)
	{
		return;
	}

	if (bComboInputBuffered)
	{
		return;
	}

	if (ResolveNextComboSectionIndex() == INDEX_NONE)
	{
		return;
	}

	bComboInputBuffered = true;
	bComboInputFromHold = bFromHold;
	bWaitingForPostComboInput = false;
	SetComboWindowTagActive(false);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PostComboInputWindowTimerHandle);
		World->GetTimerManager().SetTimerForNextTick(this, &ULSGA_PlayerBasicAttack::ConsumePostComboInput);
	}

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
	bComboInputFromHold = false;
	bComboWindowOpen = false;
	bWaitingForPostComboInput = false;
	bComboWindowTagActive = false;
	CurrentSectionIndex = INDEX_NONE;
	CurrentComboTag = 0;
	CurrentComboInputWindowSeconds = 0.0f;

	ALSCharacterBase* Character = Cast<ALSCharacterBase>(GetAvatarActorFromActorInfo());
	ULSPlayerCombatComponent* PlayerCombatComponent = Character ? Character->FindComponentByClass<ULSPlayerCombatComponent>() : nullptr;
	if (!Character || !PlayerCombatComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedPlayerCombatComponent = PlayerCombatComponent;

	ActiveAttackMontage = PlayerCombatComponent->GetBasicAttackMontage();
	if (!ActiveAttackMontage || ComboSections.Num() == 0)
	{
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

	int32 StartSectionIndex = 0;
	int32 StartComboTag = 0;
	if (PlayerCombatComponent->ConsumePendingBasicAttackComboIndexOverride(StartSectionIndex, StartComboTag))
	{
		UE_LOG(LogLS, Log, TEXT("%s basic attack starts from override combo index %d."),
			*GetNameSafe(Character),
			StartSectionIndex);
		CurrentComboTag = FMath::Max(0, StartComboTag);
	}
	if (!ComboSections.IsValidIndex(StartSectionIndex))
	{
		StartSectionIndex = 0;
	}

	PlayComboSection(StartSectionIndex);
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
	CachedPlayerCombatComponent = nullptr;
	CurrentSectionIndex = INDEX_NONE;
	CurrentComboTag = 0;
	CurrentComboInputWindowSeconds = 0.0f;
	bComboInputBuffered = false;
	bComboInputFromHold = false;
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

	ULSPlayerCombatComponent* PlayerCombatComponent = Character->FindComponentByClass<ULSPlayerCombatComponent>();
	const FLSComboAttackRow* ComboRow = PlayerCombatComponent
		? PlayerCombatComponent->ResolveComboAttackRow(SectionIndex, CurrentComboTag)
		: nullptr;
	CurrentComboInputWindowSeconds = ComboRow && ComboRow->Combo_Input_Window > 0.0f
		? ComboRow->Combo_Input_Window
		: PostComboInputWindowSeconds;

	if (PlayerCombatComponent)
	{
		PlayerCombatComponent->ResetBasicAttackHit();
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const float AttackSpeed = ASC
		? ASC->GetNumericAttribute(ULSCharacterAttributeSet::GetAttackSpeedAttribute())
		: 1.0f;
	Character->MulticastPlayLSMontage(ActiveAttackMontage, ComboSections[SectionIndex], ResolveComboPlayRate(ComboRow, SectionIndex, AttackSpeed));
	Character->MulticastSetLSMontageNextSection(ActiveAttackMontage, ComboSections[SectionIndex], NAME_None);

	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !AnimInstance->Montage_IsPlaying(ActiveAttackMontage))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &ULSGA_PlayerBasicAttack::HandleAttackMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveAttackMontage);

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
		const float WindowSeconds = GetCurrentComboInputWindowSeconds();
		World->GetTimerManager().ClearTimer(PostComboInputWindowTimerHandle);
		World->GetTimerManager().SetTimer(
			PostComboInputWindowTimerHandle,
			this,
			&ULSGA_PlayerBasicAttack::ClosePostComboInputWindow,
			WindowSeconds,
			false);
	}

	// 공격 버튼 홀드 중이면 윈도우가 열리자마자 자동 버퍼링 — 연타 최속과 동일한 타이밍으로 콤보가 이어진다.
	if (IsBasicAttackHeld())
	{
		QueueComboInput(true);
	}
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

	// 홀드 유래 버퍼는 소비 직전에 홀드를 재확인한다. 해제됐으면 취소하고 윈도우를 복원해 이후 탭 입력을 기존 방식으로 받는다.
	if (bComboInputFromHold && !IsBasicAttackHeld())
	{
		bComboInputBuffered = false;
		bComboInputFromHold = false;
		OpenPostComboInputWindow();
		return;
	}

	bComboInputBuffered = false;
	bComboInputFromHold = false;
	CurrentComboTag = 0;

	const int32 NextSectionIndex = ResolveNextComboSectionIndex();
	if (NextSectionIndex == INDEX_NONE)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

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
		if (!bInterrupted && ResolveNextComboSectionIndex() != INDEX_NONE && GetCurrentComboInputWindowSeconds() > 0.0f)
		{
			OpenPostComboInputWindow();
			return;
		}

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
	}
}

float ULSGA_PlayerBasicAttack::ResolveComboPlayRate(const FLSComboAttackRow* ComboRow, int32 SectionIndex, float AttackSpeed) const
{
	const float BasePlayRate = FMath::Max(0.01f, AttackSpeed);
	if (!ComboRow || ComboRow->Combo_Time <= 0.0f || !ActiveAttackMontage || !ComboSections.IsValidIndex(SectionIndex))
	{
		return BasePlayRate;
	}

	const int32 MontageSectionIndex = ActiveAttackMontage->GetSectionIndex(ComboSections[SectionIndex]);
	if (MontageSectionIndex == INDEX_NONE)
	{
		return BasePlayRate;
	}

	const float SectionLength = ActiveAttackMontage->GetSectionLength(MontageSectionIndex);
	if (SectionLength <= 0.0f)
	{
		return BasePlayRate;
	}

	const float PlayRate = FMath::Max(0.01f, SectionLength / ComboRow->Combo_Time * AttackSpeed);
	const float FinalSectionTime = SectionLength / PlayRate;

	return PlayRate;
}

float ULSGA_PlayerBasicAttack::GetCurrentComboInputWindowSeconds() const
{
	return CurrentComboInputWindowSeconds > 0.0f ? CurrentComboInputWindowSeconds : PostComboInputWindowSeconds;
}

bool ULSGA_PlayerBasicAttack::IsBasicAttackHeld() const
{
	return CachedPlayerCombatComponent.IsValid() && CachedPlayerCombatComponent->IsBasicAttackHeld();
}

int32 ULSGA_PlayerBasicAttack::ResolveNextComboSectionIndex() const
{
	const int32 NextSectionIndex = CurrentSectionIndex + 1;
	if (ComboSections.IsValidIndex(NextSectionIndex))
	{
		return NextSectionIndex;
	}

	// 마지막 섹션 이후에도 홀드 중이면 1타부터 무한 반복. 탭 방식은 기존대로 종료.
	if (IsBasicAttackHeld() && ComboSections.Num() > 0)
	{
		return 0;
	}

	return INDEX_NONE;
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
