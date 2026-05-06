#include "Combat/LSAimComponent.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

ULSAimComponent::ULSAimComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULSAimComponent::UpdateFacing(float DeltaSeconds)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	FVector AimPoint;
	if (!GetAimPoint(AimPoint))
	{
		return;
	}

	FVector LookDirection = AimPoint - OwnerActor->GetActorLocation();
	LookDirection.Z = 0.0f;
	if (LookDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator TargetRotation = LookDirection.Rotation();
	const FRotator NewRotation = FMath::RInterpTo(
		OwnerActor->GetActorRotation(),
		FRotator(0.0f, TargetRotation.Yaw, 0.0f),
		DeltaSeconds,
		MouseFacingInterpSpeed);

	OwnerActor->SetActorRotation(NewRotation);
}

bool ULSAimComponent::GetAimPoint(FVector& OutAimPoint) const
{
	const AActor* OwnerActor = GetOwner();
	APlayerController* PlayerController = ResolvePlayerController();
	if (!OwnerActor || !PlayerController)
	{
		return false;
	}

	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;
	if (!PlayerController->DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return false;
	}

	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}

	const float AimPlaneZ = OwnerActor->GetActorLocation().Z + AimPlaneHeightOffset;
	const float TraceDistance = (AimPlaneZ - WorldOrigin.Z) / WorldDirection.Z;
	if (TraceDistance <= 0.0f)
	{
		return false;
	}

	OutAimPoint = WorldOrigin + (WorldDirection * TraceDistance);
	return true;
}

FVector ULSAimComponent::GetAimDirection() const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return FVector::ForwardVector;
	}

	FVector AimPoint;
	if (!GetAimPoint(AimPoint))
	{
		return OwnerActor->GetActorForwardVector();
	}

	FVector AimDirection = AimPoint - OwnerActor->GetActorLocation();
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		return OwnerActor->GetActorForwardVector();
	}

	return AimDirection.GetSafeNormal();
}

APlayerController* ULSAimComponent::ResolvePlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
	if (!PlayerController || !PlayerController->IsLocalPlayerController())
	{
		return nullptr;
	}

	return PlayerController;
}
