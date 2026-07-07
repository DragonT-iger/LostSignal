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

	UFUNCTION(BlueprintCallable, Category="LS/UI|Skill")
	void SetPreviewBattleProtocolLevels(int32 CurrentBattleProtocol, int32 PreviousBattleProtocol);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Skill")
	void ClearPreviewBattleProtocolLevels();

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

	/*UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<ULSSkillSlotWidget> DashSlot;*/

private:
	void RefreshProtocolVisibility();
	bool IsSkillSlotProtocolVisible() const;
	void ResolveBattleProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const;

	UPROPERTY(Transient)
	TObjectPtr<ULSPlayerSkillComponent> SkillComponent;

	int32 PreviewCurrentBattleProtocol = 0;
	int32 PreviewPreviousBattleProtocol = 0;
	bool bUsePreviewBattleProtocolLevels = false;
};
