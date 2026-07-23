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
class UMaterialInstanceDynamic;

/** Single skill slot widget. WBP must bind IconImage, ShortcutText, CooldownText, and CooldownMaskImage. */
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

	// 쿨타임 방사형(시계방향 파이 와이프) 마스크. 아이콘과 같은 크기로 겹쳐 아이콘 위를 덮는다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UImage> CooldownMaskImage;

	// 마스크 머티리얼의 진행도(0~1) 스칼라 파라미터 이름.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Skill")
	FName CooldownProgressParameterName = TEXT("Progress");

	// false(기본): 남은시간(남은/총)을 채움값으로 넣어 덮인 부채꼴이 줄어든다.
	// true: 진행도(1 - 남은/총)를 넣어 쿨타임이 진행할수록 부채꼴이 차오른다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Skill")
	bool bCooldownFillByElapsed = false;

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
	TObjectPtr<UMaterialInstanceDynamic> CooldownMaskMaterial;

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
