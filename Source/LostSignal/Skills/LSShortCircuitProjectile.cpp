#include "Skills/LSShortCircuitProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "LostSignal.h"
#include "Net/UnrealNetwork.h"
#include "Skills/LSShortCircuitField.h"
#include "Skills/LSShortCircuitSkillDataAsset.h"
#include "UObject/ConstructorHelpers.h"

ALSShortCircuitProjectile::ALSShortCircuitProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(12.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	DebugProjectileMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugProjectileMesh"));
	DebugProjectileMeshComponent->SetupAttachment(CollisionComponent);
	DebugProjectileMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugProjectileMeshComponent->SetGenerateOverlapEvents(false);
	DebugProjectileMeshComponent->SetCastShadow(false);
	DebugProjectileMeshComponent->SetHiddenInGame(true);
	DebugProjectileMeshComponent->SetVisibility(false, true);
	DebugProjectileMeshComponent->SetRelativeScale3D(FVector(0.35f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		DebugProjectileMeshComponent->SetStaticMesh(SphereMeshFinder.Object);
	}
}

void ALSShortCircuitProjectile::BeginPlay()
{
	Super::BeginPlay();

	OnRep_DebugProjectileMesh();

	if (HasAuthority() && SkillData && SkillData->bEnableDebugVisualization)
	{
		UE_LOG(LogLS, Log, TEXT("[ShortCircuit] Projectile BeginPlay: Projectile=%s CollisionEnabled=%d ObjectType=%d"),
			*GetNameSafe(this),
			CollisionComponent ? static_cast<int32>(CollisionComponent->GetCollisionEnabled()) : -1,
			CollisionComponent ? static_cast<int32>(CollisionComponent->GetCollisionObjectType()) : -1);
	}
}

void ALSShortCircuitProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}

	if (MovementDurationSeconds <= KINDA_SMALL_NUMBER)
	{
		FinishProjectile();
		return;
	}

	MovementElapsedSeconds += DeltaSeconds;
	const float Alpha = FMath::Clamp(MovementElapsedSeconds / MovementDurationSeconds, 0.0f, 1.0f);
	const FVector LinearLocation = FMath::Lerp(MovementStartLocation, MovementVisualTargetLocation, Alpha);
	const float ArcOffset = RuntimeProjectileArcHeight * FMath::Sin(UE_PI * Alpha);
	SetActorLocation(LinearLocation + FVector(0.0f, 0.0f, ArcOffset));

	if (Alpha >= 1.0f)
	{
		FinishProjectile();
	}
}

void ALSShortCircuitProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALSShortCircuitProjectile, bShowDebugProjectileMesh);
}

void ALSShortCircuitProjectile::InitializeProjectile(
	AActor* InSourceActor,
	ULSShortCircuitSkillDataAsset* InSkillData,
	const FVector& TargetLocation,
	float ProjectileDuration,
	float ProjectileArcHeight,
	float ProjectileLifeSeconds)
{
	SourceActor = InSourceActor;
	SkillData = InSkillData;
	ImpactTargetLocation = TargetLocation;
	RuntimeProjectileArcHeight = FMath::Max(0.0f, ProjectileArcHeight);

	if (CollisionComponent && SourceActor)
	{
		CollisionComponent->IgnoreActorWhenMoving(SourceActor, true);
	}

	MovementStartLocation = GetActorLocation();
	MovementVisualTargetLocation = TargetLocation;
	if (CollisionComponent)
	{
		MovementVisualTargetLocation.Z += CollisionComponent->GetScaledSphereRadius();
	}

	MovementDurationSeconds = FMath::Max(ProjectileDuration, 0.05f);
	MovementElapsedSeconds = 0.0f;
	SetLifeSpan(FMath::Max(ProjectileLifeSeconds, MovementDurationSeconds + 0.5f));
	SetActorTickEnabled(HasAuthority());

	if (HasAuthority() && SkillData && SkillData->bEnableDebugVisualization)
	{
		bShowDebugProjectileMesh = true;
		SetDebugProjectileMeshVisible(true);

		UE_LOG(LogLS, Log, TEXT("[ShortCircuit] Projectile initialized: Projectile=%s Source=%s Skill=%s Start=%s Target=%s VisualTarget=%s Speed=%.2f ArcHeight=%.2f Duration=%.2f Life=%.2f"),
			*GetNameSafe(this),
			*GetNameSafe(SourceActor),
			*GetNameSafe(SkillData),
			*MovementStartLocation.ToCompactString(),
			*TargetLocation.ToCompactString(),
			*MovementVisualTargetLocation.ToCompactString(),
			FVector::Dist2D(MovementStartLocation, TargetLocation) / MovementDurationSeconds,
			RuntimeProjectileArcHeight,
			MovementDurationSeconds,
			GetLifeSpan());
	}
}

