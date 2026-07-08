#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Session/LSSessionSubsystem.h"
#include "LSSaveGame.generated.h"

// 캐릭터 한 명의 스킬 선택 로드아웃. 세이브에 CharacterID로 키잉해 저장한다.
USTRUCT()
struct FLSSkillLoadout
{
	GENERATED_BODY()

	// 스킬 선택 슬롯 3칸(인덱스 = Skill1/Skill2/Skill3, 값 = 액티브/궁극기 Skill_ID, 0 = 빈 칸).
	UPROPERTY() TArray<int32> SkillIDs;

	// 기본 로드아웃을 1회 시딩했는지. true면 사용자가 슬롯을 다 비워도 기본값을 다시 채우지 않는다.
	UPROPERTY() bool bInitialized = false;
};

UCLASS()
class LOSTSIGNAL_API ULSSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// Legacy migration-only. New code uses Inventory/WarehouseItems/SafeStash.
	UPROPERTY() TArray<FLSSessionItem> Stash;
	UPROPERTY() TArray<FLSSessionItem> Player1Inventory;
	UPROPERTY() bool bInventoryMigrated = false;

	// Slot-based storage. Duplicate ItemRowName entries are valid when an item exceeds Item_Max.
	UPROPERTY() TArray<FLSSessionItem> Inventory;
	UPROPERTY() TArray<FLSSessionItem> WarehouseItems;
	UPROPERTY() TArray<FLSSessionItem> SafeStash;
	UPROPERTY() TArray<FLSSessionItem> ChipEquipmentSlots;
	UPROPERTY() float ChipSignalGaugePercent = 1.0f;

	// 무기/방어구 장착 5칸 (ELSEquipmentSlot 순서: Weapon/Processor/Core/Actuator/Frame). 로비 전용.
	UPROPERTY() TArray<FLSSessionItem> EquipmentSlots;

	// 캐릭터별 스킬 선택 로드아웃. 키 = CharacterID(ULSSkillPoolDataAsset::CharacterID). 로비에서 선택.
	UPROPERTY() TMap<int32, FLSSkillLoadout> SkillLoadoutsByCharacter;

	UPROPERTY() bool bRaidSaveActive = false;
	UPROPERTY() TArray<FLSSessionItem> ActiveRaidLoadout;
	UPROPERTY() TArray<FLSSessionItem> ActiveRaidConsumedItems;
};
