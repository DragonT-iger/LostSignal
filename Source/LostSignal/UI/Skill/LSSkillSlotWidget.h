#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Skills/LSSkillTypes.h"

#include "LSSkillSlotWidget.generated.h"

class ULSPlayerSkillComponent;
class ULSSkillDataAsset;
class UImage;
class UTextBlock;
class UProgressBar;

/** Single skill slot widget. WBP must bind IconImage, CooldownText, and CooldownBar. */
UCLASS()
class LOSTSIGNAL_API ULSSkillSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI|Skill")
	void InitializeSlot(ULSPlayerSkillComponent* InSkillComponent, ELSPlayerSkillSlot InSlot);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UTextBlock> CooldownText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UProgressBar> CooldownBar;

private:
	void RefreshSkillIcon();
	void RefreshCooldown();

	UPROPERTY(Transient)
	TObjectPtr<ULSPlayerSkillComponent> SkillComponent;

	UPROPERTY(Transient)
	TObjectPtr<ULSSkillDataAsset> CachedSkillData;

	ELSPlayerSkillSlot Slot = ELSPlayerSkillSlot::Skill1;
};
