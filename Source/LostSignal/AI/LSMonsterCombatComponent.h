#pragma once

#include "Combat/LSCombatTypes.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LSMonsterCombatComponent.generated.h"

class UDataTable;
class UGameplayEffect;
class UMaterialInterface;
class ULSSkillPreviewComponent;
struct FLSMonsterArchetypeRow;
struct FLSMonsterActionRow;

/**
 * Thin bridge from monster AI to GAS ability activation and data-driven action hits.
 * StateTree asks for an attack (RequestAction); the data-driven ULSGA_MonsterAction plays the
 * authored montage, whose frame notifies drive the telegraph (BeginActionTelegraph/EndActionTelegraph)
 * and the hit (PerformActionHit). All attack numbers come from DT_MonsterAction (FLSMonsterActionRow).
 */
UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSMonsterCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSMonsterCombatComponent();

	void ApplyArchetype(const FLSMonsterArchetypeRow& Row);

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	float GetLeashDistance() const { return LeashDistance; }

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	float GetAlertMoveSpeedMultiplier() const { return AlertMoveSpeedMultiplier; }

	// 일반 어빌리티 태그 활성화/취소/조회(액션 어빌리티 활성화에도 사용).
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	bool RequestAbilityByTag(FGameplayTag AbilityTag) const;

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void CancelAbilityByTag(FGameplayTag AbilityTag) const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool IsAbilityActiveByTag(FGameplayTag AbilityTag) const;

	/** 현재 거리에 맞는 액션을 골라 ULSGA_MonsterAction을 활성화한다. StateTree 공격 태스크가 호출. */
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	bool RequestAction(AActor* Target);

	/** 현재 거리에 발동 가능한(사거리 적합 + 쿨다운 준비) 액션이 있는지. StateTree 진입 판정용. */
	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool HasUsableActionInRange(float Distance) const;

	/** 타격 프레임 AnimNotify가 호출. 활성 액션 row의 히트박스로 데미지를 적용한다. */
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void PerformActionHit();

	/** 도약 프레임 AnimNotify가 호출. 활성 액션 row의 Dash_Distance/Duration으로 타겟 방향 전진 이동. */
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void PerformActionDash();

	/** 도약 루트모션 소스 제거. 어빌리티 종료/캔슬 시 호출돼 이동이 남지 않게 한다. */
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void EndActionDash();

	/** 윈드업 AnimNotifyState가 호출. 활성 액션 row 범위로 텔레그래프 표시. */
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void BeginActionTelegraph();

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void EndActionTelegraph();

	/** ULSGA_MonsterAction이 몽타주(Action_Ani) 등을 읽기 위해 활성 액션 row를 조회. */
	const FLSMonsterActionRow* GetActiveActionRow() const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	AActor* GetActiveTarget() const { return ActiveTarget.Get(); }

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool HasValidDamageEffect() const;

private:
	const FLSMonsterActionRow* FindActionRow(FName RowName) const;
	// 액션의 판정/표시 원점·방향. 도약 액션이면 착지 예정 지점(타겟까지 거리로 클램프)을 원점으로 한다.
	void ComputeActionOriginAndDirection(const FLSMonsterActionRow& Row, FVector& OutOrigin, FVector& OutDirection) const;
	FName SelectActionForDistance(float Distance) const;
	bool IsActionOnCooldown(FName RowName) const;
	void StartActionCooldown(FName RowName, float Cooldown);
	ULSSkillPreviewComponent* GetPreviewComponent() const;
	/** 전투 프로토콜 레벨 게이팅을 끼울 확장점. 현재는 항상 표시. */
	bool ShouldShowActionTelegraph() const;
	static ELSBreakPowerTier ToBreakPowerTier(int32 Impact);

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TObjectPtr<UDataTable> MonsterActionTable;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat|Telegraph")
	TObjectPtr<UMaterialInterface> TelegraphCircleMaterial;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat|Telegraph")
	TObjectPtr<UMaterialInterface> TelegraphBoxMaterial;

	UPROPERTY(EditAnywhere, Category="LS/Combat", meta=(ClampMin="0.0"))
	float LeashDistance = 2000.0f;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Combat")
	bool bCombatArchetypeApplied = false;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Combat")
	float AlertMoveSpeedMultiplier = 0.0f;

	// archetype의 Action_Group(DT_MonsterAction row 이름 목록) 캐시.
	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Combat")
	TArray<FName> ActionGroup;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Combat")
	FName ActiveActionRowName;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Combat")
	TWeakObjectPtr<AActor> ActiveTarget;

	// 액션별 쿨다운 만료 월드시각(초). 어트리뷰트가 아닌 AI 선택용 타이머.
	TMap<FName, double> ActionCooldownEndTimes;

	// 진행 중인 도약 루트모션 소스 ID. 0 = 없음(ERootMotionSourceID::Invalid).
	uint16 ActionDashRootMotionSourceID = 0;

	// 도약 착지 예정 지점/방향. PerformActionHit이 타격 프레임 타이밍과 무관하게 착지 위치에 데미지를 적용하도록 사용.
	bool bActionDashLandingValid = false;
	FVector ActionDashLandingLocation = FVector::ZeroVector;
	FVector ActionDashDirection = FVector::ZeroVector;
};
