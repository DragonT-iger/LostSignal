#pragma once

#include "Combat/LSCombatTypes.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "UI/Combat/LSDamageNumberTypes.h"
#include "LSCharacterCombatComponent.generated.h"

class AActor;
class ALSCharacterBase;
class UAbilitySystemComponent;
class UGameplayEffect;
struct FOnAttributeChangeData;
enum class ELSCharacterSkillEffectTarget : uint8;

UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSCharacterCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSCharacterCombatComponent();

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	ALSCharacterBase* GetOwnerCharacter() const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool HasCombatTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool IsDead() const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool CanStartAttack() const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	ELSTenacityTier GetCurrentTenacityTier() const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	FLSImpactResolution ResolveIncomingImpact(ELSBreakPowerTier BreakPowerTier) const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool CanApplyCrowdControl(ELSBreakPowerTier BreakPowerTier) const;

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void SetCombatTagActive(FGameplayTag Tag, bool bActive);

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	bool ApplyKnockback(const FVector& Direction, float Speed, float Duration, float UpSpeed = 0.0f);

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	bool ApplyDamageEffectToTarget(
		AActor* TargetActor,
		TSubclassOf<UGameplayEffect> DamageEffectClass,
		float EffectLevel = 1.0f,
		float FixedDamage = 0.0f,
		float AttackCoefficient = 0.0f,
		bool bCanCrit = false,
		ELSBreakPowerTier BreakPowerTier = ELSBreakPowerTier::NormalAttack) const;

	/**
	 * DataTable row의 상태이상 항목 하나를 Effect_Target에 따라 자신/대상에게 적용한다(서버 전용).
	 * 스킬/콤보 row가 공통으로 사용하는 진입점. 실제 적용은 대상의 ULSStatusEffectComponent가 담당한다.
	 * @return 적용되면 true (StatusID<=0, None/Ally 대상, 컴포넌트 부재 시 false)
	 */
	bool ApplyStatusEffectFromRow(int32 StatusID, ELSCharacterSkillEffectTarget EffectTarget, float Duration, AActor* HitTarget) const;

protected:
	virtual void BeginPlay() override;

private:
	void BindHealthDelegates();
	void BindStateTagDelegates();
	void HandleCurrentHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleStunnedTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	void RefreshDeathState();
	void HandleDeathStateChanged(bool bIsDead);
	void HandleStunStateChanged(bool bIsStunned);
	void BroadcastDamageNumberToPlayers(const FLSDamageNumberPayload& Payload) const;
	void FinishKnockback();
	void ClearKnockback();
	bool CanDamageTarget(AActor* TargetActor) const;
	bool IsFriendlyTarget(AActor* TargetActor) const;

	UPROPERTY()
	TMap<FGameplayTag, int32> LooseTagRefCounts;

	FTimerHandle KnockbackTimerHandle;
	uint16 KnockbackRootMotionSourceID = 0;
	bool bKnockbackActive = false;

	bool bCachedIsDead = false;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Combat")
	FVector DamageNumberWorldOffset = FVector(0.0f, 0.0f, 120.0f);
};