void ALSShortCircuitProjectile::OnRep_DebugProjectileMesh()
{
	SetDebugProjectileMeshVisible(bShowDebugProjectileMesh);
}

void ALSShortCircuitProjectile::FinishProjectile()
{
	if (!HasAuthority() || !SkillData)
	{
		return;
	}

	if (SkillData && SkillData->bEnableDebugVisualization)
	{
		UE_LOG(LogLS, Log, TEXT("[ShortCircuit] Projectile finished: Projectile=%s Target=%s ActorLocation=%s XYError=%.2f"),
			*GetNameSafe(this),
			*ImpactTargetLocation.ToCompactString(),
			*GetActorLocation().ToCompactString(),
			FVector::Dist2D(GetActorLocation(), ImpactTargetLocation));
		MulticastDrawDebugImpact(
			ImpactTargetLocation,
			SkillData->DebugProjectileColor,
			SkillData->DebugDrawDuration);
	}

	SpawnFieldAtLocation(ImpactTargetLocation);
	Destroy();
}

void ALSShortCircuitProjectile::SpawnFieldAtLocation(const FVector& FieldLocation)
{
	const TSubclassOf<ALSShortCircuitField> ResolvedFieldClass = SkillData ? SkillData->ResolveFieldClass() : nullptr;
	if (!SkillData || !ResolvedFieldClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[ShortCircuit] Field spawn skipped: SkillData or FieldClass missing. Projectile=%s Skill=%s FieldClass=%s"),
			*GetNameSafe(this),
			*GetNameSafe(SkillData),
			*GetNameSafe(ResolvedFieldClass.Get()));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogLS, Warning, TEXT("[ShortCircuit] Field spawn skipped: World is missing. Projectile=%s"), *GetNameSafe(this));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SourceActor;
	SpawnParams.Instigator = SourceActor ? Cast<APawn>(SourceActor) : nullptr;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ALSShortCircuitField* Field = World->SpawnActor<ALSShortCircuitField>(
		ResolvedFieldClass,
		FieldLocation,
		FRotator::ZeroRotator,
		SpawnParams);

	if (Field)
	{
		if (SkillData->bEnableDebugVisualization)
		{
			UE_LOG(LogLS, Log, TEXT("[ShortCircuit] Field spawned: Field=%s Location=%s RadiusPreview=%.2f Duration=%.2f Interval=%.2f"),
				*GetNameSafe(Field),
				*FieldLocation.ToCompactString(),
				SkillData->BuildPreviewSpec().Radius,
				SkillData->FieldDuration,
				SkillData->FieldPulseInterval);
		}

		Field->InitializeField(SourceActor, SkillData);
		return;
	}

	UE_LOG(LogLS, Warning, TEXT("[ShortCircuit] Field spawn failed: Projectile=%s FieldClass=%s Location=%s"),
		*GetNameSafe(this),
		*GetNameSafe(ResolvedFieldClass.Get()),
		*FieldLocation.ToCompactString());
}

void ALSShortCircuitProjectile::SetDebugProjectileMeshVisible(bool bVisible)
{
	if (!DebugProjectileMeshComponent)
	{
		return;
	}

	DebugProjectileMeshComponent->SetHiddenInGame(!bVisible);
	DebugProjectileMeshComponent->SetVisibility(bVisible, true);
}

void ALSShortCircuitProjectile::MulticastDrawDebugImpact_Implementation(
	FVector_NetQuantize ImpactLocation,
	FColor Color,
	float Duration)
{
	DrawDebugImpact(ImpactLocation, Color, Duration);
}

void ALSShortCircuitProjectile::DrawDebugImpact(const FVector& ImpactLocation, const FColor& Color, float Duration) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	DrawDebugSphere(World, ImpactLocation, 28.0f, 16, Color, false, Duration, 0, 3.0f);
	DrawDebugPoint(World, ImpactLocation, 16.0f, Color, false, Duration, 0);
}
