#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Skills/LSSkillAreaTypes.h"
#include "Skills/LSSkillTypes.h"
#include "LSPlayerSkillComponent.generated.h"

class ULSSkillDataAsset;
class ULSSkillPreviewComponent;

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

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	bool GetActivePreviewSpec(FLSSkillAreaPreviewSpec& OutPreviewSpec) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TMap<ELSPlayerSkillSlot, FLSPlayerSkillSlotSpec> SkillSlots;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill")
	TObjectPtr<ULSSkillDataAsset> ActiveSkillData;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill")
	ELSPlayerSkillSlot ActiveSlot = ELSPlayerSkillSlot::Skill1;

private:
	UFUNCTION(Server, Reliable)
	void ServerRequestActivateSkill(ELSPlayerSkillSlot Slot, FVector_NetQuantize TargetLocation, float AimYaw);

	bool CanUseLocalPreview() const;
	bool ActivateSkillOnServer(ELSPlayerSkillSlot Slot, const FVector& TargetLocation, float AimYaw);
	ULSSkillPreviewComponent* ResolvePreviewComponent() const;
};
