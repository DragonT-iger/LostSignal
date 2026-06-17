#include "Animation/LSAN_SpawnNiagara.h"

#include "Components/SkeletalMeshComponent.h"
#include "LostSignal.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

void ULSAN_SpawnNiagara::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	if (!NiagaraSystem)
	{
		UE_LOG(LogLS, Warning, TEXT("%s Niagara notify skipped because NiagaraSystem is not set."), *GetNameSafe(MeshComp->GetOwner()));
		return;
	}

	const bool bHasValidSocket = SocketName.IsNone() || MeshComp->DoesSocketExist(SocketName);
	const FName ResolvedSocketName = bHasValidSocket ? SocketName : NAME_None;

	if (!bHasValidSocket)
	{
		UE_LOG(LogLS, Warning, TEXT("%s Niagara notify socket '%s' is missing. Effect will use mesh transform."),
			*GetNameSafe(MeshComp->GetOwner()), *SocketName.ToString());
	}

	if (bAttachToSocket)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraSystem, MeshComp, ResolvedSocketName, LocationOffset, RotationOffset, Scale,
			EAttachLocation::KeepRelativeOffset, bAutoDestroy, ENCPoolMethod::None, bAutoActivate);
		return;
	}

	const FTransform SourceTransform = ResolvedSocketName.IsNone()
		? MeshComp->GetComponentTransform()
		: MeshComp->GetSocketTransform(ResolvedSocketName);
	const FTransform SpawnTransform(FRotationMatrix(RotationOffset).ToQuat(), LocationOffset, Scale);
	const FTransform WorldTransform = SpawnTransform * SourceTransform;

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		MeshComp->GetWorld(), NiagaraSystem, WorldTransform.GetLocation(),
		WorldTransform.GetRotation().Rotator(), WorldTransform.GetScale3D(), bAutoDestroy, bAutoActivate);
}

FString ULSAN_SpawnNiagara::GetNotifyName_Implementation() const
{
	return TEXT("LS Spawn Niagara");
}
