#pragma once

#include "CoreMinimal.h"
#include "LSCombatTypes.generated.h"

UENUM(BlueprintType)
enum class ELSCombatActionState : uint8
{
	Idle,
	BasicAttack,
	Skill,
	Dash,
	HitReaction,
	Stunned,
	Dead
};

UENUM(BlueprintType)
enum class ELSCombatActionPhase : uint8
{
	None,
	Startup,
	Active,
	Recovery
};

UENUM(BlueprintType)
enum class ELSCombatCommandType : uint8
{
	BasicAttack,
	Dash,
	Interact
};

UENUM(BlueprintType)
enum class ELSTenacityTier : uint8
{
	None = 0 UMETA(Hidden),
	Normal = 1,
	SuperArmor = 4,
	Invincible = 6
};

UENUM(BlueprintType)
enum class ELSBreakPowerTier : uint8
{
	None = 0 UMETA(Hidden),
	NormalAttack = 2,
	SpecialAttack = 3,
	HardCrowdControl = 5
};

USTRUCT(BlueprintType)
struct FLSBufferedCombatCommand
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="LS/Combat")
	ELSCombatCommandType CommandType = ELSCombatCommandType::BasicAttack;

	UPROPERTY(BlueprintReadOnly, Category="LS/Combat")
	float ExpireTime = 0.0f;

	bool IsValid(float CurrentTime) const
	{
		return ExpireTime > CurrentTime;
	}
};

USTRUCT(BlueprintType)
struct FLSImpactResolution
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="LS/Combat")
	float TargetTenacity = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category="LS/Combat")
	ELSBreakPowerTier IncomingBreakPower = ELSBreakPowerTier::NormalAttack;

	UPROPERTY(BlueprintReadOnly, Category="LS/Combat")
	bool bDamageBlocked = false;

	UPROPERTY(BlueprintReadOnly, Category="LS/Combat")
	bool bCrowdControlBlocked = false;

	UPROPERTY(BlueprintReadOnly, Category="LS/Combat")
	bool bImpactAllowed = true;
};
