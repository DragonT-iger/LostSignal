#pragma once

#include "CoreMinimal.h"
#include "Data/LSCharacterSkillRow.h"
#include "LSSkillTypes.generated.h"

class ULSSkillDataAsset;

USTRUCT(BlueprintType)
struct FLSSkillActivationContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	TObjectPtr<ULSSkillDataAsset> SkillData = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	float AimYaw = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	FLSCharacterSkillRow SkillRow;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	bool bHasSkillRow = false;
};

USTRUCT(BlueprintType)
struct FLSBasicAttackHitContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	TObjectPtr<ULSSkillDataAsset> SkillData = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	int32 ComboIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category="LS/Skill")
	int32 ValidHitCount = 0;
};

// 스킬 선택 슬롯. 4칸(Skill1~4)에 액티브/궁극기를 자유 배치한다(궁극기 전용 칸 없음).
// 일반 공격/패시브/대쉬는 슬롯이 아니라 캐릭터 고정 기본 장착이다.
UENUM(BlueprintType)
enum class ELSPlayerSkillSlot : uint8
{
	Skill1,
	Skill2,
	Skill3,
	Skill4,
	// 대쉬 전용 표시 슬롯. 발동/쿨타임은 기존 대쉬 경로(ULSGA_Dash)가 담당하고, 스킬 바에는 표시만 한다.
	Dash
};

// 스킬 슬롯별 발동 방식. 플레이어가 슬롯마다 지정하며 GameUserSettings.ini에 저장된다.
UENUM(BlueprintType)
enum class ELSSkillCastMode : uint8
{
	// 키를 누르면 프리뷰가 뜨고 마우스 클릭으로 위치를 확정해 발동(기존/기본).
	PreviewConfirm,
	// 키를 누르는 동안 프리뷰 표시, 키를 떼는 순간 커서 위치로 발동.
	QuickCastWithIndicator,
	// 키를 누르는 즉시 커서 위치로 발동(프리뷰/확정 생략).
	QuickCast
};

USTRUCT(BlueprintType)
struct FLSPlayerSkillSlotSpec
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	TObjectPtr<ULSSkillDataAsset> SkillData = nullptr;
};
