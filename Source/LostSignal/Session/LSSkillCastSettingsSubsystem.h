#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Skills/LSSkillTypes.h"
#include "LSSkillCastSettingsSubsystem.generated.h"

// 스킬 슬롯별 스마트키 사용 여부와 스마트키 공통 프리뷰 옵션을 GameUserSettings.ini에 저장한다.
// 실제 입력 처리에서는 GetSlotCastMode()로 최종 발동 모드를 해석해 사용한다.
UCLASS(config=GameUserSettings)
class LOSTSIGNAL_API ULSSkillCastSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	bool IsSlotSmartKeyEnabled(ELSPlayerSkillSlot Slot) const;

	UFUNCTION(BlueprintCallable, Category="LS/Skill")
	void SetSlotSmartKeyEnabled(ELSPlayerSkillSlot Slot, bool bEnabled);

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	bool IsSmartKeyPreviewOnReleaseEnabled() const { return bSmartKeyPreviewOnRelease; }

	UFUNCTION(BlueprintCallable, Category="LS/Skill")
	void SetSmartKeyPreviewOnReleaseEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category="LS/Skill")
	ELSSkillCastMode GetSlotCastMode(ELSPlayerSkillSlot Slot) const;

	// 기존 BP 호환용 API. 스마트키 여부와 공통 프리뷰 옵션으로 변환해 저장한다.
	UFUNCTION(BlueprintCallable, Category="LS/Skill")
	void SetSlotCastMode(ELSPlayerSkillSlot Slot, ELSSkillCastMode Mode);

private:
	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category="LS/Skill", meta=(AllowPrivateAccess="true"))
	bool bSkill1SmartKeyEnabled = true;

	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category="LS/Skill", meta=(AllowPrivateAccess="true"))
	bool bSkill2SmartKeyEnabled = true;

	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category="LS/Skill", meta=(AllowPrivateAccess="true"))
	bool bSkill3SmartKeyEnabled = true;

	UPROPERTY(config, VisibleAnywhere, BlueprintReadOnly, Category="LS/Skill", meta=(AllowPrivateAccess="true"))
	bool bSmartKeyPreviewOnRelease = true;
};
