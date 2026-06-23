#pragma once

#include "Characters/LSPlayerCharacter.h"
#include "CoreMinimal.h"
#include "LSTestPlayerCharacter.generated.h"

class UDataTable;
struct FLSCharacterStatRow;

UCLASS()
class LOSTSIGNAL_API ALSTestPlayerCharacter : public ALSPlayerCharacter
{
	GENERATED_BODY()

public:
	ALSTestPlayerCharacter();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void InitializeBaseAttributes() override;

	UPROPERTY(EditDefaultsOnly, Category="LS/Stats|DataTable")
	TObjectPtr<UDataTable> CharacterStatTable;

	UPROPERTY(EditAnywhere, Category="LS/Stats|DataTable")
	FName CharacterRowName;

	UPROPERTY(EditAnywhere, Category="LS/Stats|Default Values")
	int32 BaseAttack = 100;

	UPROPERTY(EditAnywhere, Category="LS/Stats|Default Values")
	float BaseAttackSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category="LS/Stats|Default Values")
	float BaseCooldownReduction = 0.0f;

	UPROPERTY(EditAnywhere, Category="LS/Stats|Default Values")
	float BaseCritChance = 0.3f;

	UPROPERTY(EditAnywhere, Category="LS/Stats|Default Values")
	float BaseCritDamage = 1.5f;

	UPROPERTY(EditAnywhere, Category="LS/Stats|Default Values")
	float BaseArmorPenetration = 0.0f;

	UPROPERTY(EditAnywhere, Category="LS/Stats|Default Values")
	int32 BaseHealth = 100;

	UPROPERTY(EditAnywhere, Category="LS/Stats|Default Values")
	int32 BaseDefence = 10;

	UPROPERTY(EditAnywhere, Category="LS/Stats|Default Values")
	int32 BaseRecovery = 5;

	UPROPERTY(EditAnywhere, Category="LS/Stats|Default Values")
	int32 BaseStamina = 100;

	UPROPERTY(EditAnywhere, Category="LS/Stats|Default Values")
	float BaseMoveSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category="LS/Stats|Default Values")
	float BaseDashSpeed = 1200.0f;

	UPROPERTY(EditAnywhere, Category="LS/Stats|Default Values")
	float BaseDashDuration = 0.3f;

	UPROPERTY(EditAnywhere, Category="LS/Stats|Default Values")
	float BaseDashCooldown = 1.0f;

private:
	const FLSCharacterStatRow* FindStatRow() const;
	void LoadStatsFromDataTable();
	void ApplyStatsToAttributeSet();
};
