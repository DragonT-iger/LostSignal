#pragma once

#include "CoreMinimal.h"
#include "Combat/LSCombatTypes.h"
#include "Engine/DataTable.h"
#include "UObject/Object.h"
#include "Skills/LSSkillAreaTypes.h"
#include "Skills/LSSkillTypes.h"
#include "LSSkill.generated.h"

class UGameplayEffect;
class UGameplayAbility;
struct FLSCharacterSkillRow;

UCLASS(Abstract, Blueprintable, BlueprintType)
class LOSTSIGNAL_API ULSSkill : public UObject
{
	GENERATED_BODY()

public:
	ULSSkill();

	UFUNCTION(BlueprintNativeEvent, Category="LS/Skill")
	bool ActivateSkill(const FLSSkillActivationContext& Context);
	virtual bool ActivateSkill_Implementation(const FLSSkillActivationContext& Context);

	UFUNCTION(BlueprintNativeEvent, Category="LS/Skill")
	bool HandleBasicAttackHit(const FLSBasicAttackHitContext& Context) const;
	virtual bool HandleBasicAttackHit_Implementation(const FLSBasicAttackHitContext& Context) const;

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	FLSSkillAreaPreviewSpec BuildPreviewSpec() const;

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	bool TryGetSkillRow(FLSCharacterSkillRow& OutRow) const;

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	TSubclassOf<UGameplayAbility> GetDefaultAbilityClass() const { return DefaultAbilityClass; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|DataTable")
	FDataTableRowHandle SkillRow;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TSubclassOf<UGameplayAbility> DefaultAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview")
	FLSSkillAreaPreviewSpec PreviewSpec;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Damage")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Damage", meta=(ClampMin="0.0"))
	float FixedDamage = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Damage", meta=(ClampMin="0.0"))
	float AttackCoefficient = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Damage")
	bool bCanCrit = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Damage")
	ELSBreakPowerTier BreakPower = ELSBreakPowerTier::NormalAttack;

protected:
	const FLSCharacterSkillRow* ResolveSkillRow() const;
};
