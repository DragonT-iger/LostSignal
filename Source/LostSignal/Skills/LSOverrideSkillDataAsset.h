#pragma once

#include "CoreMinimal.h"
#include "Data/LSCharacterSkillRow.h"
#include "Skills/LSSkillDataAsset.h"
#include "LSOverrideSkillDataAsset.generated.h"

class UGameplayEffect;

/** Override-specific skill data. Kernel/Surge variants use this for debug fallbacks while DataTable values remain authoritative. */
UCLASS(BlueprintType)
class LOSTSIGNAL_API ULSOverrideSkillDataAsset : public ULSSkillDataAsset
{
	GENERATED_BODY()

public:
	ULSOverrideSkillDataAsset();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override")
	ELSCharacterSkillRangeShape FallbackRangeShape = ELSCharacterSkillRangeShape::Circle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override", meta=(ClampMin="0.0"))
	float FallbackRadius = 450.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override", meta=(ClampMin="0.0"))
	float FallbackLength = 650.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override", meta=(ClampMin="0.0"))
	float FallbackWidth = 240.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override", meta=(ClampMin="0.0", ClampMax="360.0"))
	float FallbackConeAngleDegrees = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override", meta=(ClampMin="0.0"))
	float FallbackAttackCoefficient = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override", meta=(ClampMin="0.0"))
	float FallbackKnockbackSpeed = 650.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override")
	float FallbackKnockbackUpSpeed = 80.0f;

	// 넉백 지속시간 단일 출처. (DataTable Skill_Time은 시전시간 전용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override", meta=(ClampMin="0.0"))
	float FallbackKnockbackDuration = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override|Kernel")
	bool bApplyAttackSpeedBuff = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override|Kernel")
	TSubclassOf<UGameplayEffect> AttackSpeedBuffEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override|Kernel", meta=(ClampMin="0.0"))
	float FallbackAttackSpeedBonus = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override|Kernel", meta=(ClampMin="0.0"))
	float FallbackAttackSpeedDuration = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override|Debug")
	bool bEnableDebugLog = false;
};
