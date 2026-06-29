#include "Animation/LSAN_PlaySound.h"

#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "Sound/SoundBase.h"

void ULSAN_PlaySound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		UE_LOG(LogLS, Warning, TEXT("Sound notify skipped because owner actor is missing."));
		return;
	}

	if (!Sound)
	{
		UE_LOG(LogLS, Warning, TEXT("%s sound notify skipped because Sound is not set."), *GetNameSafe(OwnerActor));
		return;
	}

	const FTransform ActorTransform = OwnerActor->GetActorTransform();
	const FVector SoundLocation = ActorTransform.TransformPosition(LocationOffset);
	const FRotator SoundRotation = (ActorTransform.GetRotation().Rotator() + RotationOffset);

	UGameplayStatics::PlaySoundAtLocation(
		OwnerActor,
		Sound,
		SoundLocation,
		SoundRotation,
		VolumeMultiplier,
		PitchMultiplier,
		StartTime,
		AttenuationSettings,
		ConcurrencySettings,
		OwnerActor);
}

FString ULSAN_PlaySound::GetNotifyName_Implementation() const
{
	return TEXT("LS Play Sound");
}
