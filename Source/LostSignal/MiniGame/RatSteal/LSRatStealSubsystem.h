#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MiniGame/RatSteal/LSRatTypes.h"
#include "LSRatStealSubsystem.generated.h"

/**
 * 본편 ↔ RatSteal 미니게임 사이의 유일한 데이터 통로 (31_Flow_EntryReturn).
 * 복귀 맵/위치 보관 + 미니게임 결과 보관. 본편 GAS/세이브/인벤토리에는 접근하지 않는다.
 */
UCLASS()
class LOSTSIGNAL_API ULSRatStealSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** 캐비닛이 호출. 복귀 정보 저장 후 미니게임 레벨로 전환 */
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void EnterMiniGame(const TSoftObjectPtr<UWorld>& MiniGameLevel, APawn* Interactor);

	/** 결과 화면에서 호출. 저장된 본편 맵으로 복귀 */
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void ReturnToMainWorld();

	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void StoreResult(const FLSRatResult& Result);

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	const FLSRatResult& GetLastResult() const { return LastResult; }

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	bool HasPendingReturn() const { return bHasPendingReturn; }

	/** 본편 복귀 후 위치 복원용. 소비(Consume) 시 펜딩 해제 */
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	bool ConsumeReturnTransform(FTransform& OutTransform);

private:
	UPROPERTY()
	FName ReturnMapName = NAME_None;

	UPROPERTY()
	FTransform ReturnTransform = FTransform::Identity;

	UPROPERTY()
	FLSRatResult LastResult;

	bool bHasPendingReturn = false;
};
