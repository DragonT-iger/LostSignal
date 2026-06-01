#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSChipStatRow.generated.h"

// 스탯 범위 한 칸(min~max). CSV/텍스트에서 "10~15" 형식으로 직접 import/export 한다.
// (단일 값 "10"도 허용 → Min=Max=10)
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSStatRange
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stat")
	int32 Min = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stat")
	int32 Max = 0;

	FLSStatRange() = default;
	FLSStatRange(int32 InMin, int32 InMax) : Min(InMin), Max(InMax) {}

	// "Min~Max" 텍스트 한 칸을 파싱한다. DataTable CSV import 시 셀 단위로 호출됨.
	bool ImportTextItem(const TCHAR*& Buffer, int32 /*PortFlags*/, UObject* /*Parent*/, FOutputDevice* /*ErrorText*/)
	{
		FString Cell = Buffer;
		Cell.TrimStartAndEndInline();

		FString MinStr;
		FString MaxStr;
		if (Cell.Split(TEXT("~"), &MinStr, &MaxStr))
		{
			Min = FCString::Atoi(*MinStr.TrimStartAndEnd());
			Max = FCString::Atoi(*MaxStr.TrimStartAndEnd());
		}
		else
		{
			Min = Max = FCString::Atoi(*Cell);
		}

		Buffer += FCString::Strlen(Buffer); // 셀 전체 소비
		return true;
	}

	// 텍스트로 내보낼 때 "Min~Max" 형식 유지 (CSV export 등).
	bool ExportTextItem(FString& ValueStr, const FLSStatRange& /*DefaultValue*/, UObject* /*Parent*/, int32 /*PortFlags*/, UObject* /*ExportRootScope*/) const
	{
		ValueStr += FString::Printf(TEXT("%d~%d"), Min, Max);
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FLSStatRange> : public TStructOpsTypeTraitsBase2<FLSStatRange>
{
	enum
	{
		WithImportTextItem = true,
		WithExportTextItem = true,
	};
};

// 등급별 칩 전투 스탯 범위. 행 Name = 등급명(Supply/Standard/Precision/Tuning/Prototype/Masterpiece).
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSChipStatRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	FLSStatRange Chip_Attack;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	FLSStatRange Chip_Attack_Speed;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	FLSStatRange Chip_Skill_Haste;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	FLSStatRange Chip_Critical_Rate;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	FLSStatRange Chip_Critical_Damage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	FLSStatRange Chip_Defense_Penetration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Defense")
	FLSStatRange Chip_Health;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Defense")
	FLSStatRange Chip_Defense;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Defense")
	FLSStatRange Chip_Recovery;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Survival")
	FLSStatRange Chip_Move_Speed;
};
