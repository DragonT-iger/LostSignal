#include "GAS/Cues/LSGCN_SkillCast.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

bool ULSGCN_SkillCast::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	USoundBase* CastSound = const_cast<USoundBase*>(Cast<USoundBase>(Parameters.SourceObject.Get()));
	if (!CastSound || !MyTarget)
	{
		return false;
	}

	// 발동 측이 채워준 캐스터 위치. 비어 있으면 큐 대상(캐스터) 위치로 폴백.
	FVector Location = Parameters.Location;
	if (Location.IsNearlyZero())
	{
		Location = MyTarget->GetActorLocation();
	}

	UGameplayStatics::PlaySoundAtLocation(MyTarget, CastSound, Location);
	return true;
}
