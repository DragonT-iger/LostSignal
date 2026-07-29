#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Skills/LSSkillAreaTypes.h"
#include "Skills/LSSkillTypes.h"
#include "LSPlayerSkillComponent.generated.h"

class ULSSkillDataAsset;
class ULSSkillPoolDataAsset;
class ULSPassiveSkillDataAsset;
class ULSSkillPreviewComponent;
class UGameplayAbility;
struct FLSCharacterSkillRow;

UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSPlayerSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSPlayerSkillComponent();

	UFUNCTION(BlueprintCallable, Category="LS/Skill")
	bool BeginSkillPreview(ELSPlayerSkillSlot Slot);

	UFUNCTION(BlueprintCallable, Category="LS/Skill")
	void UpdateActiveSkillPreview(const FVector& WorldLocation, const FRotator& WorldRotation);

	UFUNCTION(BlueprintCallable, Category="LS/Skill")
	bool ConfirmActiveSkillPreview(ELSPlayerSkillSlot Slot);

	UFUNCTION(BlueprintCallable, Category="LS/Skill")
	bool ConfirmAnyActiveSkillPreview(const FVector& TargetLocation, const FRotator& AimRotation);

	// 프리뷰 없이 즉시 발동(즉시 퀵캐스트 모드용). 진행 중인 프리뷰가 있으면 취소 후 발동한다.
	UFUNCTION(BlueprintCallable, Category="LS/Skill")
	bool ActivateSkillInstant(ELSPlayerSkillSlot Slot, const FVector& TargetLocation, const FRotator& AimRotation);

	UFUNCTION(BlueprintCallable, Category="LS/Skill")
	void CancelActiveSkillPreview(ELSPlayerSkillSlot Slot);

	UFUNCTION(BlueprintCallable, Category="LS/Skill")
	void CancelAnyActiveSkillPreview();

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	bool IsPreviewingSkill() const { return ActiveSkillData != nullptr; }

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	ELSPlayerSkillSlot GetActiveSlot() const { return ActiveSlot; }

	// 슬롯의 실제 적용 캐스트 모드. 디버그 오버라이드가 켜져 있으면 그 값을, 아니면 설정 저장소 값을 반환한다.
	UFUNCTION(BlueprintPure, Category="LS/Skill")
	ELSSkillCastMode GetEffectiveCastMode(ELSPlayerSkillSlot Slot) const;

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	ULSSkillDataAsset* GetSkillData(ELSPlayerSkillSlot Slot) const;

	UFUNCTION(BlueprintCallable, Category="LS/Skill")
	bool SetSkillData(ELSPlayerSkillSlot Slot, ULSSkillDataAsset* NewSkillData);

	UFUNCTION(BlueprintCallable, Category="LS/Skill|Enhancement")
	bool ApplySkillEnhancementByIndex(ELSPlayerSkillSlot Slot, int32 EnhancementIndex);

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	bool GetActivePreviewSpec(FLSSkillAreaPreviewSpec& OutPreviewSpec) const;

	void HandleBasicAttackHit(int32 ComboIndex, int32 ComboAttackID, int32 ValidHitCount);
	bool ConsumePendingAbilityContext(TSubclassOf<UGameplayAbility> AbilityClass, FLSSkillActivationContext& OutContext);
	bool ApplySkillCooldown(const ULSSkillDataAsset* SkillData) const;
	bool IsSkillCooldownActive(const ULSSkillDataAsset* SkillData) const;
	float GetSkillCooldownRemaining(const ULSSkillDataAsset* SkillData) const;
	float GetSkillCooldownTotalDuration(const ULSSkillDataAsset* SkillData) const;

	// 디버그: 이 캐릭터의 모든 스킬(및 대쉬) 쿨타임을 즉시 초기화한다. 서버 권한에서만 유효.
	UFUNCTION(BlueprintCallable, Category="LS/Skill|Debug")
	void ResetAllSkillCooldowns();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 세이브의 캐릭터별 스킬 로드아웃(SkillPool->CharacterID 키)을 SkillPool로 해석해 3칸에 적용한다.
	// 저장된 선택이 하나도 없으면 BP 기본 SkillSlots를 폴백 기본 로드아웃으로 유지한다.
	void ApplyEquippedSkillLoadout();

	// 슬롯을 비운다(SetSkillData는 nullptr을 거부하므로 해제 전용 경로).
	void ClearSkillSlot(ELSPlayerSkillSlot Slot);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TMap<ELSPlayerSkillSlot, FLSPlayerSkillSlotSpec> SkillSlots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TArray<TObjectPtr<ULSPassiveSkillDataAsset>> PassiveSkills;

	// 이 캐릭터가 로비에서 고를 수 있는 스킬 후보 풀. 세이브에 저장된 Skill_ID를 DataAsset으로 해석할 때 쓴다.
	// 캐릭터 BP에서 DA_*_SkillPool을 매핑한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TObjectPtr<ULSSkillPoolDataAsset> SkillPool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/Skill|Debug")
	bool bAlwaysShowSkillPreviewDebug = false;

	// 디버그: 켜면 아래 맵의 슬롯 캐스트 모드를 강제한다(설정 저장소 무시). 맵에 없는 슬롯은 저장소 값을 따른다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/Skill|Debug")
	bool bOverrideCastModeForDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/Skill|Debug", meta=(EditCondition="bOverrideCastModeForDebug"))
	TMap<ELSPlayerSkillSlot, ELSSkillCastMode> DebugCastModeOverrides;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill")
	TObjectPtr<ULSSkillDataAsset> ActiveSkillData;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill")
	ELSPlayerSkillSlot ActiveSlot = ELSPlayerSkillSlot::Skill1;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill")
	FLSSkillActivationContext PendingAbilityContext;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill")
	TSubclassOf<UGameplayAbility> PendingAbilityClass;

