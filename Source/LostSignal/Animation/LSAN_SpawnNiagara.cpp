#include "Animation/LSAN_SpawnNiagara.h"

#include "Characters/LSCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "LostSignal.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace
{
FTransform BuildLSANSpawnNiagaraTransform(const FTransform& SourceTransform, const FVector& LocationOffset, const FRotator& RotationOffset, const FVector& Scale, ELSNiagaraSpawnTransformMode TransformMode)
{
	if (TransformMode == ELSNiagaraSpawnTransformMode::SourceLocationOnly)
	{
		return FTransform(RotationOffset, SourceTransform.GetLocation() + LocationOffset, Scale);
	}

	const FTransform OffsetTransform(FRotationMatrix(RotationOffset).ToQuat(), LocationOffset, Scale);
	return OffsetTransform * SourceTransform;
}

FTransform BuildLSANSpawnNiagaraSkillSourceTransform(const USkeletalMeshComponent* MeshComp, const FTransform& SourceTransform)
{
	const ALSCharacterBase* Character = MeshComp ? Cast<ALSCharacterBase>(MeshComp->GetOwner()) : nullptr;

	FRotator SkillActivationRotation;
	if (!Character || !Character->TryGetSkillActivationRotation(SkillActivationRotation))
	{
		UE_LOG(LogLS, Warning, TEXT("%s Niagara notify requested skill activation rotation, but no cached rotation exists. Falling back to source transform."),
			*GetNameSafe(MeshComp ? MeshComp->GetOwner() : nullptr));
		return SourceTransform;
	}

	return FTransform(SkillActivationRotation, SourceTransform.GetLocation(), SourceTransform.GetScale3D());
}
}

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
	const FTransform EffectiveSourceTransform = SpawnTransformMode == ELSNiagaraSpawnTransformMode::SkillActivationTransform
		? BuildLSANSpawnNiagaraSkillSourceTransform(MeshComp, SourceTransform)
		: SourceTransform;
	const FTransform WorldTransform = BuildLSANSpawnNiagaraTransform(
		EffectiveSourceTransform, LocationOffset, RotationOffset, Scale, SpawnTransformMode);

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		MeshComp->GetWorld(), NiagaraSystem, WorldTransform.GetLocation(),
		WorldTransform.GetRotation().Rotator(), WorldTransform.GetScale3D(), bAutoDestroy, bAutoActivate);
}

FString ULSAN_SpawnNiagara::GetNotifyName_Implementation() const
{
	return TEXT("LS Spawn Niagara");
}
