#include "GAS/Abilities/LSGA_ShortCircuit.h"

#include "GameFramework/Pawn.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "Skills/LSShortCircuitProjectile.h"
#include "Skills/LSShortCircuitSkillDataAsset.h"
#include "Skills/LSSkillDataAsset.h"

ULSGA_ShortCircuit::ULSGA_ShortCircuit()
{
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(LSGameplayTags::Combat_Attacking);
	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_Attacking);

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

	const FVector SpawnLocation =
		SourceLocation +
		(AimDirection * ShortCircuitData->ProjectileSpawnForwardOffset) +
		FVector(0.0f, 0.0f, ShortCircuitData->ProjectileSpawnZOffset);

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

	Projectile->InitializeProjectile(SourceActor, ShortCircuitData, SkillContext.TargetLocation);

	if (ShortCircuitData->bEnableDebugVisualization)
	{
		UE_LOG(LogLS, Log, TEXT("[GA_ShortCircuit] Projectile=%s SpawnLocation=%s Target=%s AimDirection=%s"),
			*GetNameSafe(Projectile),
			*SpawnLocation.ToCompactString(),
			*SkillContext.TargetLocation.ToCompactString(),
			*AimDirection.ToCompactString());
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
