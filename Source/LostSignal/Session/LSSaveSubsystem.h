#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Session/LSSessionSubsystem.h"
#include "LSSaveSubsystem.generated.h"

class ULSSaveGame;

UCLASS()
class LOSTSIGNAL_API ULSSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// 아이템을 스태시에 추가하고 즉시 저장
	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void AddToStash(const TArray<FLSSessionItem>& Items);

	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void ReplaceStash(const TArray<FLSSessionItem>& Items);

	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void ReplaceSafeStash(const TArray<FLSSessionItem>& Items);

	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void SortStash();

	void BeginRaidSave(const TArray<FLSSessionItem>& Loadout);
	void UpdateRaidConsumedItems(const TArray<FLSSessionItem>& ConsumedItems);
	void ClearRaidSave();

	UFUNCTION(BlueprintPure, Category="LS/Save")
	const TArray<FLSSessionItem>& GetStash() const;

	UFUNCTION(BlueprintPure, Category="LS/Save")
	const TArray<FLSSessionItem>& GetSafeStash() const;

private:
	void Load();
	void Save();
	void SaveDebugJson() const;
	void ResolveInterruptedRaid();

	UPROPERTY() TObjectPtr<ULSSaveGame> SaveData;

	static const FString SlotName;
	static const FString DebugFileName;
};
