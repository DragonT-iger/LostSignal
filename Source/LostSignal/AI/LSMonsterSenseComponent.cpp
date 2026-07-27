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

ULSMonsterSenseComponent::ULSMonsterSenseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = ActiveSenseTickInterval;
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

	ApplySenseTickInterval();
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

	UpdateDistanceDormancy();
	if (bIsDormantByDistance)
	{
		return;
	}

	UpdateSensing(DeltaTime);
}

void ULSMonsterSenseComponent::ApplyArchetype(const FLSMonsterArchetypeRow& Row)
{
	BaseSightRadius = Row.Sight_Radius;
	HearingRadius = Row.Hearing_Radius;
	PatrolMoveSpeedMultiplier = FMath::Max(0.0f, Row.Patrol_Speed);
	AlertMoveSpeedMultiplier = FMath::Max(0.0f, Row.Chase_Speed);
	bHasArchetypeMoveSpeedMultipliers = true;
}

void ULSMonsterSenseComponent::RegisterNoiseEvent(const FLSNoiseEvent& NoiseEvent)
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || NoiseEvent.RadiusCm <= 0.0f || NoiseEvent.NoiseInstigator == OwnerActor)
	{
		return;
	}

	const float EffectiveHearingRadius = HearingRadius + NoiseEvent.RadiusCm;
	if (EffectiveHearingRadius <= 0.0f)
	{
		return;
	}

	const float Distance2D = FVector::Dist2D(OwnerActor->GetActorLocation(), NoiseEvent.Location);
	if (Distance2D > EffectiveHearingRadius)
	{
		return;
	}

	// 교전 영역 밖 소음은 무시한다(보스 아레나 — 영역 밖 대상에 반응하지 않음).
	if (ShouldSuppressReturnHomeInterest(NoiseEvent.Location) || IsLocationOutsideEngageArea(NoiseEvent.Location))
	{
		return;
	}

	InterestLocation = NoiseEvent.Location;
	bHasInterestLocation = true;
	bIsDormantByDistance = false;
	ApplySenseTickInterval();
}

void ULSMonsterSenseComponent::SetCurrentTargetFromDamage(AActor* DamageInstigator)
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !DamageInstigator || DamageInstigator == OwnerActor)
	{
		return;
	}

	const FVector DamageInstigatorLocation = DamageInstigator->GetActorLocation();
	// 교전 영역 밖 공격자는 어그로를 잡지 않는다(영역 밖 원거리 견제 차단).
	if (ShouldSuppressReturnHomeInterest(DamageInstigatorLocation) || IsLocationOutsideEngageArea(DamageInstigatorLocation))
	{
		return;
	}

	// 피격은 즉시 어그로(가시 여부 무관). 신규 획득이면 앵커를 캡처하고 기억 타이머를 리셋한다.
	SetTarget(DamageInstigator, !CurrentTarget.IsValid());
	InterestLocation = DamageInstigatorLocation;
	bHasInterestLocation = true;
}

bool ULSMonsterSenseComponent::HasVisualTarget() const
{
	return bTargetVisibleThisTick;
}

bool ULSMonsterSenseComponent::HasTarget() const
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
	// P0: 이탈 판정은 최초 인식 위치(앵커) 기준. 복귀 목적지인 Home과 분리한다.
	if (!bHasAggroAnchor || LeashDistance <= 0.0f)
	{
		return false;
	}

	const AActor* OwnerActor = GetOwner();
	return OwnerActor && FVector::Dist2D(OwnerActor->GetActorLocation(), AggroAnchorLocation) > LeashDistance;
}

float ULSMonsterSenseComponent::GetCurrentSightRadius() const
{
	if (bForceMaxSightRadius)
	{
		return MaxSightRadius;
	}

	return BaseSightRadius;
}

void ULSMonsterSenseComponent::SetForceMaxSightRadius(bool bInForceMaxSightRadius)
{
	bForceMaxSightRadius = bInForceMaxSightRadius;
}

