#pragma once

#include "Combat/LSCombatTypes.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSPlayerCombatComponent.generated.h"

class UAnimMontage;
class UGameplayAbility;
class UGameplayEffect;
class UNiagaraSystem;
class ULSGA_PlayerBasicAttack;
class ULSAimComponent;
class ULSCharacterCombatComponent;
class ULSCombatStateComponent;
class ULSSkillPreviewComponent;
struct FLSComboAttackRow;

UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSPlayerCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSPlayerCombatComponent();

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	bool RequestBasicAttack();

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void SetBasicAttackHeld(bool bHeld);

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool IsBasicAttackHeld() const { return bBasicAttackHeld; }

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	UAnimMontage* GetBasicAttackMontage() const { return AttackMontage; }

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void ResetBasicAttackHit();

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void SetPendingBasicAttackComboIndexOverride(int32 ComboIndex, float ExpireSeconds, int32 ComboTag = 0);

	bool ConsumePendingBasicAttackComboIndexOverride(int32& OutComboIndex, int32& OutComboTag);
	const FLSComboAttackRow* ResolveComboAttackRow(int32 ComboSectionIndex, int32 ComboTagOverride) const;

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	bool RequestDash();

	bool RequestDash(const FVector& DashDirection);
	bool PredictDashMovement(const FVector& DashDirection);
	bool CanRequestDashLocally() const;
	bool SubmitDashInput(const FVector& DashDirection, bool& bOutShouldExecuteImmediately);
	bool GetPendingDashDirection(FVector& OutDashDirection) const;

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void PerformMeleeHit();

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void HandleCombatActionEnd(ELSCombatActionState ExpectedState);

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool IsAttackInProgress() const;

	// 현재 콤보 스윙의 히트 판정(LSAN_PlayerMeleeHit)이 이미 발동했는지. 히트 프레임 이후 회전 잠금 해제 판정에 사용.
	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool IsBasicAttackHitConsumed() const { return bAttackHitConsumed; }

protected:
	virtual void BeginPlay() override;
	// 디버그 기본 공격 범위 표시(LS.Debug.BasicAttackRange) 전용 틱. 구현: LSPlayerCombatDebugPreview.cpp
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TSubclassOf<UGameplayAbility> BasicAttackAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TSubclassOf<UGameplayAbility> DashAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TSubclassOf<UGameplayEffect> BasicAttackDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float AttackCancelBlendOutTime = 0.08f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float BasicAttackForwardOffset = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float BasicAttackRadius = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="1.0"))
	float DamageEffectLevel = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float BasicAttackFixedDamage = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float BasicAttackAttackCoefficient = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	bool bBasicAttackCanCrit = true;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	ELSBreakPowerTier BasicAttackBreakPower = ELSBreakPowerTier::NormalAttack;

	// 기본 공격이 실제로 명중했을 때 피격 위치에 재생할 Niagara. 캐릭터 BP에서 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat/Effects")
	TObjectPtr<UNiagaraSystem> BasicAttackHitEffect;

	// 지정하면 공격자 Mesh의 해당 소켓 월드 위치에 명중 VFX를 생성한다. 비어 있거나 소켓이 없으면 피격 대상 중심을 사용한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat/Effects")
	FName BasicAttackHitEffectSocketName = NAME_None;

	// 명중 VFX의 균일 스케일 배율.
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat/Effects", meta=(ClampMin="0.01"))
	float BasicAttackHitEffectScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	int32 ComboCharacterID = 101;

	FTimerHandle PredictedDashTimerHandle;
	FTimerHandle PredictedDashCooldownTimerHandle;
	FTimerHandle PendingComboIndexOverrideTimerHandle;
	bool bAttackHitConsumed = false;
	bool bBasicAttackHeld = false;
	bool bDebugRangePreviewActive = false;
	int32 DebugRangePreviewKey = INDEX_NONE;
	bool bPredictedDashInProgress = false;
	bool bPredictedDashCooldownActive = false;
	uint16 PredictedDashRootMotionSourceID = 0;
	FVector PendingDashDirection = FVector::ZeroVector;
	int32 PendingComboIndexOverride = INDEX_NONE;
	int32 PendingComboTagOverride = 0;

	ULSAimComponent* ResolveAimComponent() const;
	ULSCharacterCombatComponent* ResolveSharedCombatComponent() const;
	ULSCombatStateComponent* ResolveCombatStateComponent() const;
	class ALSCharacterBase* ResolveOwnerCharacter() const;
	FVector ResolveBasicAttackDirection() const;
	// 디버그 범위 표시(LS.Debug.BasicAttackRange) — 구현: LSPlayerCombatDebugPreview.cpp
	void UpdateDebugBasicAttackRangePreview();
	void EndDebugBasicAttackRangePreview();
	void BuildDebugBasicAttackRangeSpec(const FLSComboAttackRow* ComboRow, struct FLSSkillAreaPreviewSpec& OutSpec, float& OutForwardOffset) const;
	void FinishAttack();
	void CancelAttackForDash();
	void TryExecuteBufferedCommand();
	void FinishPredictedDash();
	void FinishPredictedDashCooldown();
	void ClearPendingBasicAttackComboIndexOverride();
	// 브로드페이즈(구체) + Row Range shape 정밀 필터로 명중 후보를 수집. 반환값: Row Range 사용 여부(false면 고정 구체 폴백).
	bool GatherBasicAttackTargets(const FVector& AttackDirection, const FLSComboAttackRow* ComboRow, TArray<AActor*>& OutActors) const;
	int32 ExecuteMeleeHit(const FVector& AttackDirection, const FLSComboAttackRow* ComboRow);
	void ExecuteBasicAttackHitCue(AActor* HitActor, const FVector& AttackDirection) const;
	int32 ResolveComboAttackID(int32 ComboSectionIndex, int32 ComboTagOverride) const;
	ULSGA_PlayerBasicAttack* FindActiveBasicAttackAbility() const;
	bool ApplyDashRootMotion(const FVector& DashDirection, uint16& OutRootMotionSourceID) const;
	float GetDashDuration() const;
	float GetDashCooldown() const;
	bool IsDashCooldownActive() const;

	UFUNCTION(Server, Reliable)
	void ServerRequestBasicAttack();

	UFUNCTION(Server, Reliable)
	void ServerSetBasicAttackHeld(bool bHeld);
};
