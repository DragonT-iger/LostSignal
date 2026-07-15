#include "GAS/Cues/LSGCN_SpawnNiagara.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

bool ULSGCN_SpawnNiagara::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	UNiagaraSystem* NiagaraSystem = const_cast<UNiagaraSystem*>(Cast<UNiagaraSystem>(Parameters.SourceObject.Get()));
	if (!NiagaraSystem || !MyTarget)
	{
		return false;
	}

	FVector Location = Parameters.Location;
	if (Location.IsNearlyZero())
	{
		Location = MyTarget->GetActorLocation();
	}

	const FRotator Rotation = Parameters.Normal.IsNearlyZero()
		? FRotator::ZeroRotator
		: Parameters.Normal.Rotation();
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(MyTarget, NiagaraSystem, Location, Rotation);
	return true;
}
