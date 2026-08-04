#include "Vision/LSVisionComponent.h"

#include "Camera/CameraComponent.h"
#include "Combat/LSAimComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "LostSignal.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Vision/LSVisionSettings.h"
#include "Vision/LSVisionMaskRenderer.h"
#include "Vision/LSVisionSolver.h"
#include "Vision/LSVisionSubsystem.h"
#include "Vision/LSVisionSurfaceComponent.h"
#include "Vision/LSVisionTargetComponent.h"

ULSVisionComponent::ULSVisionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

// Starts periodic vision updates and optionally injects the vision post-process material into the local camera.
void ULSVisionComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeLocalVision();
}

void ULSVisionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const bool bShouldBeInitialized = IsLocalVisionController();
	if (bShouldBeInitialized && !bLocalVisionInitialized)
	{
		InitializeLocalVision();
	}
	else if (!bShouldBeInitialized && bLocalVisionInitialized)
	{
		ShutdownLocalVision();
	}
}

void ULSVisionComponent::InitializeLocalVision()
{
	if (bLocalVisionInitialized || !IsLocalVisionController())
	{
		return;
	}

	bLocalVisionInitialized = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			VisionUpdateTimerHandle,
			this,
			&ULSVisionComponent::UpdateVisionPolygon,
			UpdateInterval,
			true);
	}

	// 아래 세 실패는 모두 "시야 포스트 프로세스가 화면에 안 붙는다"로 이어진다. 조용히 넘기면 원인 추적이 불가능하므로 로그를 남긴다.
	if (PostProcessMaterial == nullptr)
	{
		UE_LOG(LogLS, Warning, TEXT("LSVisionComponent '%s': PostProcessMaterial 미할당 — 시야 포스트 프로세스가 적용되지 않습니다."),
			*GetNameSafe(GetOwner()));
	}
	else
	{
		PostProcessMID = UMaterialInstanceDynamic::Create(PostProcessMaterial, this);

		if (PostProcessMID == nullptr)
		{
			UE_LOG(LogLS, Warning, TEXT("LSVisionComponent '%s': PostProcessMaterial '%s'로 MID 생성 실패 — 시야 포스트 프로세스가 적용되지 않습니다."),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(PostProcessMaterial));
		}
		else
		{
			PostProcessMID->SetScalarParameterValue(EnableParamName, bEnableVision ? 1.0f : 0.0f);

			if (UCameraComponent* Camera = GetOwner() ? GetOwner()->FindComponentByClass<UCameraComponent>() : nullptr)
			{
				Camera->PostProcessSettings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, PostProcessMID));
			}
			else
			{
				UE_LOG(LogLS, Warning, TEXT("LSVisionComponent '%s': CameraComponent를 찾지 못해 시야 포스트 프로세스를 블렌더블에 등록하지 못했습니다."),
					*GetNameSafe(GetOwner()));
			}
		}
	}

	UpdateVisionPolygon();
}

void ULSVisionComponent::ShutdownLocalVision()
{
	if (!bLocalVisionInitialized)
	{
		return;
	}

	bLocalVisionInitialized = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(VisionUpdateTimerHandle);
	}

	if (PostProcessMID != nullptr)
	{
		if (UCameraComponent* Camera = GetOwner() ? GetOwner()->FindComponentByClass<UCameraComponent>() : nullptr)
		{
			Camera->PostProcessSettings.WeightedBlendables.Array.RemoveAll(
				[this](const FWeightedBlendable& WeightedBlendable)
				{
					return WeightedBlendable.Object == PostProcessMID;
				});
		}

		PostProcessMID = nullptr;
	}
}

// Stops the periodic vision update loop when the owning actor leaves the world.
void ULSVisionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShutdownLocalVision();

	Super::EndPlay(EndPlayReason);
}

