#include "AI/LSMonsterSenseComponent.h"

#include "AbilitySystemComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Data/LSMonsterArchetypeRow.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GAS/LSGameplayTags.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/LSNoiseSubsystem.h"
#include "Gameplay/LSNoiseTypes.h"
#include "LostSignal.h"

ULSMonsterSenseComponent::ULSMonsterSenseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void ULSMonsterSenseComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const AActor* OwnerActor = GetOwner())
	{
		HomeLocation = OwnerActor->GetActorLocation();

		if (OwnerActor->HasAuthority())
		{
			if (ULSNoiseSubsystem* NoiseSubsystem = GetWorld()->GetSubsystem<ULSNoiseSubsystem>())
			{
				NoiseSubsystem->RegisterListener(this);
			}
		}
	}
}

void ULSMonsterSenseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (ULSNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<ULSNoiseSubsystem>())
		{
			NoiseSubsystem->UnregisterListener(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ULSMonsterSenseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (IsOwnerDead())
	{
		ClearInterest();
		SetComponentTickEnabled(false);
		return;
	}

#if !UE_BUILD_SHIPPING
	if (bDrawSenseDebug)
	{
		DrawSenseDebug();
	}
#endif

	if (!OwnerActor->HasAuthority())
	{
		return;
	}

	UpdateSensing(DeltaTime);
}

void ULSMonsterSenseComponent::ApplyArchetype(const FLSMonsterArchetypeRow& Row)
{
	BaseSightRadius = Row.Sight_Radius;
	HearingRadius = Row.Hearing_Radius;
	AlertMoveSpeedMultiplier = Row.Chase_Speed;
}

void ULSMonsterSenseComponent::RegisterNoiseEvent(const FLSNoiseEvent& NoiseEvent)
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || NoiseEvent.RadiusCm <= 0.0f)
	{
		return;
	}

	const float EffectiveHearingRadius = FMath::Min(HearingRadius, NoiseEvent.RadiusCm);
	if (EffectiveHearingRadius <= 0.0f)
	{
		return;
	}

	if (FVector::DistSquared2D(OwnerActor->GetActorLocation(), NoiseEvent.Location) > FMath::Square(EffectiveHearingRadius))
	{
		return;
	}

	InterestLocation = NoiseEvent.Location;
	bHasInterestLocation = true;
}

bool ULSMonsterSenseComponent::HasVisualTarget() const
{
	return CurrentTarget.IsValid();
}

bool ULSMonsterSenseComponent::HasInterestLocation() const
{
	return bHasInterestLocation;
}

FVector ULSMonsterSenseComponent::GetInterestLocation() const
{
	return bHasInterestLocation ? InterestLocation : HomeLocation;
}

float ULSMonsterSenseComponent::GetDistanceFromHome() const
{
	const AActor* OwnerActor = GetOwner();
	return OwnerActor ? FVector::Dist2D(OwnerActor->GetActorLocation(), HomeLocation) : 0.0f;
}

bool ULSMonsterSenseComponent::IsBeyondLeashDistance() const
{
	return LeashDistance > 0.0f && GetDistanceFromHome() > LeashDistance;
}

float ULSMonsterSenseComponent::GetCurrentSightRadius() const
{
	if (bForceMaxSightRadius)
	{
		return MaxSightRadius;
	}

	return FMath::Clamp(BaseSightRadius * ThreatMultiplier, BaseSightRadius, MaxSightRadius);
}

void ULSMonsterSenseComponent::SetThreatMultiplier(float InThreatMultiplier)
{
	ThreatMultiplier = FMath::Max(1.0f, InThreatMultiplier);
}

void ULSMonsterSenseComponent::SetForceMaxSightRadius(bool bInForceMaxSightRadius)
{
	bForceMaxSightRadius = bInForceMaxSightRadius;
}

void ULSMonsterSenseComponent::ClearVisualTarget()
{
	CurrentTarget.Reset();
}

void ULSMonsterSenseComponent::ClearInterest()
{
	CurrentTarget.Reset();
	InterestLocation = FVector::ZeroVector;
	bHasInterestLocation = false;
}

bool ULSMonsterSenseComponent::CanSeeActor(const AActor* Actor) const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !Actor)
	{
		return false;
	}

	const FVector Origin = OwnerActor->GetActorLocation();
	const FVector Target = Actor->GetActorLocation();
	FVector ToTarget = Target - Origin;
	ToTarget.Z = 0.0f;

	if (ToTarget.IsNearlyZero())
	{
		return true;
	}

	if (ToTarget.SizeSquared() > FMath::Square(GetCurrentSightRadius()))
	{
		return false;
	}

	const FVector Forward = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
	const FVector Direction = ToTarget.GetSafeNormal();
	const float CosThreshold = FMath::Cos(FMath::DegreesToRadians(SightHalfAngleDegrees));
	if (FVector::DotProduct(Forward, Direction) < CosThreshold)
	{
		return false;
	}

	FHitResult HitResult;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LSMonsterVision), false, OwnerActor);
	Params.AddIgnoredActor(Actor);

	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Origin + FVector(0.0f, 0.0f, 50.0f),
		Target + FVector(0.0f, 0.0f, 50.0f),
		ECC_Visibility,
		Params
	);

	return !bBlocked;
}

