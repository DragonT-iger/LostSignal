#include "Characters/LSCharacterBase.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Combat/LSCombatStateComponent.h"
#include "Combat/LSStatusEffectComponent.h"
#include "Components/CapsuleComponent.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LostSignal.h"

ALSCharacterBase::ALSCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	GetMesh()->SetRelativeLocationAndRotation(
		FVector(0.0f, 0.0f, -96.0f),
		FRotator(0.0f, -90.0f, 0.0f));

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);

	CombatAttributeSet = CreateDefaultSubobject<ULSCombatAttributeSet>(TEXT("CombatAttributeSet"));
	CharacterCombatComponent = CreateDefaultSubobject<ULSCharacterCombatComponent>(TEXT("CharacterCombatComponent"));
	CombatStateComponent = CreateDefaultSubobject<ULSCombatStateComponent>(TEXT("CombatStateComponent"));
	StatusEffectComponent = CreateDefaultSubobject<ULSStatusEffectComponent>(TEXT("StatusEffectComponent"));

	GetMesh()->SetRenderCustomDepth(true);
	GetMesh()->SetCustomDepthStencilValue(1);
}

UAbilitySystemComponent* ALSCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ALSCharacterBase::MulticastPlayLSMontage_Implementation(UAnimMontage* Montage, FName StartSection, float PlayRate)
{
	if (!Montage)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	AnimInstance->Montage_Play(Montage, FMath::Max(0.01f, PlayRate));
	if (!StartSection.IsNone())
	{
		AnimInstance->Montage_JumpToSection(StartSection, Montage);
		AnimInstance->Montage_SetNextSection(StartSection, NAME_None, Montage);
	}
}

void ALSCharacterBase::MulticastJumpLSMontageSection_Implementation(UAnimMontage* Montage, FName SectionName)
{
	if (!Montage || SectionName.IsNone())
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !AnimInstance->Montage_IsPlaying(Montage))
	{
		return;
	}

	AnimInstance->Montage_JumpToSection(SectionName, Montage);
}

void ALSCharacterBase::MulticastSetLSMontageNextSection_Implementation(UAnimMontage* Montage, FName SectionNameToChange, FName NextSection)
{
	if (!Montage || SectionNameToChange.IsNone())
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !AnimInstance->Montage_IsPlaying(Montage))
	{
		return;
	}

	AnimInstance->Montage_SetNextSection(SectionNameToChange, NextSection, Montage);
}

void ALSCharacterBase::MulticastStopLSMontage_Implementation(UAnimMontage* Montage, float BlendOutTime)
{
	if (!Montage)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !AnimInstance->Montage_IsPlaying(Montage))
	{
		return;
	}

	AnimInstance->Montage_Stop(BlendOutTime, Montage);
}

void ALSCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void ALSCharacterBase::GrantAbility(TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!HasAuthority() || !AbilitySystemComponent || !AbilityClass)
	{
		return;
	}

	FGameplayAbilitySpec Spec(AbilityClass, 1);
	AbilitySystemComponent->GiveAbility(Spec);

	UE_LOG(LogLS, Log, TEXT("GrantAbility: %s -> %s"), *GetNameSafe(this), *AbilityClass->GetName());
}
