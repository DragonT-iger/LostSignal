// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/LSCharacterBase.h"
#include "LSEnemyCharacter.generated.h"

class UDataTable;
class UGameplayAbility;
class UAnimMontage;
class ULSCharacterAttributeSet;
class ULSHpDebugWidget;
class ULSMinimapMarkerComponent;
class ULSMonsterCombatComponent;
class ULSMonsterSenseComponent;
class ULSNoiseEmitterComponent;
class ULSEnemyHealthBarComponent;
class ULSSkillPreviewComponent;
struct FLSMonsterArchetypeRow;

/**
 * Base enemy pawn that wires monster AI components, animation data, and grants the data-driven monster action ability.
 * Not marked Abstract: StateTree(ST_*)의 Context Actor Class로 이 타입을 골라 evaluator의 EnemyCharacter 바인딩에 쓰기 위함.
 * 직접 스폰은 의도하지 않음(자식 ALSEnemy* 사용). MonsterRowName 미설정 시 BeginPlay에서 경고 로그.
 */
UCLASS()
class LOSTSIGNAL_API ALSEnemyCharacter : public ALSCharacterBase
{
	GENERATED_BODY()

public:
	ALSEnemyCharacter();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category="LS/AI")
	ULSMonsterSenseComponent* GetMonsterSenseComponent() const { return MonsterSenseComponent; }

	UFUNCTION(BlueprintPure, Category="LS/AI")
	ULSMonsterCombatComponent* GetMonsterCombatComponent() const { return MonsterCombatComponent; }

	UFUNCTION(BlueprintPure, Category="LS/Minimap")
	ULSMinimapMarkerComponent* GetMinimapMarkerComponent() const { return MinimapMarkerComponent; }

	UFUNCTION(BlueprintPure, Category="LS/Noise")
	ULSNoiseEmitterComponent* GetNoiseEmitterComponent() const { return NoiseEmitterComponent; }

	UFUNCTION(BlueprintPure, Category="LS/UI|Combat")
	ULSEnemyHealthBarComponent* GetHealthBarComponent() const { return HealthBarComponent; }

	UFUNCTION(BlueprintPure, Category="LS/GAS")
	ULSCharacterAttributeSet* GetMonsterAttributeSet() const { return MonsterAttributeSet; }

	/** Returns the death montage authored for this enemy. */
	UFUNCTION(BlueprintPure, Category="LS/Combat")
	UAnimMontage* GetDeathMontage() const { return DeathMontage; }

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayAbilityMontage(UAnimMontage* Montage);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopAbilityMontage(UAnimMontage* Montage, float BlendOutTime);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Data-driven monster attack ability granted on BeginPlay; activated via ULSMonsterCombatComponent::RequestAction. */
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TSubclassOf<UGameplayAbility> MonsterActionAbilityClass;

	/** Death animation authored per enemy BP and played by the Dead state before AI logic stops. */
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TObjectPtr<UAnimMontage> DeathMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="LS/AI|DataTable")
	TObjectPtr<UDataTable> MonsterArchetypeTable;

	UPROPERTY(EditDefaultsOnly, Category="LS/AI|DataTable")
	FName MonsterRowName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/AI")
	TObjectPtr<ULSMonsterSenseComponent> MonsterSenseComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/AI")
	TObjectPtr<ULSMonsterCombatComponent> MonsterCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Minimap")
	TObjectPtr<ULSMinimapMarkerComponent> MinimapMarkerComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Noise")
	TObjectPtr<ULSNoiseEmitterComponent> NoiseEmitterComponent;

	// 공격 범위 텔레그래프(스킬 인디케이터 재사용). ULSMonsterCombatComponent가 구동한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Combat")
	TObjectPtr<ULSSkillPreviewComponent> SkillPreviewComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<ULSEnemyHealthBarComponent> HealthBarComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/GAS")
	TObjectPtr<ULSCharacterAttributeSet> MonsterAttributeSet;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Debug")
	bool bCreateDebugHpWidget = true;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Debug")
	TSubclassOf<ULSHpDebugWidget> DebugHpWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Debug")
	FVector2D DebugHpWidgetBasePosition = FVector2D(40.0f, 120.0f);

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Debug", meta=(ClampMin="0.0"))
	float DebugHpWidgetVerticalSpacing = 60.0f;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/UI|Debug")
	TObjectPtr<ULSHpDebugWidget> DebugHpWidgetInstance;

private:
	const FLSMonsterArchetypeRow* FindMonsterArchetypeRow() const;
	void InitializeMonsterArchetype();
	void ApplyMonsterAttributes(const FLSMonsterArchetypeRow& Row);
	void TryCreateDebugHpWidget();
	void DestroyDebugHpWidget();

	bool bWarnedMissingDebugHpWidgetClass = false;
};
