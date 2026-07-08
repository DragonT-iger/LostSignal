#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Skills/LSSkillTypes.h"
#include "LSControlSkillRowWidget.generated.h"

class UCheckBox;
class UTextBlock;
class ULSSkillCastSettingsSubsystem;

// 컨트롤 설정 화면의 스킬 슬롯 1줄. 슬롯별 스마트키 사용 여부만 저장한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSControlSkillRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="LS/UI|Settings")
	void InitializeRow(ELSPlayerSkillSlot InSlot, const FText& InSlotLabel);

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<UTextBlock> SlotNameText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<UCheckBox> SmartKeyCheckBox;

	UFUNCTION(BlueprintImplementableEvent, Category="LS/UI|Settings")
	void OnSmartKeyChanged(bool bEnabled);

private:
	UFUNCTION()
	void HandleSmartKeyCheckChanged(bool bIsChecked);

	void RefreshSmartKeyCheck(bool bEnabled);
	void BindCheckBoxEvents();
	void UnbindCheckBoxEvents();
	void LogMissingBindings() const;
	ULSSkillCastSettingsSubsystem* GetCastSettingsSubsystem() const;

	ELSPlayerSkillSlot Slot = ELSPlayerSkillSlot::Skill1;
	bool bRefreshingCheckBox = false;
};
