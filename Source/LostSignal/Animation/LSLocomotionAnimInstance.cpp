#include "Animation/LSLocomotionAnimInstance.h"

#include "GameFramework/Pawn.h"
#include "KismetAnimationLibrary.h"

void ULSLocomotionAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	UpdateLocomotionData();
}

// Try Get Pawn Owner → 유효하면 속도/방향/속력/Yaw를 갱신한다(그래프의 Is Valid 분기 대응).
void ULSLocomotionAnimInstance::UpdateLocomotionData()
{
	const APawn* PawnOwner = TryGetPawnOwner();
	if (!PawnOwner)
	{
		return;
	}

	CurrentVelocity = PawnOwner->GetVelocity();

	Speed = CurrentVelocity.Size();
	Direction = UKismetAnimationLibrary::CalculateDirection(CurrentVelocity, PawnOwner->GetActorRotation());

	// 이동속도 배수(베이스에서 갱신)로 정규화한 기준 속력. BlendSpace X축에 이 값을 넣고 Play Rate엔 배수를 넣는다.
	GaitSpeed = MoveSpeedMultiplier > 0.0f ? Speed / MoveSpeedMultiplier : Speed;
}
