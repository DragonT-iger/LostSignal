#include "Skills/LSSkillPreviewComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Skills/LSSkillAreaPreviewActor.h"

ULSSkillPreviewComponent::ULSSkillPreviewComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PreviewActorClass = ALSSkillAreaPreviewActor::StaticClass();
}

bool ULSSkillPreviewComponent::BeginAreaPreview(const FLSSkillAreaPreviewSpec& PreviewSpec)
{
	if (GetNetMode() == NM_DedicatedServer || !PreviewActorClass)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (!ActivePreviewActor)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ActivePreviewActor = World->SpawnActor<ALSSkillAreaPreviewActor>(
			PreviewActorClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams);
	}

	if (!ActivePreviewActor)
	{
		return false;
	}

	ActivePreviewActor->SetAreaSpec(PreviewSpec);
	ActivePreviewActor->SetPreviewVisible(true);
	return true;
}

void ULSSkillPreviewComponent::UpdateAreaPreview(const FVector& WorldLocation, const FRotator& WorldRotation)
{
	if (ActivePreviewActor)
	{
		ActivePreviewActor->SetPreviewTransform(WorldLocation, WorldRotation);
	}
}

void ULSSkillPreviewComponent::EndAreaPreview()
{
	if (!ActivePreviewActor)
	{
		return;
	}

	ActivePreviewActor->Destroy();
	ActivePreviewActor = nullptr;
}

void ULSSkillPreviewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndAreaPreview();

	Super::EndPlay(EndPlayReason);
}
