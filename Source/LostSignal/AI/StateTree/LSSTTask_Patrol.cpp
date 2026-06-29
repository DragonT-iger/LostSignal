#include "AI/StateTree/LSSTTask_Patrol.h"

#include "AIController.h"
#include "Characters/LSEnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"
#include "LostSignal.h"

namespace
{
	// 반경을 벗어나는 방향이면 Home 쪽으로 분산 편향한 이동 방향을 고른다.
	FVector ResolvePatrolWalkDirection(const FVector& Current, const FVector& Home, float WalkDistance, float PatrolRadius, float HomeBiasSpreadDegrees)
	{
		const float RandomYaw = FMath::FRandRange(0.0f, 360.0f);
		FVector Direction = FRotator(0.0f, RandomYaw, 0.0f).Vector();

		const FVector Candidate = Current + Direction * WalkDistance;
		if (FVector::Dist2D(Candidate, Home) > PatrolRadius)
		{
			FVector ToHome = Home - Current;
			ToHome.Z = 0.0f;
			ToHome = ToHome.GetSafeNormal();
			if (!ToHome.IsNearlyZero())
			{
				const float Spread = FMath::FRandRange(-HomeBiasSpreadDegrees, HomeBiasSpreadDegrees);
				Direction = ToHome.RotateAngleAxis(Spread, FVector::UpVector);
			}
		}

		return Direction.GetSafeNormal2D();
	}

	// 직선 구간 목적지: 방향 선택 → 반경 클램프 → 네비 경계 정지(여유거리) → 도달 가능 위치 보정.
	FVector ComputePatrolWalkDestination(UWorld* World, const FVector& Current, const FVector& Home,
		float WalkDistance, float PatrolRadius, float ObstacleStopMargin, float HomeBiasSpreadDegrees)
	{
		const FVector Direction = ResolvePatrolWalkDirection(Current, Home, WalkDistance, PatrolRadius, HomeBiasSpreadDegrees);
		FVector Destination = Current + Direction * WalkDistance;

		// 반경 안전 클램프: Home 중심 원 위로 투영.
		if (FVector::Dist2D(Destination, Home) > PatrolRadius)
		{
			FVector FromHome = Destination - Home;
			FromHome.Z = 0.0f;
			Destination = Home + FromHome.GetSafeNormal() * PatrolRadius;
			Destination.Z = Current.Z;
		}

		if (!World)
		{
			return Destination;
		}

		// 네비 경계(벽)에 막히면 여유거리만큼 앞에서 멈춤.
		FVector HitLocation;
		if (UNavigationSystemV1::NavigationRaycast(World, Current, Destination, HitLocation))
		{
			const float HitDistance = FVector::Dist2D(Current, HitLocation);
			const float StopDistance = FMath::Max(0.0f, HitDistance - ObstacleStopMargin);
			Destination = Current + Direction * StopDistance;
		}

		// 도달 가능한 네비 위치로 보정.
		if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World))
		{
			FNavLocation ProjectedLocation;
			if (NavSys->ProjectPointToNavigation(Destination, ProjectedLocation))
			{
				Destination = ProjectedLocation.Location;
			}
		}

		return Destination;
	}
}

FLSSTTask_Patrol::FLSSTTask_Patrol()
{
	bShouldCallTick = true;
	bShouldCopyBoundPropertiesOnTick = true;
}

EStateTreeRunStatus FLSSTTask_Patrol::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UE_LOG(LogLS, Warning, TEXT("Enter Patrol State Task Patrol"));

	// 에셋 AIController 바인딩 누락 대비 컨텍스트 소유자에서 해석.
	if (!InstanceData.AIController)
	{
		InstanceData.AIController = Cast<AAIController>(Context.GetOwner());
	}

	if (!InstanceData.EnemyCharacter && InstanceData.AIController)
	{
		InstanceData.EnemyCharacter = Cast<ALSEnemyCharacter>(InstanceData.AIController->GetPawn());
	}

	if (!InstanceData.AIController || !InstanceData.EnemyCharacter)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed To Enter Patrol State"));
		return EStateTreeRunStatus::Failed;
	}

	// HomeLocation 미바인딩(0) 시 진입 위치를 순찰 기준점으로 폴백.
	if (InstanceData.HomeLocation.IsNearlyZero())
	{
		InstanceData.HomeLocation = InstanceData.EnemyCharacter->GetActorLocation();
	}

	if (UCharacterMovementComponent* MovementComponent = InstanceData.EnemyCharacter->GetCharacterMovement())
	{
		InstanceData.PreviousMaxWalkSpeed = MovementComponent->MaxWalkSpeed;
		MovementComponent->MaxWalkSpeed = InstanceData.PreviousMaxWalkSpeed * FMath::Max(0.0f, InstanceData.PatrolSpeedMultiplier);
	}

	InstanceData.LookAroundElapsed = 0.0f;
	BeginWalkSegment(InstanceData);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FLSSTTask_Patrol::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.AIController)
	{
		return EStateTreeRunStatus::Running;
	}

	switch (InstanceData.Phase)
	{
	case ELSPatrolPhase::Walking:
		// 도착/실패/정지로 이동이 끝나면 둘러보기로 전환.
		if (InstanceData.AIController->GetMoveStatus() != EPathFollowingStatus::Moving)
		{
			InstanceData.Phase = ELSPatrolPhase::LookingAround;
			InstanceData.LookAroundElapsed = 0.0f;
		}
		break;

	case ELSPatrolPhase::LookingAround:
		InstanceData.LookAroundElapsed += DeltaTime;
		if (InstanceData.LookAroundElapsed >= InstanceData.LookAroundDuration)
		{
			BeginWalkSegment(InstanceData);
		}
		break;
	}

	return EStateTreeRunStatus::Running;
}

void FLSSTTask_Patrol::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.AIController)
	{
		InstanceData.AIController->StopMovement();
	}

	if (InstanceData.EnemyCharacter && InstanceData.PreviousMaxWalkSpeed > 0.0f)
	{
		if (UCharacterMovementComponent* MovementComponent = InstanceData.EnemyCharacter->GetCharacterMovement())
		{
			MovementComponent->MaxWalkSpeed = InstanceData.PreviousMaxWalkSpeed;
		}
	}
}

void FLSSTTask_Patrol::BeginWalkSegment(FInstanceDataType& InstanceData) const
{
	UE_LOG(LogLS, Warning, TEXT("AI BeginWalkSegment Called"));

	AAIController* AIController = InstanceData.AIController;
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!AIController || !Pawn)
	{
		UE_LOG(LogLS, Warning, TEXT("AI BeginWalkSegment Pawn is Null"));
		return;
	}

	const FVector Current = Pawn->GetActorLocation();
	const FVector Destination = ComputePatrolWalkDestination(AIController->GetWorld(), Current, InstanceData.HomeLocation,
		InstanceData.WalkDistance, InstanceData.PatrolRadius, InstanceData.ObstacleStopMargin, InstanceData.HomeBiasSpreadDegrees);

	AIController->MoveToLocation(Destination, InstanceData.AcceptanceRadius, /*bStopOnOverlap*/true, /*bUsePathfinding*/true, /*bProjectDestinationToNavigation*/true);
	InstanceData.Phase = ELSPatrolPhase::Walking;
}