private:
	UFUNCTION(Server, Reliable)
	void ServerRequestActivateSkill(ELSPlayerSkillSlot Slot, FVector_NetQuantize TargetLocation, float AimYaw);

	bool CanUseLocalPreview() const;
	bool IsSkillRangeProtocolVisible() const;
	void ResolveBattleProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const;
	bool ActivateSkillOnServer(ELSPlayerSkillSlot Slot, const FVector& TargetLocation, float AimYaw);
	// 프리뷰 확정/즉발 공통 발동 커밋: 사거리 클램프 후 서버 직접 발동 또는 클라 예측+서버 RPC.
	bool CommitSkillActivation(ELSPlayerSkillSlot Slot, ULSSkillDataAsset* SkillData, const FVector& TargetLocation, const FRotator& AimRotation);
	const FLSCharacterSkillRow* ResolveActiveSkillRow(const ULSSkillDataAsset* SkillData, const TCHAR* Context) const;
	FLSSkillAreaPreviewSpec BuildPreviewSpecForSkill(const ULSSkillDataAsset* SkillData) const;
	float ResolveSkillCooldownDuration(const ULSSkillDataAsset* SkillData) const;
	float ResolveReducedSkillCooldownDuration(float BaseDuration) const;
	FVector ClampTargetLocationToCastRange(const ULSSkillDataAsset* SkillData, const FVector& TargetLocation) const;
	void LogSkillCooldownBlocked(const ULSSkillDataAsset* SkillData, const TCHAR* Phase) const;
	bool TryActivateGameplayAbility(ULSSkillDataAsset* SkillData, const FLSSkillActivationContext& Context);
	// 스킬 시전음을 GameplayCue로 발동(서버→전 클라 멀티캐스트). SkillData.CastSound를 Cue 파라미터로 전달.
	void PlaySkillCastCue(const ULSSkillDataAsset* SkillData) const;
	bool TrySendPassiveGameplayEvent(ULSPassiveSkillDataAsset* SkillData, int32 ComboIndex, int32 ComboAttackID) const;
	bool TryPredictFastMovementSkill(ULSSkillDataAsset* SkillData, const FVector& TargetLocation, float AimYaw);
	bool ResolvePredictedFastMovementParams(ULSSkillDataAsset* SkillData, float& OutDistance, float& OutDuration) const;
	void IgnoreEnemiesForPredictedFastMovement(ACharacter* OwnerCharacter, const FVector& StartLocation, const FVector& Direction, float Distance);
	void ClearIgnoredEnemiesForPredictedFastMovement(ACharacter* OwnerCharacter);
	void FinishPredictedFastMovementSkill();
	ULSSkillPreviewComponent* ResolvePreviewComponent() const;

	FTimerHandle PredictedFastMovementTimerHandle;
	uint16 PredictedFastMovementRootMotionSourceID = 0;
	TArray<TWeakObjectPtr<AActor>> PredictedFastMovementIgnoredEnemies;
	bool bPredictedFastMovementInProgress = false;
};
