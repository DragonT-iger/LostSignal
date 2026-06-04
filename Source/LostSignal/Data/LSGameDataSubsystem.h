#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LSGameDataSubsystem.generated.h"

class UDataTable;
struct FLSCharacterPassiveSkillRow;
struct FLSCharacterSkillRow;
struct FLSStatusEffectRow;

UCLASS()
class LOSTSIGNAL_API ULSGameDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category="LS/Data")
	void ReloadTables();

	const FLSCharacterSkillRow* FindActiveSkillRow(FName RowName, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	const FLSCharacterSkillRow* FindActiveSkillRowByID(int32 SkillID, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	const FLSCharacterPassiveSkillRow* FindPassiveSkillRow(FName RowName, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	const FLSStatusEffectRow* FindStatusEffectRow(FName RowName, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	const FLSStatusEffectRow* FindStatusEffectRowByID(int32 StatusID, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;

private:
	void LoadTables();
	void NormalizeActiveSkillRows() const;

	UPROPERTY()
	TObjectPtr<UDataTable> CharacterActiveSkillTable;

	UPROPERTY()
	TObjectPtr<UDataTable> CharacterPassiveSkillTable;

	UPROPERTY()
	TObjectPtr<UDataTable> StatusEffectTable;
};
