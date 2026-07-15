#include "Animation/LSAN_Footstep.h"

#include "Characters/LSCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

void ULSAN_Footstep::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	// Persona 프리뷰 등 캐릭터가 아닌 소유자에서는 조용히 무시 (경고 스팸 방지)
	const ALSCharacterBase* Character = Cast<ALSCharacterBase>(MeshComp->GetOwner());
	if (!Character)
	{
		return;
	}

	// 사운드·VFX는 캐릭터 BP에서 매핑한다. 미할당이면 해당 항목만 조용히 생략.
	USoundBase* FootstepSound = Character->GetFootstepSound();
	UNiagaraSystem* FootstepVFX = Character->GetFootstepVFX();
	if (!FootstepSound && !FootstepVFX)
	{
		return;
	}

	// 공중(점프/낙하)이거나 이동 스킬(대시/바이패스/처형 등 RootMotionSource 이동) 중에는 보행 발소리를 내지 않는다.
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (Movement && (!Movement->IsMovingOnGround() || Movement->CurrentRootMotion.HasActiveRootMotionSources()))
	{
		return;
	}

	const FVector Location = (!SocketName.IsNone() && MeshComp->DoesSocketExist(SocketName))
		? MeshComp->GetSocketLocation(SocketName)
		: MeshComp->GetComponentLocation();

	// 노티파이는 각 클라의 로컬 애님 재생에서 발화하므로 로컬 재생/스폰 → MO 복제 불필요(데디 서버는 코스메틱 없음).
	if (FootstepSound)
	{
		UGameplayStatics::PlaySoundAtLocation(MeshComp, FootstepSound, Location);
	}

	// 발에 붙이지 않고 접지 지점에 남기는 비부착 스폰. 진행 방향 연출을 위해 캐릭터 회전만 적용.
	if (FootstepVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			MeshComp->GetWorld(), FootstepVFX, Location, Character->GetActorRotation());
	}
}

FString ULSAN_Footstep::GetNotifyName_Implementation() const
{
	return TEXT("LS Footstep");
}
