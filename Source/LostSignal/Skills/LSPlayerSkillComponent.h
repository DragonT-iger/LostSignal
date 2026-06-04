#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Skills/LSSkillAreaTypes.h"
#include "Skills/LSSkillTypes.h"
#include "LSPlayerSkillComponent.generated.h"

class ULSSkillDataAsset;
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

	UFUNCTION(BlueprintCallable, Category="LS/Skill")
	void CancelActiveSkillPreview(ELSPlayerSkillSlot Slot);

	UFUNCTION(BlueprintCallable, Category="LS/Skill")
	void CancelAnyActiveSkillPreview();

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	bool IsPreviewingSkill() const { return ActiveSkillData != nullptr; }

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	ELSPlayerSkillSlot GetActiveSlot() const { return ActiveSlot; }

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	ULSSkillDataAsset* GetSkillData(ELSPlayerSkillSlot Slot) const;

	UFUNCTION(BlueprintCallable, Category="LS/Skill")
	bool SetSkillData(ELSPlayerSkillSlot Slot, ULSSkillDataAsset* NewSkillData);

	UFUNCTION(BlueprintCallable, Category="LS/Skill|Enhancement")
	bool ApplySkillEnhancementByIndex(ELSPlayerSkillSlot Slot, int32 EnhancementIndex);

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	bool GetActivePreviewSpec(FLSSkillAreaPreviewSpec& OutPreviewSpec) const;

	void HandleBasicAttackHit(int32 ComboIndex, int32 ValidHitCount);
	bool ConsumePendingAbilityContext(TSubclassOf<UGameplayAbility> AbilityClass, FLSSkillActivationContext& OutContext);
	bool ApplySkillCooldown(const ULSSkillDataAsset* SkillData) const;
	bool IsSkillCooldownActive(const ULSSkillDataAsset* SkillData) const;
	float GetSkillCooldownRemaining(const ULSSkillDataAsset* SkillData) const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TMap<ELSPlayerSkillSlot, FLSPlayerSkillSlotSpec> SkillSlots;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TArray<TObjectPtr<ULSPassiveSkillDataAsset>> PassiveSkills;

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
	bool ActivateSkillOnServer(ELSPlayerSkillSlot Slot, const FVector& TargetLocation, float AimYaw);
	const FLSCharacterSkillRow* ResolveActiveSkillRow(const ULSSkillDataAsset* SkillData, const TCHAR* Context) const;
	FLSSkillAreaPreviewSpec BuildPreviewSpecForSkill(const ULSSkillDataAsset* SkillData) const;
	float ResolveSkillCooldownDuration(const ULSSkillDataAsset* SkillData) const;
	FVector ClampTargetLocationToCastRange(const ULSSkillDataAsset* SkillData, const FVector& TargetLocation) const;
	void LogSkillCooldownBlocked(const ULSSkillDataAsset* SkillData, const TCHAR* Phase) const;
	bool TryActivateGameplayAbility(ULSSkillDataAsset* SkillData, const FLSSkillActivationContext& Context);
	bool TrySendPassiveGameplayEvent(ULSPassiveSkillDataAsset* SkillData, int32 ComboIndex) const;
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