void ULSMonsterSenseComponent::SetReturnHomeMode(bool bInReturnHomeMode)
{
	bReturnHomeMode = bInReturnHomeMode;
}

void ULSMonsterSenseComponent::ClearInterest()
{
	ReleaseTarget();
	InterestLocation = FVector::ZeroVector;
	bHasInterestLocation = false;
}

void ULSMonsterSenseComponent::UpdateDistanceDormancy()
{
	NearestPlayerDistance = ComputeNearestPlayerDistance();

	if (ShouldForceActiveSense())
	{
		bIsDormantByDistance = false;
		ApplySenseTickInterval();
		return;
	}

	const float WakeThreshold = FMath::Max(0.0f, WakeDistance);
	const float SleepThreshold = FMath::Max(WakeThreshold, SleepDistance);
	const bool bNextDormant = bIsDormantByDistance
		? NearestPlayerDistance > WakeThreshold
		: NearestPlayerDistance > SleepThreshold;

	if (bIsDormantByDistance != bNextDormant)
	{
		bIsDormantByDistance = bNextDormant;
		bTargetVisibleThisTick = false;
		ApplySenseTickInterval();
	}
}

float ULSMonsterSenseComponent::ComputeNearestPlayerDistance() const
{
	UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();
	if (!World || !OwnerActor)
	{
		return MAX_flt;
	}

	float NearestDistanceSq = MAX_flt;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PlayerController = It->Get();
		const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
		if (!Pawn || Pawn == OwnerActor)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared2D(OwnerActor->GetActorLocation(), Pawn->GetActorLocation());
		NearestDistanceSq = FMath::Min(NearestDistanceSq, DistSq);
	}

	return NearestDistanceSq < MAX_flt ? FMath::Sqrt(NearestDistanceSq) : MAX_flt;
}

bool ULSMonsterSenseComponent::ShouldForceActiveSense() const
{
	return !bEnableDistanceDormancy
		|| CurrentTarget.IsValid()
		|| bHasInterestLocation
		|| bReturnHomeMode
		|| IsOwnerAttacking();
}

void ULSMonsterSenseComponent::ApplySenseTickInterval()
{
	const float TargetInterval = bIsDormantByDistance ? DormantSenseTickInterval : ActiveSenseTickInterval;
	PrimaryComponentTick.TickInterval = FMath::Max(0.0f, TargetInterval);
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
	// P0(해제)는 여기서 직접 처리하지 않는다. IsBeyondLeashDistance()(앵커 기준)를
	// 데이터로만 노출하고, 실제 타겟 해제는 StateTree가 ReturnHome으로 전이하며
	// ClearInterest를 호출할 때 일어난다. (데이터=SenseComponent, 전이=StateTree)

	// 교전 영역 밖으로 나간 타겟은 즉시 해제(보스 아레나 이탈 = 전투 해제).
	// 조사(Alert) 없이 바로 복귀하도록 관심 위치도 함께 지운다.
	if (CurrentTarget.IsValid() && IsLocationOutsideEngageArea(CurrentTarget.Get()->GetActorLocation(), EngageAreaReleaseBuffer))
	{
		ClearInterest();
	}

	// P2: 시야(전방 부채꼴 + LOS) 내 가장 가까운 대상.
	AActor* VisibleTarget = FindBestVisibleTarget();
	if (VisibleTarget && ShouldSuppressReturnHomeInterest(VisibleTarget->GetActorLocation()))
	{
		VisibleTarget = nullptr;
	}

	// 공격 중에는 타겟 식별자 전환을 보류한다(모션 캔슬 금지, 기획 예외 규칙).
	const bool bAttacking = IsOwnerAttacking();
	const bool bHoldDuringAttack = bAttacking && CurrentTarget.IsValid() && CurrentTarget.Get() != VisibleTarget;
	if (VisibleTarget && !bHoldDuringAttack && CurrentTarget.Get() != VisibleTarget)
	{
		// 타겟이 없다가 생긴 경우에만 앵커를 캡처한다(P2 최근접 전환은 앵커 유지).
		SetTarget(VisibleTarget, !CurrentTarget.IsValid());
	}

	// 보유 타겟이 이번 틱 실제로 보이는지 판정한다.
	AActor* HeldTarget = CurrentTarget.Get();
	bTargetVisibleThisTick = HeldTarget && CanSeeActor(HeldTarget);

	if (bTargetVisibleThisTick)
	{
		InterestLocation = HeldTarget->GetActorLocation();
		bHasInterestLocation = true;
		TimeSinceTargetLastSeen = 0.0f;
		return;
	}

	// P3: 시야 상실. 기억 시간 동안 타겟 유지(InterestLocation 동결), 초과 시 해제.
	// 공격 중에는 타이머를 멈춰 해제를 미룬다.
	if (CurrentTarget.IsValid() && !bAttacking)
	{
		TimeSinceTargetLastSeen += DeltaTime;
		if (TimeSinceTargetLastSeen >= LostSightMemorySeconds)
		{
			ReleaseTarget();
		}
	}
}