// Recomputes the current visibility polygon and propagates the result to mask, surfaces, and targets.
void ULSVisionComponent::UpdateVisionPolygon()
{
	if (!IsLocalVisionController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr || GetOwner() == nullptr)
	{
		return;
	}

	ULSVisionSubsystem* VisionSubsystem = World->GetSubsystem<ULSVisionSubsystem>();
	if (VisionSubsystem == nullptr || VisionSubsystem->GetMaskRenderer() == nullptr)
	{
		// 이 레벨에서 시야/마스크가 전혀 동작하지 않는 원인 진단용. 매 프레임 스팸을 피해 1회만 남긴다.
		if (!bWarnedMissingMaskRenderer)
		{
			UE_LOG(LogLS, Warning, TEXT("LSVisionComponent '%s': 시야 갱신 스킵 — %s. 이 레벨에서 마스크/시야가 동작하지 않습니다."),
				*GetNameSafe(GetOwner()),
				VisionSubsystem == nullptr ? TEXT("VisionSubsystem 없음") : TEXT("MaskRenderer 미바인딩(미스폰)"));
			bWarnedMissingMaskRenderer = true;
		}

		return;
	}

	// 미바인딩 상태에서 복구되면(예: WP 스트리밍 완료) 한 번 알리고 경고 플래그를 리셋한다.
	if (bWarnedMissingMaskRenderer)
	{
		UE_LOG(LogLS, Log, TEXT("LSVisionComponent '%s': MaskRenderer 복구됨, 시야 갱신을 재개합니다."), *GetNameSafe(GetOwner()));
		bWarnedMissingMaskRenderer = false;
	}

	const FVector ActorLocation = GetOwner()->GetActorLocation();
	const FVector2D ActorLocation2D(ActorLocation.X, ActorLocation.Y);

	// 플레이어 발 높이(바닥 기준 Z). 오클루더 슬라이스 평면과 마스크 투영의 "높이 0" 기준으로 공용한다.
	float PlayerFootZ = ActorLocation.Z;
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (const UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent())
		{
			PlayerFootZ -= Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	// 플레이어 발 높이 기준 모드면 오클루더 슬라이스 평면을 발 높이 + 오프셋으로 갱신(변할 때만 재슬라이스).
	if (const ULSVisionSettings* VisionSettings = GetDefault<ULSVisionSettings>(); VisionSettings != nullptr && VisionSettings->bSliceHeightFromPlayer)
	{
		VisionSubsystem->SetRuntimeSliceZ(PlayerFootZ + VisionSettings->OccluderSliceHeight);
	}

	// 마스크 폴리곤이 잘린 슬라이스 평면 Z. 머티리얼 노멀 푸시의 높이 기준(GroundZ)을 이 평면과 동일하게 맞춰
	// 푸시가 0이 되는 지점이 마스크가 유효한 평면과 일치하게 한다. bSliceHeightFromPlayer=false면 고정 절대 Z라 수직 이동에 불변.
	const float SliceZ = VisionSubsystem->GetRuntimeSliceZ();

	// 시야 방향은 항상 캐릭터→마우스 방향으로 잡는다(캐릭터 실제 facing이 공격/대시 등으로 고정돼도 시야는 조준을 따른다).
	// AimComponent가 마우스 조준 방향(없으면 액터 forward)을 반환하므로, 없을 때만 액터 forward로 폴백.
	FVector ActorForward = GetOwner()->GetActorForwardVector();
	if (const ULSAimComponent* AimComponent = GetOwner()->FindComponentByClass<ULSAimComponent>())
	{
		ActorForward = AimComponent->GetAimDirection();
	}
	const FVector2D ActorForward2D = FVector2D(ActorForward.X, ActorForward.Y).GetSafeNormal();

	// apex(레이 원점)를 캐릭터 뒤로 물리는 거리. 근접 원(VisionRadius) 안쪽이 콘 밖으로 빠지지 않도록
	// 반경보다 이 여유만큼 짧게 잡는다. 음수가 되면 apex가 캐릭터 앞으로 나와 콘이 뒤집히므로 0에서 막는다.
	constexpr float RayOriginPullbackMargin = 50.0f;
	const float RayOriginPullback = FMath::Max(VisionRadius - RayOriginPullbackMargin, 0.0f);
	const FVector2D RayOrigin2D = ActorLocation2D - (ActorForward2D * RayOriginPullback);

	// 플레이어 포즈·오클루더 토폴로지·활성화 플래그가 모두 직전과 같으면 폴리곤/마스크가 그대로이므로 재계산을 건너뛴다.
	// 단, 타겟(몬스터 등)은 플레이어가 멈춰 있어도 움직이므로 가시성 갱신은 계속 수행한다.
	const int32 CurrentTopologyVersion = VisionSubsystem->GetSegmentTopologyVersion();
	if (bHasSolvedOnce
		&& bLastEnableVision == bEnableVision
		&& LastSolveTopologyVersion == CurrentTopologyVersion
		&& ActorLocation2D.Equals(LastSolveOrigin, 0.5f)
		&& ActorForward2D.Equals(LastSolveForward, 0.001f))
	{
		// 비영구(1프레임) 디버그 선은 매 프레임 다시 그려야 하므로, 재계산을 건너뛰어도 캐시된 폴리곤으로 다시 그린다.
		if (bDrawDebugRays)
		{
			DrawDebugVisionRays();
		}

		// 폴리곤은 그대로지만 서피스가 새로 등록됐다면(WP 스트리밍 인 등) 그 서피스의 MID는 아직 마스크 파라미터가 비어 있다.
		// 재solve 없이 파라미터만 다시 푸시해, 플레이어가 멈춰 있어도 새 지오메트리가 즉시 올바르게 마스킹되게 한다.
		const int32 CurrentSurfaceRegistryVersion = VisionSubsystem->GetSurfaceRegistryVersion();
		if (LastSurfaceRegistryVersion != CurrentSurfaceRegistryVersion)
		{
			LastSurfaceRegistryVersion = CurrentSurfaceRegistryVersion;
			ApplyVisionParametersToSurfaces(LastSolveForward, SliceZ);
		}

		UpdateVisionTargets(ActorLocation2D);
		return;
	}

	bHasSolvedOnce = true;
	bLastEnableVision = bEnableVision;
	LastSolveTopologyVersion = CurrentTopologyVersion;
	LastSolveOrigin = ActorLocation2D;
	LastSolveForward = ActorForward2D;

	FLSVisionSolverInfo SolverInfo;
	SolverInfo.OriginPos = ActorLocation2D;
	SolverInfo.RayOriginPos = RayOrigin2D;
	SolverInfo.OriginForward = ActorForward2D;
	SolverInfo.HalfFovDegrees = HalfFOVDegrees;
	SolverInfo.VisionRadius = VisionRadius;
	SolverInfo.AngleEpsilon = 0.01f;
	SolverInfo.DivideAngleDegree = DivideAngleDegree;
	SolverInfo.MaxRayDistance = MaxRayDistance;

	VisionSubsystem->QuerySegmentsInRadius(RayOrigin2D, MaxRayDistance, SolverInfo.Segments);

	CurrentPolygon = FLSVisionSolver::Solve(SolverInfo);

	if (bDrawDebugRays)
	{
		DrawDebugVisionRays();
	}

	if (PostProcessMID != nullptr)
	{
		PostProcessMID->SetScalarParameterValue(EnableParamName, bEnableVision ? 1.0f : 0.0f);
		// XY = 시야 폴리곤 원점(마스크 투영 기준). Z 슬롯은 현재 머티리얼에서 미사용(투영은 XY, 노멀 푸시는 높이 무관)이라 슬라이스 평면 높이를 참고용으로만 실어둔다.
		PostProcessMID->SetVectorParameterValue(MaskOriginParamName, FLinearColor(CurrentPolygon.Origin.X, CurrentPolygon.Origin.Y, SliceZ, 0.0f));
		//MaskExtent -> RenderTarget을 World좌표범위로 치환한 값. ex)extent = 2500 -> uv 0 ~ 1 = world -2500 ~ 2500 크기, 원점은 플레이어 기준
		PostProcessMID->SetScalarParameterValue(MaskExtentParamName, CurrentPolygon.Extent);

		// 오클루더 벽이 자기 발자국 경계를 샘플해 통째로 어두워지는 것을 막는 노멀 푸시. 앞면 샘플을 시야 안쪽으로 민다.
		// 머티리얼: offset.xy = WorldNormal.XY × 이 값 (월드 유닛, 높이 무관).
		if (const ULSVisionSettings* VisionSettings = GetDefault<ULSVisionSettings>())
		{
			PostProcessMID->SetScalarParameterValue(SurfacePushParamName, VisionSettings->SurfaceNormalPush);
		}

		if (UTextureRenderTarget2D* VisibilityMaskRT = VisionSubsystem->GetVisibilityMaskRenderTarget())
		{
			PostProcessMID->SetTextureParameterValue(VisibilityMaskTextureParamName, VisibilityMaskRT);
		}
	}

	VisionSubsystem->GetMaskRenderer()->RequestMaskUpdate(CurrentPolygon);

	LastSurfaceRegistryVersion = VisionSubsystem->GetSurfaceRegistryVersion();
	ApplyVisionParametersToSurfaces(SolverInfo.OriginForward, SliceZ);

	UpdateVisionTargets(SolverInfo.OriginPos);
}

// Pushes the current polygon's mask placement into every registered surface material.
void ULSVisionComponent::ApplyVisionParametersToSurfaces(const FVector2D& Forward2D, const float SliceZ)
{
	ULSVisionSubsystem* VisionSubsystem = GetWorld() ? GetWorld()->GetSubsystem<ULSVisionSubsystem>() : nullptr;
	if (VisionSubsystem == nullptr)
	{
		return;
	}

	for (ULSVisionSurfaceComponent* SurfaceComponent : VisionSubsystem->GetRegisteredVisionSurfaces())
	{
		if (SurfaceComponent != nullptr)
		{
			SurfaceComponent->ApplyVisionParameters(
				VisionSubsystem->GetVisibilityMaskRenderTarget(),
				FVector(CurrentPolygon.Origin.X, CurrentPolygon.Origin.Y, SliceZ),
				CurrentPolygon.Extent,
				Forward2D);
		}
	}
}

// Draws each sampled visibility ray so the current endpoint-based solver can be inspected in the world.
void ULSVisionComponent::DrawDebugVisionRays() const
{
	UWorld* World = GetWorld();
	if (World == nullptr || CurrentPolygon.Points.Num() <= 1)
	{
		return;
	}

	// 비전 계산은 XY만 쓰므로, 디버그 선은 플레이어 Z를 기준으로 그린다.
	// (절대 Z를 쓰면 바닥이 0이 아닌 레벨에서 선이 지면 아래에 그려져 보이지 않는다.)
	const float DrawZ = (GetOwner() != nullptr ? GetOwner()->GetActorLocation().Z : 0.0f) + DebugRayZOffset;

	const FVector RayOrigin(CurrentPolygon.RayOrigin.X, CurrentPolygon.RayOrigin.Y, DrawZ);
	const bool bPersistentLines = DebugRayDuration > 0.0f;

	// Points[0]은 apex(RayOrigin) 자신이므로 건너뛴다. 이후 점들이 각 레이의 실제 히트 지점이다.
	for (int32 PointIndex = 1; PointIndex < CurrentPolygon.Points.Num(); ++PointIndex)
	{
		const FVector2D& Point2D = CurrentPolygon.Points[PointIndex];
		const FVector RayEnd(Point2D.X, Point2D.Y, DrawZ);

		DrawDebugLine(
			World,
			RayOrigin,
			RayEnd,
			DebugRayColor,
			bPersistentLines,
			DebugRayDuration,
			0,
			DebugRayThickness);
	}
}

// Limits vision simulation to the locally controlled pawn so remote pawns do not drive local rendering.
bool ULSVisionComponent::IsLocalVisionController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr || !OwnerPawn->IsPlayerControlled())
	{
		return false;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(OwnerPawn->GetController());
	return PlayerController != nullptr && PlayerController->IsLocalPlayerController();
}

