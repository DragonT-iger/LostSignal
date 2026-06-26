#include "GAS/Abilities/Character1/LSGA_ShortCircuit.h"

#include "GameFramework/Pawn.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"
#include "Skills/LSPlayerSkillComponent.h"
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

ULSGA_ShortCircuit::ULSGA_ShortCircuit()
{
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Stunned);
	ActivationBlockedTags.AddTag(LSGameplayTags::Combat_SkillCasting); // 스킬끼리만 차단 (기본공격은 통과)
	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_Attacking);      // 공통 "진행 중" 의미 유지
	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_SkillCasting);   // 시전 중 표식
	CancelAbilitiesWithTag.AddTag(LSGameplayTags::Ability_PlayerBasicAttack); // 기본공격 모션 캔슬 후 발동

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void ULSGA_ShortCircuit::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* SourceActor = GetAvatarActorFromActorInfo();
	ULSPlayerSkillComponent* SkillComponent = SourceActor ? SourceActor->FindComponentByClass<ULSPlayerSkillComponent>() : nullptr;
	if (!SourceActor || !SourceActor->HasAuthority() || !SkillComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FLSSkillActivationContext SkillContext;
	if (!SkillComponent->ConsumePendingAbilityContext(GetClass(), SkillContext) || !SkillContext.SkillData)
	{
		UE_LOG(LogLS, Warning, TEXT("%s ShortCircuit ability missing pending skill context."), *GetNameSafe(SourceActor));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ULSShortCircuitSkillDataAsset* ShortCircuitData = Cast<ULSShortCircuitSkillDataAsset>(SkillContext.SkillData);
	const TSubclassOf<ALSShortCircuitProjectile> ResolvedProjectileClass = ShortCircuitData
		? ShortCircuitData->ResolveProjectileClass()
		: nullptr;
	if (!ShortCircuitData || !ResolvedProjectileClass)
	{
		UE_LOG(LogLS, Warning, TEXT("%s ShortCircuit ability requires ULSShortCircuitSkillDataAsset with ProjectileClass."), *GetNameSafe(SourceActor));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	SkillComponent->ApplySkillCooldown(SkillContext.SkillData);

	UWorld* World = SourceActor->GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FLSCharacterSkillRow* Row = SkillContext.bHasSkillRow ? &SkillContext.SkillRow : nullptr;
	if (!Row)
	{
		UE_LOG(LogLS, Warning, TEXT("%s ShortCircuit ability missing active skill row."), *GetNameSafe(SourceActor));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector SourceLocation = SourceActor->GetActorLocation();
	FVector AimDirection = SkillContext.TargetLocation - SourceLocation;
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = FRotator(0.0f, SkillContext.AimYaw, 0.0f).Vector();
	}

	AimDirection = AimDirection.GetSafeNormal2D();
	if (AimDirection.IsNearlyZero())
	{
		UE_LOG(LogLS, Warning, TEXT("[GA_ShortCircuit] Activate rejected: AimDirection is zero. Source=%s Target=%s AimYaw=%.2f"),
			*GetNameSafe(SourceActor),
			*SkillContext.TargetLocation.ToCompactString(),
			SkillContext.AimYaw);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
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
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float ProjectileDuration = ResolveShortCircuitProjectileDuration(Row, ShortCircuitData, SpawnLocation, SkillContext.TargetLocation);
	const float ProjectileArcHeight = ResolveShortCircuitArcHeight(Row, ShortCircuitData);
	const float ProjectileLifeSeconds = ResolveShortCircuitLifeSeconds(ProjectileDuration, ShortCircuitData);
	Projectile->InitializeProjectile(SourceActor, ShortCircuitData, SkillContext.TargetLocation, ProjectileDuration, ProjectileArcHeight, ProjectileLifeSeconds);

	if (ShortCircuitData->bEnableDebugVisualization)
	{
		UE_LOG(LogLS, Log, TEXT("[GA_ShortCircuit] Projectile=%s SpawnLocation=%s Target=%s AimDirection=%s Duration=%.2f Arc=%.2f Life=%.2f"),
			*GetNameSafe(Projectile),
			*SpawnLocation.ToCompactString(),
			*SkillContext.TargetLocation.ToCompactString(),
			*AimDirection.ToCompactString(),
			ProjectileDuration,
			ProjectileArcHeight,
			ProjectileLifeSeconds);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