void ULSMonsterSenseComponent::SetTarget(AActor* NewTarget, bool bCaptureAnchor)
{
	CurrentTarget = NewTarget;
	TimeSinceTargetLastSeen = 0.0f;
	bIsDormantByDistance = false;
	ApplySenseTickInterval();

	if (bCaptureAnchor)
	{
		if (const AActor* OwnerActor = GetOwner())
		{
			AggroAnchorLocation = OwnerActor->GetActorLocation();
			bHasAggroAnchor = true;
		}
	}
}

void ULSMonsterSenseComponent::ReleaseTarget()
{
	CurrentTarget.Reset();
	bHasAggroAnchor = false;
	TimeSinceTargetLastSeen = 0.0f;
	bTargetVisibleThisTick = false;
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

		// 교전 영역 밖 대상은 보여도 획득하지 않는다.
		if (IsLocationOutsideEngageArea(Pawn->GetActorLocation()) || !CanSeeActor(Pawn))
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

bool ULSMonsterSenseComponent::IsLocationBeyondLeashDistance(const FVector& Location) const
{
	return LeashDistance > 0.0f && FVector::Dist2D(Location, HomeLocation) > LeashDistance;
}

bool ULSMonsterSenseComponent::IsLocationOutsideEngageArea(const FVector& Location, float ExtraBuffer) const
{
	if (!bUseEngageArea || EngageAreaRadius <= 0.0f)
	{
		return false;
	}

	return FVector::Dist2D(Location, HomeLocation) > EngageAreaRadius + ExtraBuffer;
}

bool ULSMonsterSenseComponent::ShouldSuppressReturnHomeInterest(const FVector& InterestCandidateLocation) const
{
	return bReturnHomeMode && IsLocationBeyondLeashDistance(InterestCandidateLocation);
}

bool ULSMonsterSenseComponent::IsOwnerDead() const
{
	const ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner());
	const UAbilitySystemComponent* ASC = OwnerCharacter ? OwnerCharacter->GetAbilitySystemComponent() : nullptr;
	return ASC && ASC->HasMatchingGameplayTag(LSGameplayTags::State_Dead);
}

bool ULSMonsterSenseComponent::IsOwnerAttacking() const
{
	const ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner());
	const UAbilitySystemComponent* ASC = OwnerCharacter ? OwnerCharacter->GetAbilitySystemComponent() : nullptr;
	return ASC && ASC->HasMatchingGameplayTag(LSGameplayTags::Combat_Attacking);
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

	if (HearingRadius > 0.0f)
	{
		DrawDebugCircle(World, Origin, HearingRadius, 48, FColor::Blue, false, Duration, 0, 1.5f, FVector::ForwardVector, FVector::RightVector, false);
	}

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
