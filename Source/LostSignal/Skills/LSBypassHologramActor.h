#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LSBypassHologramActor.generated.h"

class ACharacter;
class UMaterialInterface;
class USceneComponent;
class USkeletalMeshComponent;

/** Visual decoy spawned by Bypass-Spoofing at the caster's start location. */
UCLASS()
class LOSTSIGNAL_API ALSBypassHologramActor : public AActor
{
	GENERATED_BODY()

public:
	ALSBypassHologramActor();

	void InitializeFromCharacter(const ACharacter* SourceCharacter, UMaterialInterface* OverrideMaterial, float LifeSeconds);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Skill|Bypass")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Skill|Bypass")
	TObjectPtr<USkeletalMeshComponent> HologramMesh;
};
