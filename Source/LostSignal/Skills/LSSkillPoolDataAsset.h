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
	// 이 풀이 속한 캐릭터 ID. 세이브의 캐릭터별 스킬 로드아웃 키로 쓴다(로비 UI·런타임 컴포넌트 공용).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	int32 CharacterID = 0;

	// 이 캐릭터가 선택 가능한 스킬 후보. 액티브/궁극기 DataAsset만 등록한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TArray<TObjectPtr<ULSSkillDataAsset>> SelectableSkills;

	// 최초 진입 시 스킬 슬롯 3칸에 기본으로 채울 액티브/궁극기 Skill_ID. 순서 = Skill1/2/3, 최대 3개.
	// SelectableSkills 안에 있는 스킬이어야 로비/런타임에서 아이콘·발동까지 정상 해석된다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TArray<int32> DefaultEquippedSkillIDs;

	// Skill_ID로 후보 DataAsset을 찾는다. 없으면 nullptr.
	UFUNCTION(BlueprintPure, Category="LS/Skill")
	ULSSkillDataAsset* FindSkillByID(int32 SkillID) const;
};
