#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LSGameDataSubsystem.generated.h"

class UDataTable;
struct FLSComboAttackRow;
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
	const FLSCharacterPassiveSkillRow* FindPassiveSkillRowByID(int32 PassiveSkillID, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	const FLSComboAttackRow* FindComboAttackRow(FName RowName, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	const FLSComboAttackRow* FindComboAttackRowByID(int32 ComboID, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	const FLSComboAttackRow* FindComboAttackRowByIndex(int32 CharacterID, int32 ComboIndex, int32 ComboTag = 0, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	const FLSStatusEffectRow* FindStatusEffectRow(FName RowName, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	const FLSStatusEffectRow* FindStatusEffectRowByID(int32 StatusID, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;

private:
	void LoadTables();
	void NormalizeActiveSkillRows() const;
	void NormalizePassiveSkillRows() const;
	void NormalizeComboAttackRows() const;

	UPROPERTY()
	TObjectPtr<UDataTable> CharacterActiveSkillTable;

	UPROPERTY()
	TObjectPtr<UDataTable> CharacterPassiveSkillTable;

	UPROPERTY()
	TObjectPtr<UDataTable> ComboAttackTable;

	UPROPERTY()
	TObjectPtr<UDataTable> StatusEffectTable;
};
