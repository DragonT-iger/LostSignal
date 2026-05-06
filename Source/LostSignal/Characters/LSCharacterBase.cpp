#include "Characters/LSCharacterBase.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Combat/LSCharacterCombatComponent.h"
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
}

UAbilitySystemComponent* ALSCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
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
