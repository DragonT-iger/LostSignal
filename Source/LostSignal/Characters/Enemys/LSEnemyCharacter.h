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
class ULSVisionTargetComponent;
class ULSVisionGhostComponent;
class ULSEnemyHealthBarComponent;
class ULSSkillPreviewComponent;
struct FLSMonsterArchetypeRow;

/** 애니 정지(bPauseAnims) 사유 비트. 여러 사유가 겹칠 때 마지막 사유 해제에서만 재생이 복원되게 한다. */
enum class ELSEnemyAnimPauseReason : uint8
{
	Dormant = 1 << 0,
	Knockback = 1 << 1,
};

/**
 * Base enemy pawn that wires monster AI components, animation data, and grants the data-driven monster action ability.
 * Not marked Abstract: StateTree(ST_*)의 Context Actor Class로 이 타입을 골라 evaluator의 EnemyCharacter 바인딩에 쓰기 위함.
 * 직접 스폰은 의도하지 않음(자식 몬스터 클래스 사용). MonsterRowName 미설정 시 BeginPlay에서 경고 로그.
 */
UCLASS()
class LOSTSIGNAL_API ALSEnemyCharacter : public ALSCharacterBase
{
	GENERATED_BODY()

public:
	ALSEnemyCharacter();

	virtual void BeginPlay() override;

	/** 멀티플라이어가 적용되지 않은 기준(base) MaxWalkSpeed. BeginPlay에서 1회 저장. 이동 Task/AnimInstance가 base × 배수의 단일 출처로 사용. */
	float GetDefaultMaxWalkSpeed() const { return DefaultMaxWalkSpeed; }

	UFUNCTION(BlueprintPure, Category="LS/AI")
	ULSMonsterSenseComponent* GetMonsterSenseComponent() const { return MonsterSenseComponent; }

	UFUNCTION(BlueprintPure, Category="LS/AI")
	ULSMonsterCombatComponent* GetMonsterCombatComponent() const { return MonsterCombatComponent; }

	UFUNCTION(BlueprintPure, Category="LS/Minimap")
	ULSMinimapMarkerComponent* GetMinimapMarkerComponent() const { return MinimapMarkerComponent; }

	UFUNCTION(BlueprintPure, Category="LS/Noise")
	ULSNoiseEmitterComponent* GetNoiseEmitterComponent() const { return NoiseEmitterComponent; }

	UFUNCTION(BlueprintPure, Category="LS/Vision")
	ULSVisionTargetComponent* GetVisionTargetComponent() const { return VisionTargetComponent; }

	UFUNCTION(BlueprintPure, Category="LS/Vision")
	ULSVisionGhostComponent* GetVisionGhostComponent() const { return VisionGhostComponent; }

	UFUNCTION(BlueprintPure, Category="LS/UI|Combat")
	ULSEnemyHealthBarComponent* GetHealthBarComponent() const { return HealthBarComponent; }

	UFUNCTION(BlueprintPure, Category="LS/GAS")
	ULSCharacterAttributeSet* GetMonsterAttributeSet() const { return MonsterAttributeSet; }

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayAbilityMontage(UAnimMontage* Montage);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopAbilityMontage(UAnimMontage* Montage, float BlendOutTime);

	/** 애니 정지 사유 비트를 갱신하고, 사유가 하나라도 남아 있으면 메시 애니를 정지한다(휴면 태스크·넉백 프리즈 공용). */
	void SetAnimPauseReason(ELSEnemyAnimPauseReason Reason, bool bActive);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 사망 진입 시 캡슐·메시 콜리전을 해제해 시체가 이동을 막거나 추가 타격 대상이 되지 않게 한다. */
	virtual void OnDeathStateChanged(bool bIsDead) override;

	/** 넉백 시작 시 전용 몽타주 재생(할당 시) 또는 애니 정지(미할당 시), 종료 시 복원. */
	virtual void OnKnockbackStateChanged(bool bKnockbackActive) override;

	/** 넉백 프리즈를 전 클라에 적용/해제(몽타주 미할당 몬스터 전용). */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetKnockbackAnimFrozen(bool bFrozen);

	// 넉백 동안 재생할 전용 몽타주(BP에서 매핑). 루트모션 없는 in-place 권장 — 이동은 넉백 루트모션 소스가 담당. 미할당이면 넉백 동안 애니를 정지한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TObjectPtr<UAnimMontage> KnockbackMontage;

	// 넉백 종료 시 넉백 몽타주 블렌드아웃 시간(초). AI가 즉시 이동/공격을 재개하므로 끝까지 틀지 않는다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float KnockbackMontageBlendOutTime = 0.15f;

	/** Data-driven monster attack ability granted on BeginPlay; activated via ULSMonsterCombatComponent::RequestAction. */
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TSubclassOf<UGameplayAbility> MonsterActionAbilityClass;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Vision")
	TObjectPtr<ULSVisionTargetComponent> VisionTargetComponent;

	// 시야 이탈 시 마지막 위치·포즈의 실루엣 잔상을 남기는 로컬 전용 연출.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Vision")
	TObjectPtr<ULSVisionGhostComponent> VisionGhostComponent;

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

	// 현재 활성인 애니 정지 사유 비트(ELSEnemyAnimPauseReason 조합). 0이면 재생.
	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Combat")
	uint8 AnimPauseReasonBits = 0;

	// 멀티 미적용 기준 MaxWalkSpeed(생성자/BP 기본값). BeginPlay에서 캡처해 고정.
	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/AI")
	float DefaultMaxWalkSpeed = 0.0f;
};
