#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LSSkillPoolDataAsset.generated.h"

class ULSSkillDataAsset;

// 캐릭터가 로비에서 스킬 슬롯에 고를 수 있는 액티브/궁극기 후보 목록.
// 로비 선택 UI와 런타임 Skill_ID -> ULSSkillDataAsset 해석의 공용 출처다.
// (스킬 타입 Active/Ultimate 판정은 각 후보의 Skill_ID로 DataTable을 조회해서 한다 — 수치/타입은 DataTable이 단일 출처.)
UCLASS(BlueprintType)
class LOSTSIGNAL_API ULSSkillPoolDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// 이 캐릭터가 선택 가능한 스킬 후보. 액티브/궁극기 DataAsset만 등록한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TArray<TObjectPtr<ULSSkillDataAsset>> SelectableSkills;

	// Skill_ID로 후보 DataAsset을 찾는다. 없으면 nullptr.
	UFUNCTION(BlueprintPure, Category="LS/Skill")
	ULSSkillDataAsset* FindSkillByID(int32 SkillID) const;
};
