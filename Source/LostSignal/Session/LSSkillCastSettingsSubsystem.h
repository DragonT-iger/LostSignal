#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Skills/LSSkillTypes.h"
#include "LSSkillCastSettingsSubsystem.generated.h"

// 스킬 슬롯별 발동 방식(프리뷰-확정/홀드-프리뷰/즉시) 저장소. GameUserSettings.ini에 저장되어
// New Game으로도 초기화되지 않는다. 설정 UI(WBP)는 여기 BlueprintCallable 게터/세터로 값을 읽고 쓴다.
UCLASS(config=GameUserSettings)
class LOSTSIGNAL_API ULSSkillCastSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="LS/Skill")
	ELSSkillCastMode GetSlotCastMode(ELSPlayerSkillSlot Slot) const;

	// 슬롯 모드를 지정하고 즉시 config에 저장한다.
	UFUNCTION(BlueprintCallable, Category="LS/Skill")
	void SetSlotCastMode(ELSPlayerSkillSlot Slot, ELSSkillCastMode Mode);

private:
	UPROPERTY(config)
	ELSSkillCastMode Skill1CastMode = ELSSkillCastMode::PreviewConfirm;

	UPROPERTY(config)
	ELSSkillCastMode Skill2CastMode = ELSSkillCastMode::PreviewConfirm;

	UPROPERTY(config)
	ELSSkillCastMode Skill3CastMode = ELSSkillCastMode::PreviewConfirm;

	UPROPERTY(config)
	ELSSkillCastMode Skill4CastMode = ELSSkillCastMode::PreviewConfirm;

	UPROPERTY(config)
	ELSSkillCastMode UltimateCastMode = ELSSkillCastMode::PreviewConfirm;
};
