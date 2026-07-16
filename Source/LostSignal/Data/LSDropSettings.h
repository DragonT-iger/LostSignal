#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/DeveloperSettings.h"
#include "LSDropSettings.generated.h"

UCLASS(config=Game, defaultconfig, meta=(DisplayName="LS Drop Settings"))
class LOSTSIGNAL_API ULSDropSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> RootingObjectTable;

	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> DropTable;

	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> GroupTable;

	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> ChipTable;

	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> WeaponTable;

	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> ArmorTable;

	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> ItemTable;

	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> ChipStatTable;

	// 자판기 판매 목록(DT_StoreStock, Row=FLSStoreStockRow). 상점 UI가 읽는다.
	UPROPERTY(config, EditAnywhere, Category="DataTables")
	TSoftObjectPtr<UDataTable> StoreStockTable;

	ULSDropSettings()
	{
		// 루트박스 단계 공개: 등급이 높을수록 공개 전 대기를 길게 둬 긴장감을 쌓는다.
		// 실제 수치는 기획이 프로젝트 설정에서 조정한다. 여기 값은 초기 기본값일 뿐.
		GradeRevealDelaySeconds.Add(TEXT("Supply"), 0.4f);
		GradeRevealDelaySeconds.Add(TEXT("Standard"), 0.7f);
		GradeRevealDelaySeconds.Add(TEXT("Precision"), 1.2f);
		GradeRevealDelaySeconds.Add(TEXT("Tuning"), 2.0f);
		GradeRevealDelaySeconds.Add(TEXT("Prototype"), 3.0f);
		GradeRevealDelaySeconds.Add(TEXT("Masterpiece"), 4.0f);
	}

	// 등급명(Supply/Standard/Precision/Tuning/Prototype/Masterpiece) → 그 아이템이 뜨기 전 대기(초).
	UPROPERTY(config, EditAnywhere, Category="Drop Reveal")
	TMap<FString, float> GradeRevealDelaySeconds;

	// 등급을 알 수 없거나 매핑이 없는 아이템의 fallback 대기(초).
	UPROPERTY(config, EditAnywhere, Category="Drop Reveal")
	float DefaultRevealDelaySeconds = 0.5f;
};
