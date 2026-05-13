#include "Skills/LSShortCircuitSkill.h"

#include "GameFramework/Pawn.h"
#include "LostSignal.h"
#include "Skills/LSShortCircuitField.h"
#include "Skills/LSShortCircuitProjectile.h"

ULSShortCircuitSkill::ULSShortCircuitSkill()
{
	ProjectileClass = ALSShortCircuitProjectile::StaticClass();
	FieldClass = ALSShortCircuitField::StaticClass();
	AttackCoefficient = 1.5f;
}

bool ULSShortCircuitSkill::ActivateSkill_Implementation(const FLSSkillActivationContext& Context)
{
	AActor* SourceActor = Context.SourceActor.Get();
	const TSubclassOf<ALSShortCircuitProjectile> ResolvedProjectileClass = ResolveProjectileClass();
	if (!SourceActor)
	{
		UE_LOG(LogLS, Warning, TEXT("[ShortCircuit] Activate rejected: SourceActor is missing. Skill=%s"), *GetNameSafe(this));
		return false;
	}

	if (!SourceActor->HasAuthority())
	{
		UE_LOG(LogLS, Warning, TEXT("[ShortCircuit] Activate rejected: SourceActor has no authority. Source=%s Skill=%s"),
			*GetNameSafe(SourceActor),
			*GetNameSafe(this));
		return false;
	}

	if (!ResolvedProjectileClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[ShortCircuit] Activate rejected: ProjectileClass is missing. Source=%s Skill=%s"),
			*GetNameSafe(SourceActor),
			*GetNameSafe(this));
		return false;
	}

	UWorld* World = SourceActor->GetWorld();
	if (!World)
	{
		UE_LOG(LogLS, Warning, TEXT("[ShortCircuit] Activate rejected: World is missing. Source=%s Skill=%s"),
			*GetNameSafe(SourceActor),
			*GetNameSafe(this));
		return false;
	}

	if (bEnableDebugVisualization)
	{
		UE_LOG(LogLS, Log, TEXT("[ShortCircuit] Activate begin: Source=%s Skill=%s ProjectileClass=%s FieldClass=%s Target=%s AimYaw=%.2f"),
			*GetNameSafe(SourceActor),
			*GetNameSafe(this),
			*GetNameSafe(ResolvedProjectileClass.Get()),
			*GetNameSafe(ResolveFieldClass().Get()),
			*Context.TargetLocation.ToCompactString(),
			Context.AimYaw);
	}

	const FVector SourceLocation = SourceActor->GetActorLocation();
	FVector AimDirection = Context.TargetLocation - SourceLocation;
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = FRotator(0.0f, Context.AimYaw, 0.0f).Vector();
	}

	AimDirection = AimDirection.GetSafeNormal2D();
	if (AimDirection.IsNearlyZero())
	{
		UE_LOG(LogLS, Warning, TEXT("[ShortCircuit] Activate rejected: AimDirection is zero. Source=%s SourceLocation=%s Target=%s AimYaw=%.2f"),
			*GetNameSafe(SourceActor),
			*SourceLocation.ToCompactString(),
			*Context.TargetLocation.ToCompactString(),
			Context.AimYaw);
		return false;
	}

	const FVector SpawnLocation =
		SourceLocation +
		(AimDirection * ProjectileSpawnForwardOffset) +
		FVector(0.0f, 0.0f, ProjectileSpawnZOffset);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = SourceActor;
	SpawnParams.Instigator = Cast<APawn>(SourceActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ALSShortCircuitProjectile* Projectile = World->SpawnActor<ALSShortCircuitProjectile>(
		ResolvedProjectileClass,
		SpawnLocation,
		AimDirection.Rotation(),
		SpawnParams);

	if (!Projectile)
	{
		UE_LOG(LogLS, Warning, TEXT("[ShortCircuit] Activate rejected: failed to spawn projectile. Source=%s ProjectileClass=%s SpawnLocation=%s AimDirection=%s"),
			*GetNameSafe(SourceActor),
			*GetNameSafe(ResolvedProjectileClass.Get()),
			*SpawnLocation.ToCompactString(),
			*AimDirection.ToCompactString());
		return false;
	}

	Projectile->InitializeProjectile(SourceActor, this, Context.TargetLocation);

	if (bEnableDebugVisualization)
	{
		UE_LOG(LogLS, Log, TEXT("[ShortCircuit] Activate success: Projectile=%s SpawnLocation=%s Target=%s AimDirection=%s Speed=%.2f ArcHeight=%.2f Life=%.2f"),
			*GetNameSafe(Projectile),
			*SpawnLocation.ToCompactString(),
			*Context.TargetLocation.ToCompactString(),
			*AimDirection.ToCompactString(),
			ProjectileSpeed,
			ProjectileArcHeight,
			ProjectileLifeSeconds);
	}

	return true;
}

TSubclassOf<ALSShortCircuitProjectile> ULSShortCircuitSkill::ResolveProjectileClass() const
{
	if (ProjectileClass)
	{
		return ProjectileClass;
	}

	return ALSShortCircuitProjectile::StaticClass();
}

TSubclassOf<ALSShortCircuitField> ULSShortCircuitSkill::ResolveFieldClass() const
{
	if (FieldClass)
	{
		return FieldClass;
	}

	return ALSShortCircuitField::StaticClass();
}
