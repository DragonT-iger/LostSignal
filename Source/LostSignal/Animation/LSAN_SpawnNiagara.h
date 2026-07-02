#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "LSAN_SpawnNiagara.generated.h"

class UNiagaraSystem;

UENUM(BlueprintType)
enum class ELSNiagaraSpawnTransformMode : uint8
{
	SourceTransform,
	SourceLocationOnly,
	SkillActivationTransform
};

UCLASS()
class LOSTSIGNAL_API ULSAN_SpawnNiagara : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

private:
	UPROPERTY(EditAnywhere, Category="LS/VFX")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditAnywhere, Category="LS/VFX")
	FName SocketName = NAME_None;

	UPROPERTY(EditAnywhere, Category="LS/VFX")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category="LS/VFX")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category="LS/VFX", meta=(EditCondition="!bAttachToSocket"))
	ELSNiagaraSpawnTransformMode SpawnTransformMode = ELSNiagaraSpawnTransformMode::SourceTransform;

	UPROPERTY(EditAnywhere, Category="LS/VFX")
	FVector Scale = FVector(1.0f);

	UPROPERTY(EditAnywhere, Category="LS/VFX")
	bool bAttachToSocket = true;

	UPROPERTY(EditAnywhere, Category="LS/VFX", meta=(EditCondition="bAttachToSocket"))
	bool bDetachAfterSpawn = false;

	UPROPERTY(EditAnywhere, Category="LS/VFX")
	bool bAutoDestroy = true;

	UPROPERTY(EditAnywhere, Category="LS/VFX")
	bool bAutoActivate = true;
};
