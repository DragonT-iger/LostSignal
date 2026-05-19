#include "Skills/LSBypassHologramActor.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInterface.h"

ALSBypassHologramActor::ALSBypassHologramActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	HologramMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HologramMesh"));
	HologramMesh->SetupAttachment(SceneRoot);
	HologramMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HologramMesh->SetGenerateOverlapEvents(false);
	HologramMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
}

void ALSBypassHologramActor::InitializeFromCharacter(const ACharacter* SourceCharacter, UMaterialInterface* OverrideMaterial, float LifeSeconds)
{
	const USkeletalMeshComponent* SourceMesh = SourceCharacter ? SourceCharacter->GetMesh() : nullptr;
	if (!SourceCharacter || !SourceMesh || !HologramMesh)
	{
		return;
	}

	SetActorTransform(SourceCharacter->GetActorTransform());
	HologramMesh->SetRelativeTransform(SourceMesh->GetRelativeTransform());
	HologramMesh->SetSkeletalMeshAsset(SourceMesh->GetSkeletalMeshAsset());
	HologramMesh->SetAnimInstanceClass(SourceMesh->GetAnimInstance() ? SourceMesh->GetAnimInstance()->GetClass() : nullptr);

	if (OverrideMaterial)
	{
		const int32 MaterialCount = HologramMesh->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			HologramMesh->SetMaterial(MaterialIndex, OverrideMaterial);
		}
	}

	if (LifeSeconds > 0.0f)
	{
		SetLifeSpan(LifeSeconds);
	}
}
