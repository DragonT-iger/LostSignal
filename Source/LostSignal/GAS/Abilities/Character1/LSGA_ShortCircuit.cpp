#include "GAS/Abilities/Character1/LSGA_ShortCircuit.h"

#include "Data/LSCharacterSkillRow.h"
#include "GameFramework/Pawn.h"
#include "LostSignal.h"
#include "Skills/LSShortCircuitProjectile.h"
#include "Skills/LSShortCircuitSkillDataAsset.h"
#include "Skills/LSSkillDataAsset.h"

namespace
{
	float ResolveShortCircuitProjectileDuration(const FLSCharacterSkillRow* Row, const ULSShortCircuitSkillDataAsset* SkillData, const FVector& StartLocation, const FVector& TargetLocation)
	{
		if (Row && Row->Skill_Time > 0.0f)
		{
			return Row->Skill_Time;
		}

		const float FallbackSpeed = SkillData ? FMath::Max(SkillData->ProjectileSpeed, 1.0f) : 1200.0f;
		return FMath::Max(FVector::Dist2D(StartLocation, TargetLocation) / FallbackSpeed, 0.05f);
	}

	float ResolveShortCircuitArcHeight(const FLSCharacterSkillRow* Row, const ULSShortCircuitSkillDataAsset* SkillData)
	{
		if (Row && Row->Range_Z > 0.0f)
		{
			return Row->Range_Z;
		}

		return SkillData ? SkillData->ProjectileArcHeight : 0.0f;
	}

	float ResolveShortCircuitLifeSeconds(float ProjectileDuration, const ULSShortCircuitSkillDataAsset* SkillData)
	{
		const float FallbackLifeSeconds = SkillData ? SkillData->ProjectileLifeSeconds : 0.0f;
		return FMath::Max(FallbackLifeSeconds, ProjectileDuration + 0.5f);
	}

	float ResolveShortCircuitSpawnForwardOffset(const FLSCharacterSkillRow* Row, const ULSShortCircuitSkillDataAsset* SkillData)
	{
		if (Row && Row->Move_Distance > 0.0f)
		{
			return Row->Move_Distance;
		}

		return SkillData ? SkillData->ProjectileSpawnForwardOffset : 0.0f;
	}

	float ResolveShortCircuitSpawnZOffset(const FLSCharacterSkillRow* Row, const ULSShortCircuitSkillDataAsset* SkillData)
	{
		if (Row && Row->Move_Duration > 0.0f)
		{
			return Row->Move_Duration;
		}

		return SkillData ? SkillData->ProjectileSpawnZOffset : 0.0f;
	}
}

bool ULSGA_ShortCircuit::PrepareSkillExecution()
{
	const FLSSkillActivationContext& SkillCtx = GetSkillContext();

	CachedShortCircuitData = Cast<ULSShortCircuitSkillDataAsset>(SkillCtx.SkillData);
	CachedProjectileClass = CachedShortCircuitData ? CachedShortCircuitData->ResolveProjectileClass() : nullptr;

	// 데이터에셋/발사체 클래스가 없으면 커밋·쿨타임 없이 발동을 취소한다(기존 즉발 경로와 동일).
	if (!CachedShortCircuitData || !CachedProjectileClass)
	{
		UE_LOG(LogLS, Warning, TEXT("%s ShortCircuit ability requires ULSShortCircuitSkillDataAsset with ProjectileClass."), *GetNameSafe(GetSkillSourceActor()));
		return false;
	}

	return true;
}

void ULSGA_ShortCircuit::ExecuteSkillEffect()
{
	AActor* SourceActor = GetSkillSourceActor();
	const FLSSkillActivationContext& SkillCtx = GetSkillContext();
	UWorld* World = SourceActor ? SourceActor->GetWorld() : nullptr;
	if (!SourceActor || !World || !CachedShortCircuitData || !CachedProjectileClass)
	{
		return;
	}

	ULSShortCircuitSkillDataAsset* ShortCircuitData = CachedShortCircuitData;
	const TSubclassOf<ALSShortCircuitProjectile> ResolvedProjectileClass = CachedProjectileClass;

	const FLSCharacterSkillRow* Row = SkillCtx.bHasSkillRow ? &SkillCtx.SkillRow : nullptr;
	if (!Row)
	{
		UE_LOG(LogLS, Warning, TEXT("%s ShortCircuit ability missing active skill row."), *GetNameSafe(SourceActor));
		return;
	}

	const FVector SourceLocation = SourceActor->GetActorLocation();
	FVector AimDirection = SkillCtx.TargetLocation - SourceLocation;
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = FRotator(0.0f, SkillCtx.AimYaw, 0.0f).Vector();
	}

	AimDirection = AimDirection.GetSafeNormal2D();
	if (AimDirection.IsNearlyZero())
	{
		UE_LOG(LogLS, Warning, TEXT("[GA_ShortCircuit] Activate rejected: AimDirection is zero. Source=%s Target=%s AimYaw=%.2f"),
			*GetNameSafe(SourceActor),
			*SkillCtx.TargetLocation.ToCompactString(),
			SkillCtx.AimYaw);
		return;
	}

	const float SpawnForwardOffset = ResolveShortCircuitSpawnForwardOffset(Row, ShortCircuitData);
	const float SpawnZOffset = ResolveShortCircuitSpawnZOffset(Row, ShortCircuitData);
	const FVector SpawnLocation =
		SourceLocation +
		(AimDirection * SpawnForwardOffset) +
		FVector(0.0f, 0.0f, SpawnZOffset);

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
		UE_LOG(LogLS, Warning, TEXT("[GA_ShortCircuit] Failed to spawn projectile. Source=%s ProjectileClass=%s"),
			*GetNameSafe(SourceActor),
			*GetNameSafe(ResolvedProjectileClass.Get()));
		return;
	}

	const float ProjectileDuration = ResolveShortCircuitProjectileDuration(Row, ShortCircuitData, SpawnLocation, SkillCtx.TargetLocation);
	const float ProjectileArcHeight = ResolveShortCircuitArcHeight(Row, ShortCircuitData);
	const float ProjectileLifeSeconds = ResolveShortCircuitLifeSeconds(ProjectileDuration, ShortCircuitData);
	Projectile->InitializeProjectile(SourceActor, ShortCircuitData, SkillCtx.TargetLocation, ProjectileDuration, ProjectileArcHeight, ProjectileLifeSeconds);

	if (ShortCircuitData->bEnableDebugVisualization)
	{
		UE_LOG(LogLS, Log, TEXT("[GA_ShortCircuit] Projectile=%s SpawnLocation=%s Target=%s AimDirection=%s Duration=%.2f Arc=%.2f Life=%.2f"),
			*GetNameSafe(Projectile),
			*SpawnLocation.ToCompactString(),
			*SkillCtx.TargetLocation.ToCompactString(),
			*AimDirection.ToCompactString(),
			ProjectileDuration,
			ProjectileArcHeight,
			ProjectileLifeSeconds);
	}
}
