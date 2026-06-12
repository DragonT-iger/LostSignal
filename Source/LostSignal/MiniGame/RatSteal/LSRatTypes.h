#pragma once

#include "CoreMinimal.h"
#include "LSRatTypes.generated.h"

/** 작물 종류 (원작 Crops enum) */
UENUM(BlueprintType)
enum class ELSRatCropType : uint8
{
	Eggplant	UMETA(DisplayName = "가지"),
	Potato		UMETA(DisplayName = "감자"),
	Pumpkin		UMETA(DisplayName = "호박"),
	None		UMETA(DisplayName = "없음"),
};

/** 작물 성장 단계 (원작 Size enum). S부터 훔치기 가능 */
UENUM(BlueprintType)
enum class ELSRatCropSize : uint8
{
	Born = 0,
	S = 1,
	M = 2,
	L = 3,
};

/** 밭 랭크. C랭크는 M까지만 성장 */
UENUM(BlueprintType)
enum class ELSRatFarmRank : uint8
{
	RankA = 0,
	RankB = 1,
	RankC = 2,
};

/** 라운드 진행 상태 (01_CoreLoop 상태 모델. 원작 Rage는 미구현이라 미채택) */
UENUM(BlueprintType)
enum class ELSRatPhase : uint8
{
	Ready,
	Playing,
	Paused,
	End,
};

/** 종료 사유 (원작 GameManager::EndReason + TimeUp=Happy) */
UENUM(BlueprintType)
enum class ELSRatEndReason : uint8
{
	None,
	TimeUp			UMETA(DisplayName = "3분 생존"),
	BabyStarved		UMETA(DisplayName = "아기 굶주림"),
	PlayerDead		UMETA(DisplayName = "플레이어 사망"),
};

/** 농부 AI 상태 (원작 FarmerState. Alert는 빈 함수라 미채택) */
UENUM(BlueprintType)
enum class ELSRatFarmerState : uint8
{
	Patrol,
	Chase,
	Attack,
};

/** 인벤토리 슬롯 데이터 (원작 SlotData) */
USTRUCT(BlueprintType)
struct FLSRatSlotData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "LS/RatSteal")
	ELSRatCropType Type = ELSRatCropType::None;

	/** 크기 반영 누적 카운트 (S+1 / M+6 / L+9) */
	UPROPERTY(BlueprintReadOnly, Category = "LS/RatSteal")
	int32 Count = 0;

	bool IsEmpty() const { return Type == ELSRatCropType::None || Count <= 0; }

	void Reset()
	{
		Type = ELSRatCropType::None;
		Count = 0;
	}
};

/** 한 판 결과 (서브시스템 경유로 본편에 전달) */
USTRUCT(BlueprintType)
struct FLSRatResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "LS/RatSteal")
	ELSRatEndReason EndReason = ELSRatEndReason::None;

	UPROPERTY(BlueprintReadOnly, Category = "LS/RatSteal")
	int32 TotalScore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LS/RatSteal")
	int32 EggplantCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LS/RatSteal")
	int32 PotatoCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LS/RatSteal")
	int32 PumpkinCount = 0;

	/** 점수 등급(★ 0~3). 생존 종료에만 부여 (신규, 02_Progression) */
	UPROPERTY(BlueprintReadOnly, Category = "LS/RatSteal")
	int32 Stars = 0;
};

namespace LSRat
{
	/** 종류별 1카운트당 점수 (21_System_Score 단일 출처: 가지25/감자10/호박50) */
	inline int32 GetScorePerCount(ELSRatCropType Type)
	{
		switch (Type)
		{
		case ELSRatCropType::Eggplant: return 25;
		case ELSRatCropType::Potato:   return 10;
		case ELSRatCropType::Pumpkin:  return 50;
		default:                       return 0;
		}
	}

	/** 크기 → 슬롯 카운트 (원작 Slot::AddItem: S+1, 그 외 size*3 → M+6/L+9) */
	inline int32 GetCountForSize(ELSRatCropSize Size)
	{
		return Size == ELSRatCropSize::S ? 1 : static_cast<int32>(Size) * 3;
	}
}
