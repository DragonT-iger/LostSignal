#include "Vision/LSMPCVisionSourceComponent.h"

#include "GameFramework/Pawn.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"

ULSMPCVisionSourceComponent::ULSMPCVisionSourceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void ULSMPCVisionSourceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ShouldUpdateForLocalView())
	{
		PushParametersToMPC(1.0f);
	}
}

void ULSMPCVisionSourceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ShouldUpdateForLocalView())
	{
		ResetParameters();
	}

	Super::EndPlay(EndPlayReason);
}

void ULSMPCVisionSourceComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ShouldUpdateForLocalView())
	{
		return;
	}

	PushParametersToMPC(1.0f);
}

bool ULSMPCVisionSourceComponent::ShouldUpdateForLocalView() const
{
	if (VisionParameterCollection == nullptr || GetWorld() == nullptr || GetOwner() == nullptr)
	{
		return false;
	}

	if (!bOnlyLocallyControlled)
	{
		return true;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn != nullptr && OwnerPawn->IsLocallyControlled();
}

FVector ULSMPCVisionSourceComponent::GetVisionCenterWorld() const
{
	return GetOwner() != nullptr ? GetOwner()->GetActorLocation() + WorldOffset : WorldOffset;
}

void ULSMPCVisionSourceComponent::PushParametersToMPC(const float EnabledValue) const
{
	if (VisionParameterCollection == nullptr || GetWorld() == nullptr)
	{
		return;
	}

	const FVector CenterWorld = GetVisionCenterWorld();

	UKismetMaterialLibrary::SetVectorParameterValue(
		GetWorld(),
		VisionParameterCollection,
		CenterParameterName,
		FLinearColor(CenterWorld.X, CenterWorld.Y, CenterWorld.Z, 1.0f));

	UKismetMaterialLibrary::SetScalarParameterValue(
		GetWorld(),
		VisionParameterCollection,
		RadiusParameterName,
		Radius);

	UKismetMaterialLibrary::SetScalarParameterValue(
		GetWorld(),
		VisionParameterCollection,
		FeatherParameterName,
		FeatherWidth);

	UKismetMaterialLibrary::SetScalarParameterValue(
		GetWorld(),
		VisionParameterCollection,
		EnabledParameterName,
		EnabledValue);
}

void ULSMPCVisionSourceComponent::ResetParameters() const
{
	if (VisionParameterCollection == nullptr || GetWorld() == nullptr)
	{
		return;
	}

	UKismetMaterialLibrary::SetScalarParameterValue(
		GetWorld(),
		VisionParameterCollection,
		EnabledParameterName,
		0.0f);
}
