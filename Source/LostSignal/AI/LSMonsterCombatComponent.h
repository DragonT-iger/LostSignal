#pragma once

#include "Combat/LSCombatTypes.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LSMonsterCombatComponent.generated.h"

class UGameplayEffect;
class UMaterialInterface;
class UPrimitiveComponent;
class ULSSkillPreviewComponent;
struct FLSMonsterArchetypeRow;
struct FLSMonsterActionRow;
struct FHitResult;

DECLARE_MULTICAST_DELEGATE(FLSMonsterActionChargeStarted);
DECLARE_MULTICAST_DELEGATE_OneParam(FLSMonsterActionChargeFinished, bool /* bHit */);

UENUM(BlueprintType)
enum class ELSMonsterTelegraphOrigin : uint8
{
	Caster UMETA(DisplayName="시전자"),
	Target UMETA(DisplayName="타겟")
};

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

	/** 복귀 등 전투 리셋 시 액션 쿨다운 타이머 전체 초기화. */
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void ResetActionCooldowns() { ActionCooldownEndTimes.Empty(); }

	/** 타격 프레임 AnimNotify가 호출. 활성 액션 row의 히트박스로 데미지를 적용한다. */
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void PerformActionHit();

	/** 도약 프레임 AnimNotify가 호출. 활성 액션 row의 Dash_Distance/Duration으로 타겟 방향 전진 이동. */
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void PerformActionDash();

	/** 충돌형 돌진 시작 Notify가 호출. 고정된 전방으로 이동하며 첫 플레이어 충돌 시 데미지를 적용한다. */
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void BeginActionCharge();

	/** 충돌형 돌진 상태와 이동을 결과 알림 없이 정리한다. 어빌리티 종료/캔슬 시 호출. */
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void CancelActionCharge();

	/** 도약 루트모션 소스 제거. 어빌리티 종료/캔슬 시 호출돼 이동이 남지 않게 한다. */
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void EndActionDash();

	/** 윈드업 AnimNotifyState Begin이 호출. 선택한 위치 기준에 활성 액션 row 범위를 표시한다. */
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void BeginActionTelegraph(float Duration = 0.0f, ELSMonsterTelegraphOrigin OriginMode = ELSMonsterTelegraphOrigin::Caster);

	/** 윈드업 AnimNotifyState Tick이 호출. 경과 시간 비율로 텔레그래프 fill을 0→1로 채운다. */
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void UpdateActionTelegraphFill(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void EndActionTelegraph();

	/** ULSGA_MonsterAction이 몽타주(Action_Ani) 등을 읽기 위해 활성 액션 row를 조회. */
	const FLSMonsterActionRow* GetActiveActionRow() const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	AActor* GetActiveTarget() const { return ActiveTarget.Get(); }

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool HasValidDamageEffect() const;

	FLSMonsterActionChargeStarted& OnActionChargeStarted() { return ActionChargeStartedDelegate; }
	FLSMonsterActionChargeFinished& OnActionChargeFinished() { return ActionChargeFinishedDelegate; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	const FLSMonsterActionRow* FindActionRow(FName RowName) const;
	// 액션의 이동/판정 원점·방향. 도약 액션이면 착지 예정 지점(타겟까지 거리로 클램프)을 원점으로 한다.
	void ComputeActionOriginAndDirection(const FLSMonsterActionRow& Row, FVector& OutOrigin, FVector& OutDirection) const;
	// 텔레그래프 NotifyState가 선택한 시전자/타겟 위치 기준 원점과 방향을 계산한다.
	void ComputeActionTelegraphOriginAndDirection(ELSMonsterTelegraphOrigin OriginMode, FVector& OutOrigin, FVector& OutDirection) const;
	// 액션 row의 CC_Type/CC_Value를 캐릭터 스킬과 같은 경로(강인도 게이트 + ApplyKnockback)로 명중 대상에 적용.
	void ApplyActionCrowdControl(const FLSMonsterActionRow& Row, AActor* HitActor, const FVector& Origin, const FVector& AimDir, ELSBreakPowerTier BreakPower) const;
	FName SelectActionForDistance(float Distance) const;
	bool IsActionOnCooldown(FName RowName) const;
	void StartActionCooldown(FName RowName, float Cooldown);
	bool TryApplyActionDamage(const FLSMonsterActionRow& Row, AActor* HitActor, const FVector& Origin, const FVector& AimDir, ELSBreakPowerTier BreakPower) const;
	void FinishActionCharge(bool bHit);
	void HandleActionChargeTimeout();
	UFUNCTION()
	void HandleOwnerCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
	ULSSkillPreviewComponent* GetPreviewComponent() const;
	/** 전투 프로토콜 레벨 게이팅을 끼울 확장점. 현재는 항상 표시. */
	bool ShouldShowActionTelegraph() const;
	static ELSBreakPowerTier ToBreakPowerTier(int32 Impact);

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

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

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ResolvedTelegraphCircleMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ResolvedTelegraphBoxMaterial;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Combat")
	TWeakObjectPtr<AActor> ActiveTarget;

	// 액션별 쿨다운 만료 월드시각(초). 어트리뷰트가 아닌 AI 선택용 타이머.
	TMap<FName, double> ActionCooldownEndTimes;

	// 진행 중인 도약 루트모션 소스 ID. 0 = 없음(ERootMotionSourceID::Invalid).
	uint16 ActionDashRootMotionSourceID = 0;

	// 전용 ChargeStart Notify가 시작한 충돌형 돌진 상태. DataTable 스키마와 분리해 몽타주 저작으로 선택한다.
	bool bActionChargeActive = false;
	bool bActionUsesContactHit = false;
	FTimerHandle ActionChargeTimerHandle;
	FLSMonsterActionChargeStarted ActionChargeStartedDelegate;
	FLSMonsterActionChargeFinished ActionChargeFinishedDelegate;

	// 도약 착지 예정 지점/방향. PerformActionHit이 타격 프레임 타이밍과 무관하게 착지 위치에 데미지를 적용하도록 사용.
	bool bActionDashLandingValid = false;
	FVector ActionDashLandingLocation = FVector::ZeroVector;
	FVector ActionDashDirection = FVector::ZeroVector;

	// 텔레그래프 fill 차오름: 윈드업 NotifyState 윈도우 길이와 경과 시간.
	float TelegraphDuration = 0.0f;
	float TelegraphElapsed = 0.0f;
};
