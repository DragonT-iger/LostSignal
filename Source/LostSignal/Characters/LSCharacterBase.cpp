#include "Characters/LSCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LostSignal.h"

ALSCharacterBase::ALSCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	// 컨트롤러 회전을 따르지 않음 — 마우스 방향 또는 AI가 직접 회전 제어
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 이동 방향으로 자동 회전하지 않음 — 플레이어는 마우스, 적은 AI가 회전 담당
	GetCharacterMovement()->bOrientRotationToMovement = false;

	// 메시 원점(발바닥)과 캡슐 원점(중심)의 차이 보정
	GetMesh()->SetRelativeLocationAndRotation(
		FVector(0.0f, 0.0f, -96.0f),
		FRotator(0.0f, -90.0f, 0.0f)
	);

	// AbilitySystemComponent 생성
	// Unity: new AbilityManager() 에 해당 — GAS의 어빌리티·이펙트·태그를 모두 관리
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// 복제 모드: 싱글은 Full, 멀티 전환 시 Mixed(플레이어) / Minimal(적)로 변경
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);
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
		// InitAbilityActorInfo: ASC에 "나는 이 액터의 ASC다" 라고 등록.
		// 인자1(OwnerActor): 소유자(여기선 캐릭터 자신)
		// 인자2(AvatarActor): 실제 월드에서 움직이는 액터(동일)
		// PlayerState 방식으로 전환 시 인자1만 PlayerState로 변경
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void ALSCharacterBase::GrantAbility(TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!HasAuthority() || !AbilitySystemComponent || !AbilityClass)
	{
		return;
	}

	FGameplayAbilitySpec Spec(AbilityClass, /*Level=*/1);
	AbilitySystemComponent->GiveAbility(Spec);

	UE_LOG(LogLS, Log, TEXT("GrantAbility: %s → %s"), *GetNameSafe(this), *AbilityClass->GetName());
}
