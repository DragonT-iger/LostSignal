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
	Skill1,
	Skill2,
	Skill3,
	Ultimate,
	Interact
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