void ULSMonsterSenseComponent::UpdateSensing(float DeltaTime)
{
	AActor* VisibleTarget = FindBestVisibleTarget();
	if (VisibleTarget)
	{
		CurrentTarget = VisibleTarget;
		InterestLocation = VisibleTarget->GetActorLocation();
		bHasInterestLocation = true;

		/*UE_LOG(LogLS, Warning, TEXT("Sense Hit: Owner=%s Target=%s HasVisual=%d"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(CurrentTarget.Get()),
			HasVisualTarget());*/

		return;
	}

	CurrentTarget.Reset();
}

AActor* ULSMonsterSenseComponent::FindBestVisibleTarget() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AActor* BestTarget = nullptr;
	float BestDistSq = MAX_flt;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PlayerController = It->Get();
		if (!PlayerController)
		{
			continue;
		}

		APawn* Pawn = PlayerController->GetPawn();
		if (!Pawn || Pawn == GetOwner())
		{
			continue;
		}

		if (!CanSeeActor(Pawn))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared2D(GetOwner()->GetActorLocation(), Pawn->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Pawn;
		}
	}

	return BestTarget;
}

bool ULSMonsterSenseComponent::IsOwnerDead() const
{
	const ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner());
	const UAbilitySystemComponent* ASC = OwnerCharacter ? OwnerCharacter->GetAbilitySystemComponent() : nullptr;
	return ASC && ASC->HasMatchingGameplayTag(LSGameplayTags::State_Dead);
}

void ULSMonsterSenseComponent::DrawSenseDebug() const
{
	UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();
	if (!World || !OwnerActor)
	{
		return;
	}

	const FVector Origin = OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, SenseDebugDrawHeight);
	const FVector Forward = OwnerActor->GetActorForwardVector().GetSafeNormal2D();
	if (Forward.IsNearlyZero())
	{
		return;
	}

	const float Radius = GetCurrentSightRadius();
	const float Duration = FMath::Max(PrimaryComponentTick.TickInterval * 1.5f, 0.05f);
	const int32 SegmentCount = 24;
	const FColor SightColor = HasVisualTarget() ? FColor::Red : FColor::Green;

	const FVector LeftEdge = Forward.RotateAngleAxis(-SightHalfAngleDegrees, FVector::UpVector).GetSafeNormal2D();
	const FVector RightEdge = Forward.RotateAngleAxis(SightHalfAngleDegrees, FVector::UpVector).GetSafeNormal2D();

	DrawDebugLine(World, Origin, Origin + (LeftEdge * Radius), SightColor, false, Duration, 0, 2.0f);
	DrawDebugLine(World, Origin, Origin + (RightEdge * Radius), SightColor, false, Duration, 0, 2.0f);

	FVector PreviousPoint = Origin + (LeftEdge * Radius);
	for (int32 SegmentIndex = 1; SegmentIndex <= SegmentCount; ++SegmentIndex)
	{
		const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
		const float Angle = FMath::Lerp(-SightHalfAngleDegrees, SightHalfAngleDegrees, Alpha);
		const FVector Direction = Forward.RotateAngleAxis(Angle, FVector::UpVector).GetSafeNormal2D();
		const FVector CurrentPoint = Origin + (Direction * Radius);

		DrawDebugLine(World, PreviousPoint, CurrentPoint, SightColor, false, Duration, 0, 2.0f);
		PreviousPoint = CurrentPoint;
	}

	if (HasVisualTarget())
	{
		const FVector DebugInterestLocation = InterestLocation + FVector(0.0f, 0.0f, SenseDebugDrawHeight);
		DrawDebugLine(World, Origin, DebugInterestLocation, FColor::Yellow, false, Duration, 0, 1.5f);
		DrawDebugSphere(World, DebugInterestLocation, 20.0f, 8, FColor::Yellow, false, Duration);
	}
	else if (HasInterestLocation())
	{
		const FVector DebugInterestLocation = InterestLocation + FVector(0.0f, 0.0f, SenseDebugDrawHeight);
		DrawDebugSphere(World, DebugInterestLocation, 20.0f, 8, FColor::Orange, false, Duration);
	}
}
