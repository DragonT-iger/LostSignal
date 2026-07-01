#include "Characters/LSFootstepComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/LSGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ULSFootstepComponent::ULSFootstepComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULSFootstepComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!FootstepSound || IsOwnerDead())
	{
		return;
	}

	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	// 공중(점프/낙하)이면 발소리 없음.
	const UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement();
	if (Movement && !Movement->IsMovingOnGround())
	{
		return;
	}

	// 이동 스킬(대시/바이패스/처형 등 RootMotionSource 이동) 중에는 보행 발소리를 내지 않는다.
	if (Movement && Movement->CurrentRootMotion.HasActiveRootMotionSources())
	{
		return;
	}

	const float Speed2D = OwnerCharacter->GetVelocity().Size2D();
	if (Speed2D < MinFootstepSpeed)
	{
		return;
	}

	// 이동 거리 누적 → StrideLength마다 좌우 번갈아 1회. 속도가 빠를수록 발소리가 빨라진다.
	FootstepDistanceAccum += Speed2D * DeltaTime;
	if (FootstepDistanceAccum >= StrideLength)
	{
		FootstepDistanceAccum -= StrideLength;
		PlayFootstep(bNextFootLeft ? LeftFootBone : RightFootBone);
		bNextFootLeft = !bNextFootLeft;
	}
}

void ULSFootstepComponent::PlayFootstep(FName FootBone) const
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	const USkeletalMeshComponent* Mesh = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
	if (!Mesh)
	{
		return;
	}

	const FVector Location = Mesh->DoesSocketExist(FootBone)
		? Mesh->GetSocketLocation(FootBone)
		: Mesh->GetComponentLocation();

	// 컴포넌트 틱은 각 클라에서 로컬로 도므로 발소리도 로컬 재생 → MO 복제 불필요(서버는 오디오 없음 → 무음).
	UGameplayStatics::PlaySoundAtLocation(this, FootstepSound, Location);
}

bool ULSFootstepComponent::IsOwnerDead() const
{
	if (const IAbilitySystemInterface* AbilitySystemOwner = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (const UAbilitySystemComponent* ASC = AbilitySystemOwner->GetAbilitySystemComponent())
		{
			return ASC->HasMatchingGameplayTag(LSGameplayTags::State_Dead);
		}
	}
	return false;
}
