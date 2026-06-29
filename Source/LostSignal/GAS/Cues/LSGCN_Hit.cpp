#include "GAS/Cues/LSGCN_Hit.h"

#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "Sound/SoundBase.h"

bool ULSGCN_Hit::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!MyTarget)
	{
		return false;
	}

	if (!HitSound)
	{
		UE_LOG(LogLS, Warning, TEXT("%s: 피격 GameplayCue가 HitSound 미설정으로 건너뜀."), *GetNameSafe(MyTarget));
		return false;
	}

	// 피격자 위치에서 재생. 데디케이티드 서버는 오디오 디바이스가 없어 자연히 무시된다.
	UGameplayStatics::PlaySoundAtLocation(
		MyTarget,
		HitSound,
		MyTarget->GetActorLocation(),
		FRotator::ZeroRotator,
		VolumeMultiplier,
		PitchMultiplier,
		0.0f,
		AttenuationSettings,
		ConcurrencySettings,
		MyTarget);

	return true;
}
