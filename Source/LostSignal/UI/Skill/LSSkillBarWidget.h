#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Skills/LSSkillTypes.h"
#include "LSSkillBarWidget.generated.h"

class ULSSkillSlotWidget;
class ULSPlayerSkillComponent;

/** Skill bar widget that binds the player skill slots (Skill1~4) and the dash display slot. */
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

	UFUNCTION(BlueprintCallable, Category="LS/UI|Skill")
	void SetTextOverrideEnabled(bool bInTextOverride);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Skill")
	void SetSkillSlotTextOverride(ELSPlayerSkillSlot InSlot, const FText& InTextOverride);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/UI|Skill|Text")
	bool bTextOverride = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/UI|Skill|Text", meta=(EditCondition="bTextOverride"))
	FText Skill1TextOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/UI|Skill|Text", meta=(EditCondition="bTextOverride"))
	FText Skill2TextOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/UI|Skill|Text", meta=(EditCondition="bTextOverride"))
	FText Skill3TextOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/UI|Skill|Text", meta=(EditCondition="bTextOverride"))
	FText Skill4TextOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/UI|Skill|Text", meta=(EditCondition="bTextOverride"))
	FText DashTextOverride;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<ULSSkillSlotWidget> Skill1Slot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<ULSSkillSlotWidget> Skill2Slot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<ULSSkillSlotWidget> Skill3Slot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<ULSSkillSlotWidget> Skill4Slot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<ULSSkillSlotWidget> DashSlot;

private:
	void ApplyTextOverridesToSlots();
	void RefreshProtocolVisibility();
	bool IsSkillSlotProtocolVisible() const;
	void ResolveBattleProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const;

	UPROPERTY(Transient)
	TObjectPtr<ULSPlayerSkillComponent> SkillComponent;

	int32 PreviewCurrentBattleProtocol = 0;
	int32 PreviewPreviousBattleProtocol = 0;
	bool bUsePreviewBattleProtocolLevels = false;
};