// Checks whether a 2D point lies inside the latest solved visibility polygon.
bool ULSVisionComponent::IsPointVisibleInCurrentVision(const FVector2D& Point2D) const
{
	if (CurrentPolygon.Points.Num() < 3)
	{
		return false;
	}

	bool bInside = false;
	for (int32 CurrentIndex = 0, PreviousIndex = CurrentPolygon.Points.Num() - 1; CurrentIndex < CurrentPolygon.Points.Num(); PreviousIndex = CurrentIndex++)
	{
		const FVector2D& CurrentPoint = CurrentPolygon.Points[CurrentIndex];
		const FVector2D& PreviousPoint = CurrentPolygon.Points[PreviousIndex];

		const bool bCrossesHorizontalRay = ((CurrentPoint.Y > Point2D.Y) != (PreviousPoint.Y > Point2D.Y));
		if (!bCrossesHorizontalRay)
		{
			continue;
		}

		const float Denominator = PreviousPoint.Y - CurrentPoint.Y;
		if (FMath::IsNearlyZero(Denominator))
		{
			continue;
		}

		const float XIntersection = ((PreviousPoint.X - CurrentPoint.X) * (Point2D.Y - CurrentPoint.Y) / Denominator) + CurrentPoint.X;
		if (Point2D.X < XIntersection)
		{
			bInside = !bInside;
		}
	}

	return bInside;
}

