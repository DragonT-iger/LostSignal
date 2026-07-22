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
class ULSCharacterHitAudioData;
class USoundBase;
struct FOnAttributeChangeData;
struct FLSConsumableRow;
struct FLSConsumableEffectRow;
enum class ELSCharacterSkillEffectTarget : uint8;
enum class ELSConsumableEffectTarget : uint8;

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

	/** 지속시간·감속 커브는 전 스킬·몬스터 공용값(ULSCombatSettings)을 읽는다. 호출부는 방향·속도만 결정. */
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	bool ApplyKnockback(const FVector& Direction, float Speed);

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	bool ApplyDamageEffectToTarget(
		AActor* TargetActor,
		TSubclassOf<UGameplayEffect> DamageEffectClass,
		float EffectLevel = 1.0f,
		float FixedDamage = 0.0f,
		float AttackCoefficient = 0.0f,
		bool bCanCrit = false,
		ELSBreakPowerTier BreakPowerTier = ELSBreakPowerTier::NormalAttack) const;

	void RecordDamageExecutionResult(float DamageAmount, bool bCriticalHit) const;

	/**
	 * DataTable row의 상태이상 항목 하나를 Effect_Target에 따라 자신/대상에게 적용한다(서버 전용).
	 * 스킬/콤보 row가 공통으로 사용하는 진입점. 실제 적용은 대상의 ULSStatusEffectComponent가 담당한다.
	 * @return 적용되면 true (StatusID<=0, None/Ally 대상, 컴포넌트 부재 시 false)
	 */
	bool ApplyStatusEffectFromRow(int32 StatusID, ELSCharacterSkillEffectTarget EffectTarget, float Duration, AActor* HitTarget) const;

	/**
	 * 소모품 Row의 효과 배열을 성질(Kind)별 경로로 적용한다(서버 전용).
	 * InstantAttribute는 SetByCaller Instant GE, Apply/RemoveStatus는 대상의 ULSStatusEffectComponent로 라우팅한다.
	 * 소유 액터가 사용자(Self)이며, Enemy 효과는 HitTarget에 적용한다.
	 * @return 효과 중 하나라도 적용되면 true
	 */
	bool ApplyConsumableEffects(const FLSConsumableRow& ConsumableRow, AActor* HitTarget = nullptr) const;

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

	// 소모품 효과 대상(Self=소유자, Enemy=HitTarget)을 해석한다. Friendly/All은 미지원.
	AActor* ResolveConsumableEffectActor(ELSConsumableEffectTarget Target, AActor* OwnerActor, AActor* HitTarget) const;
	// Attribute 효과(즉발 수치 가감)를 SetByCaller Instant GE로 적용한다(수치는 사전 참조 원소가 전달한 Value).
	bool ApplyConsumableAttributeEffect(AActor* EffectActor, const FLSConsumableEffectRow& EffectDef, float Value, AActor* Instigator) const;
	// Status 효과(부여/제거)를 대상의 ULSStatusEffectComponent로 라우팅한다.
	bool ApplyConsumableStatusEffect(AActor* EffectActor, const FLSConsumableEffectRow& EffectDef, AActor* Instigator) const;
	// 피격 시 재질 임팩트음 + 보이스를 GameplayCue로 발동(서버→전 클라 복제). 피격자 데이터(HitAudioData)가 종류·재질별 단일 출처.
	void PlayHitAudio();
	// 주어진 사운드를 파라미터에 실어 GameplayCue 발동(피격자 위치). 권한 측에서 호출돼 멀티캐스트된다.
	void FireHitAudioCue(FGameplayTag CueTag, USoundBase* Sound) const;

	UPROPERTY()
	TMap<FGameplayTag, int32> LooseTagRefCounts;

	FTimerHandle KnockbackTimerHandle;
	uint16 KnockbackRootMotionSourceID = 0;
	bool bKnockbackActive = false;

	bool bCachedIsDead = false;
	mutable float LastDamageExecutionAmount = 0.0f;
	mutable bool bLastDamageExecutionCritical = false;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Combat")
	FVector DamageNumberWorldOffset = FVector(0.0f, 0.0f, 120.0f);

	// 캐릭터/몬스터별 피격 오디오 묶음. 캐릭터 BP의 컴포넌트 기본값에서 매핑(미할당이면 무음).
	UPROPERTY(EditDefaultsOnly, Category="LS/Audio")
	TObjectPtr<ULSCharacterHitAudioData> HitAudioData;

	// 소모품 즉발 어트리뷰트 변경용 GE. 기본값은 네이티브 클래스이며 BP에서 교체 가능.
	UPROPERTY(EditDefaultsOnly, Category="LS/Consumable")
	TSubclassOf<UGameplayEffect> HealthChangeEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/Consumable")
	TSubclassOf<UGameplayEffect> StaminaChangeEffectClass;

	// 임팩트음/보이스 스로틀용 마지막 재생 월드시각(초).
	double LastImpactTime = 0.0;
	double LastVoiceTime = 0.0;
};
