#include "Characters/Enemys/LSTrainingDummyCharacter.h"

#include "AbilitySystemComponent.h"
#include "GAS/Effects/LSGE_TrainingDummyRecovery.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"

ALSTrainingDummyCharacter::ALSTrainingDummyCharacter()
{
	AIControllerClass = nullptr;
	AutoPossessAI = EAutoPossessAI::Disabled;
	DefaultStateTree = nullptr;
	MonsterActionAbilityClass = nullptr;
	DeathLootBoxClass = nullptr;
	DeathLootRowName = NAME_None;
}

void ALSTrainingDummyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	InitializeTrainingDummyRecovery();
}

void ALSTrainingDummyCharacter::InitializeTrainingDummyRecovery()
{
	if (!AbilitySystemComponent || !MonsterAttributeSet)
	{
		UE_LOG(LogLS, Warning, TEXT("%s: 허수아비 회복 초기화에 필요한 ASC 또는 AttributeSet이 없습니다."), *GetNameSafe(this));
		return;
	}

	const float ClampedRecovery = FMath::Max(0.0f, RecoveryPerSecond);
	MonsterAttributeSet->InitRecovery(ClampedRecovery);
	AbilitySystemComponent->AddLooseGameplayTag(LSGameplayTags::State_CannotDie);

	if (ClampedRecovery <= 0.0f)
	{
		return;
	}

	FGameplayEffectSpecHandle RecoverySpec = AbilitySystemComponent->MakeOutgoingSpec(
		ULSGE_TrainingDummyRecovery::StaticClass(),
		1.0f,
		AbilitySystemComponent->MakeEffectContext());
	if (!RecoverySpec.IsValid())
	{
		UE_LOG(LogLS, Warning, TEXT("%s: 허수아비 지속 회복 GameplayEffect Spec 생성에 실패했습니다."), *GetNameSafe(this));
		return;
	}

	RecoverySpec.Data->SetSetByCallerMagnitude(LSGameplayTags::Data_Health_Amount, MonsterAttributeSet->GetRecovery());
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*RecoverySpec.Data.Get());
}