// Updates registered targets so locally hidden actors can be culled outside the visible area.
void ULSVisionComponent::UpdateVisionTargets(const FVector2D& VisionOrigin2D)
{
	ULSVisionSubsystem* VisionSubsystem = GetWorld() ? GetWorld()->GetSubsystem<ULSVisionSubsystem>() : nullptr;
	if (VisionSubsystem == nullptr)
	{
		return;
	}

	for (ULSVisionTargetComponent* VisionTarget : VisionSubsystem->GetRegisteredVisionTargets())
	{
		if (VisionTarget == nullptr || VisionTarget->GetOwner() == GetOwner())
		{
			continue;
		}

		bool bVisible = false;
		TArray<FVector> SamplePoints;
		VisionTarget->GatherVisibilitySamplePoints(SamplePoints);

		for (const FVector& SamplePoint : SamplePoints)
		{
			const FVector2D SamplePoint2D(SamplePoint.X, SamplePoint.Y);
			const float Distance = FVector2D::Distance(VisionOrigin2D, SamplePoint2D);

			if (Distance > MaxRayDistance)
			{
				continue;
			}

			if (Distance < VisionRadius || IsPointVisibleInCurrentVision(SamplePoint2D))
			{
				bVisible = true;
				break;
			}
		}

		VisionTarget->SetLocallyVisible(bVisible);
	}
}
