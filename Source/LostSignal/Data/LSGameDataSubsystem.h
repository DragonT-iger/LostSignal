#pragma once

#include "CoreMinimal.h"
#include "Data/LSProtocolTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LSGameDataSubsystem.generated.h"

class UDataTable;
struct FLSComboAttackRow;
struct FLSCharacterPassiveSkillRow;
struct FLSCharacterSkillRow;
struct FLSConsumableRow;
struct FLSConsumableEffectRow;
struct FLSMonsterActionRow;
struct FLSMonsterArchetypeRow;
struct FLSNoiseProfileRow;
struct FLSProtocolUnlockRow;
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
	const FLSConsumableRow* FindConsumableRow(FName RowName, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	const FLSConsumableEffectRow* FindConsumableEffectRow(FName RowName, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	// 사용 중 회복 미리보기용: 자기 대상 즉발(Once/Flat) 체력 증가량 합. 회복 효과가 없으면 0.
	// 분류 기준은 LSCharacterCombatComponent::ApplyConsumableAttributeEffect의 즉발 Health/Add 경로와 동일하게 맞춘다.
	float GetConsumableSelfInstantHealthRecovery(const FLSConsumableRow& ConsumableRow, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	const FLSMonsterArchetypeRow* FindMonsterArchetypeRow(FName RowName, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	const FLSMonsterActionRow* FindMonsterActionRow(FName RowName, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	const FLSNoiseProfileRow* FindNoiseProfileRow(FName RowName, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	const FLSProtocolUnlockRow* FindProtocolUnlockRow(FName RowName, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	const FLSProtocolUnlockRow* FindProtocolUnlockRowByEnableName(ELSProtocolType ProtocolType, FName EnableName, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	void GetProtocolUnlockRows(ELSProtocolType ProtocolType, TArray<const FLSProtocolUnlockRow*>& OutRows, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	int32 CountProtocolUnlockRows(ELSProtocolType ProtocolType, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	int32 GetMaxProtocolRequiredLevel(ELSProtocolType ProtocolType, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	int32 CountVisibleProtocolUnlockRows(ELSProtocolType ProtocolType, int32 CurrentLevel, int32 PreviousLevel, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	int32 GetVisibleProtocolEnableValueSum(ELSProtocolType ProtocolType, FName EnableName, int32 CurrentLevel, int32 PreviousLevel, const TCHAR* Context = TEXT("LSGameDataSubsystem")) const;
	bool IsProtocolUnlockVisible(const FLSProtocolUnlockRow& Row, int32 CurrentLevel, int32 PreviousLevel, bool* bOutProtected = nullptr) const;

private:
	void LoadTables();
	void LogMissingTables() const;
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

	UPROPERTY()
	TObjectPtr<UDataTable> ConsumableTable;

	UPROPERTY()
	TObjectPtr<UDataTable> ConsumableEffectTable;

	UPROPERTY()
	TObjectPtr<UDataTable> ProtocolUnlockTable;

	UPROPERTY()
	TObjectPtr<UDataTable> MonsterArchetypeTable;

	UPROPERTY()
	TObjectPtr<UDataTable> MonsterActionTable;

	UPROPERTY()
	TObjectPtr<UDataTable> NoiseProfileTable;
};
