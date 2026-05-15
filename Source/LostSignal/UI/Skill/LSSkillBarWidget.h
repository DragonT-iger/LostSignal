#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSSkillBarWidget.generated.h"

class ULSSkillSlotWidget;
class ULSPlayerSkillComponent;

/** Skill bar widget that binds the five player skill slots. */
UCLASS()
class LOSTSIGNAL_API ULSSkillBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI|Skill")
	void InitializeSkillBar(ULSPlayerSkillComponent* InSkillComponent);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<ULSSkillSlotWidget> Skill1Slot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<ULSSkillSlotWidget> Skill2Slot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<ULSSkillSlotWidget> Skill3Slot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<ULSSkillSlotWidget> Skill4Slot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<ULSSkillSlotWidget> UltimateSlot;

private:
	UPROPERTY(Transient)
	TObjectPtr<ULSPlayerSkillComponent> SkillComponent;
};
