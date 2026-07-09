#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Skills/LSSkillTypes.h"

#include "LSSkillSlotWidget.generated.h"

class ULSPlayerSkillComponent;
class ULSSkillDataAsset;
class UInputAction;
class UImage;
class UTextBlock;
class UProgressBar;

/** Single skill slot widget. WBP must bind IconImage, ShortcutText, CooldownText, and CooldownBar. */
UCLASS()
class LOSTSIGNAL_API ULSSkillSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI|Skill")
	void InitializeSlot(ULSPlayerSkillComponent* InSkillComponent, ELSPlayerSkillSlot InSlot);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Skill")
	void SetShortcutTextOverride(bool bInTextOverride, const FText& InTextOverride);

	void SetPreviewBattleProtocolLevels(int32 CurrentBattleProtocol, int32 PreviousBattleProtocol);
	void ClearPreviewBattleProtocolLevels();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UTextBlock> ShortcutText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UTextBlock> CooldownText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UProgressBar> CooldownBar;

private:
	void RefreshSkillIcon();
	void RefreshShortcutText();
	void RefreshCooldown();
	FText ResolveShortcutText() const;
	FText ResolveShortcutTextFromInputMappings(const UInputAction* InputAction) const;
	static FText GetShortcutTextForSlot(ELSPlayerSkillSlot InSlot);
	bool IsCooldownNumberProtocolVisible() const;
	bool IsCooldownGaugeProtocolVisible() const;
	void ResolveBattleProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const;

	UPROPERTY(Transient)
	TObjectPtr<ULSPlayerSkillComponent> SkillComponent;

	UPROPERTY(Transient)
	TObjectPtr<ULSSkillDataAsset> CachedSkillData;

	ELSPlayerSkillSlot Slot = ELSPlayerSkillSlot::Skill1;
	FText ShortcutTextOverride;
	int32 PreviewCurrentBattleProtocol = 0;
	int32 PreviewPreviousBattleProtocol = 0;
	bool bShortcutTextOverride = false;
	bool bUsePreviewBattleProtocolLevels = false;
};
