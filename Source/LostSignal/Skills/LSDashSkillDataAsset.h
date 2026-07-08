#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Skills/LSSkillDataAsset.h"
#include "LSDashSkillDataAsset.generated.h"

/**
 * 대쉬 표시 전용 DataAsset.
 *
 * 대쉬 쿨타임은 스킬 테이블이 아니라 캐릭터 어트리뷰트(예: DashCooldown)를 단일 출처로 쓴다.
 * `CooldownAttribute`에 그 어트리뷰트를 지정하면 스킬 바 표시 총시간이 소유 ASC의 해당 값을 읽는다.
 * 서버 GE(ULSGE_DashCooldown)·로컬 예측도 같은 어트리뷰트를 읽어 셋이 일치한다.
 */
UCLASS(BlueprintType)
class LOSTSIGNAL_API ULSDashSkillDataAsset : public ULSSkillDataAsset
{
	GENERATED_BODY()

public:
	// 쿨타임 총시간을 읽을 캐릭터 어트리뷰트(대쉬 = DashCooldown). 비어 있으면 쿨타임 총시간 0.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Cooldown")
	FGameplayAttribute CooldownAttribute;
};
