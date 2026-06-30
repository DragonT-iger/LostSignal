#include "GAS/Cues/LSGCN_PlaySound.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

bool ULSGCN_PlaySound::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	USoundBase* Sound = const_cast<USoundBase*>(Cast<USoundBase>(Parameters.SourceObject.Get()));
	if (!Sound || !MyTarget)
	{
		return false;
	}

	// 발동 측이 채워준 위치. 비어 있으면 큐 대상 위치로 폴백.
	FVector Location = Parameters.Location;
	if (Location.IsNearlyZero())
	{
		Location = MyTarget->GetActorLocation();
	}

	UGameplayStatics::PlaySoundAtLocation(MyTarget, Sound, Location);
	return true;
}
