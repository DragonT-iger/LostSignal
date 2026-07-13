#include "Animation/LSMonsterLocomotionAnimInstance.h"

#include "AI/LSMonsterSenseComponent.h"
#include "Characters/LSEnemyCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void ULSMonsterLocomotionAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	UpdateMovementSpeedCache();
	UpdateLocomotionGait();
}

void ULSMonsterLocomotionAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	UpdateMovementSpeedCache();
	UpdateLocomotionGait();
}

void ULSMonsterLocomotionAnimInstance::UpdateMovementSpeedCache()
{
	const UCharacterMovementComponent* MovementComponent = GetOwnerMovementComponent();
	CurrentMaxWalkSpeed = MovementComponent ? MovementComponent->MaxWalkSpeed : 0.0f;

	// 기준(base) 속도는 ALSEnemyCharacter가 BeginPlay에 저장한 멀티 미적용 기본 속도를 단일 출처로 읽는다.
	// (이동 Task의 일시적 MaxWalkSpeed 변경에 영향받지 않음)
	const ALSEnemyCharacter* EnemyCharacter = Cast<ALSEnemyCharacter>(TryGetPawnOwner());
	BaseMaxWalkSpeed = EnemyCharacter ? EnemyCharacter->GetDefaultMaxWalkSpeed() : 0.0f;
}

void ULSMonsterLocomotionAnimInstance::UpdateLocomotionGait()
{
	if (Speed <= MovingSpeedThreshold)
	{
		LocomotionGait = ELSMonsterLocomotionGait::Idle;
		return;
	}

	if (bAlwaysWalkWhenMoving)
	{
		LocomotionGait = ELSMonsterLocomotionGait::Walk;
		return;
	}

	const float WalkRunMaxSpeedThreshold = ResolveWalkRunMaxSpeedThreshold();
	if (WalkRunMaxSpeedThreshold > 0.0f && CurrentMaxWalkSpeed > 0.0f)
	{
		LocomotionGait = CurrentMaxWalkSpeed <= WalkRunMaxSpeedThreshold
			? ELSMonsterLocomotionGait::Walk
			: ELSMonsterLocomotionGait::Run;
		return;
	}

	LocomotionGait = ELSMonsterLocomotionGait::Run;
}

float ULSMonsterLocomotionAnimInstance::ResolveWalkRunMaxSpeedThreshold() const
{
	if (BaseMaxWalkSpeed <= 0.0f)
	{
		return 0.0f;
	}

	// Patrol만 walk다. Chase는 속도 변경이 없어 base(=run) 속도로 달리므로,
	// 임계선을 Patrol 속도와 base 속도의 중간에 둔다(Patrol < 임계 < base).
	const ALSEnemyCharacter* EnemyCharacter = Cast<ALSEnemyCharacter>(TryGetPawnOwner());
	const ULSMonsterSenseComponent* SenseComponent = EnemyCharacter ? EnemyCharacter->GetMonsterSenseComponent() : nullptr;
	if (SenseComponent && SenseComponent->HasArchetypeMoveSpeedMultipliers())
	{
		const float PatrolMultiplier = SenseComponent->GetPatrolMoveSpeedMultiplier();
		if (PatrolMultiplier < 1.0f)
		{
			return BaseMaxWalkSpeed * ((PatrolMultiplier + 1.0f) * 0.5f);
		}
	}

	return BaseMaxWalkSpeed * WalkRunThresholdAlpha;
}

const UCharacterMovementComponent* ULSMonsterLocomotionAnimInstance::GetOwnerMovementComponent() const
{
	const ACharacter* CharacterOwner = Cast<ACharacter>(TryGetPawnOwner());
	return CharacterOwner ? CharacterOwner->GetCharacterMovement() : nullptr;
}
