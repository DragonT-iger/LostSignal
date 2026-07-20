#include "Session/LSSaveSubsystem.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSChipStats.h"
#include "Data/LSDropSettings.h"
#include "Session/LSSaveGame.h"
#include "Session/LSSaveSettings.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Policies\PrettyJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"

constexpr int32 SaveDefaultMaxInventorySlotCount = 10;
constexpr int32 ChipEquipmentSlotCount = 10;
// 무기/방어구 장착칸 수는 공용 상수 LSInventorySlotUtils::EquipmentSlotCount를 쓴다.
constexpr int32 EquipmentSlotCount = LSInventorySlotUtils::EquipmentSlotCount;
constexpr int32 EquippedSkillSlotCount = 4;

const FString ULSSaveSubsystem::SlotName = TEXT("LostSignalSave");
const FString ULSSaveSubsystem::DebugFileName = TEXT("LostSignalSave_Debug.json");

void ULSSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Load();
}

void ULSSaveSubsystem::AddToInventory(const TArray<FLSSessionItem>& Items)
{
	if (!SaveData || Items.IsEmpty())
	{
		return;
	}

	TArray<FLSSessionItem>& Inv = GetMutableInventory();
	for (const FLSSessionItem& NewItem : Items)
	{
		FLSSessionItem IgnoredRemainingItem;
		LSInventorySlotUtils::TryAddItemsToSlotArray(Inv, NewItem.ItemRowName, NewItem.Amount, GetMaxInventorySlotCount(), NewItem.ChipStats, IgnoredRemainingItem);
	}

	UE_LOG(LogLS, Log, TEXT("[Save] Inventory updated: added %d entries, total slots %d"), Items.Num(), Inv.Num());
	Save();
}

bool ULSSaveSubsystem::TryAddToInventory(const FName ItemRowName, const int32 Amount, const TArray<FLSChipResolvedStat>& ChipStats, FLSSessionItem& OutRemainingItem)
{
	OutRemainingItem = FLSSessionItem();
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot add inventory item because SaveData is missing."));
		return false;
	}

	const bool bChanged = LSInventorySlotUtils::TryAddItemsToSlotArray(GetMutableInventory(), ItemRowName, Amount, GetMaxInventorySlotCount(), ChipStats, OutRemainingItem);
	if (bChanged)
	{
		Save();
	}
	return bChanged;
}

void ULSSaveSubsystem::ReplaceInventory(const TArray<FLSSessionItem>& Items)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace inventory because SaveData is missing."));
		return;
	}

	SaveData->Inventory = Items;
	LSInventorySlotUtils::NormalizeSlotArray(SaveData->Inventory);

	UE_LOG(LogLS, Log, TEXT("[Save] Inventory replaced. Total slots: %d"), SaveData->Inventory.Num());
	Save();
}

void ULSSaveSubsystem::ReplaceWarehouseItems(const TArray<FLSSessionItem>& Items)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace warehouse because SaveData is missing."));
		return;
	}

	SaveData->WarehouseItems = Items;
	LSInventorySlotUtils::NormalizeSlotArray(SaveData->WarehouseItems);
	if (HasWarehouseOverflow())
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Warehouse replacement contains slots beyond capacity. Items are preserved, but new warehouse entries are blocked. Slots=%d Max=%d"),
			SaveData->WarehouseItems.Num(), GetMaxWarehouseSlotCount());
	}

	UE_LOG(LogLS, Log, TEXT("[Save] Warehouse replaced. Total slots: %d"), SaveData->WarehouseItems.Num());
	Save();
}

void ULSSaveSubsystem::ReplaceSafeStash(const TArray<FLSSessionItem>& Items)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace safe stash because SaveData is missing."));
		return;
	}

	SaveData->SafeStash = Items;
	LSInventorySlotUtils::NormalizeSlotArray(SaveData->SafeStash);

	UE_LOG(LogLS, Log, TEXT("[Save] Safe stash replaced. Total slots: %d"), SaveData->SafeStash.Num());
	Save();
}

void ULSSaveSubsystem::ReplaceEquipmentSlots(const TArray<FLSSessionItem>& Items)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace equipment slots because SaveData is missing."));
		return;
	}

	// 장비 배열은 인덱스=슬롯타입 불변식이라 Normalize/Sort/Compact 금지. SetNum(5) 패딩만 한다.
	SaveData->EquipmentSlots = Items;
	EnsureEquipmentSlots();

	UE_LOG(LogLS, Log, TEXT("[Save] Equipment slots replaced. Total slots: %d"), SaveData->EquipmentSlots.Num());
	Save();
	OnEquipmentChanged.Broadcast();
}

const TArray<FLSSessionItem>& ULSSaveSubsystem::GetInventory() const
{
	static TArray<FLSSessionItem> Empty;
	return SaveData ? SaveData->Inventory : Empty;
}

const TArray<FLSSessionItem>& ULSSaveSubsystem::GetWarehouseItems() const
{
	static TArray<FLSSessionItem> Empty;
	return SaveData ? SaveData->WarehouseItems : Empty;
}

const TArray<FLSSessionItem>& ULSSaveSubsystem::GetSafeStash() const
{
	static TArray<FLSSessionItem> Empty;
	return SaveData ? SaveData->SafeStash : Empty;
}

const TArray<FLSSessionItem>& ULSSaveSubsystem::GetChipEquipmentSlots() const
{
	static TArray<FLSSessionItem> Empty;
	return SaveData ? SaveData->ChipEquipmentSlots : Empty;
}

const TArray<int32>& ULSSaveSubsystem::GetEquippedSkillIDs(const int32 CharacterID) const
{
	static const TArray<int32> Empty;
	if (!SaveData)
	{
		return Empty;
	}
	const FLSSkillLoadout* Loadout = SaveData->SkillLoadoutsByCharacter.Find(CharacterID);
	return Loadout ? Loadout->SkillIDs : Empty;
}

bool ULSSaveSubsystem::SetEquippedSkillSlot(const int32 CharacterID, const int32 SlotIndex, const int32 SkillID)
{
	if (!SaveData)
	{
		return false;
	}

	if (SlotIndex < 0 || SlotIndex >= EquippedSkillSlotCount)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] SetEquippedSkillSlot invalid slot index %d"), SlotIndex);
		return false;
	}

	// 빈 칸(0) 요청은 해제로 처리한다.
	if (SkillID == 0)
	{
		return ClearEquippedSkillSlot(CharacterID, SlotIndex);
	}

	FLSSkillLoadout& Loadout = EnsureSkillLoadout(CharacterID);

	// 같은 스킬이 다른 칸에 이미 있으면 그 칸을 비워 중복 장착을 막는다(이동).
	for (int32 Index = 0; Index < EquippedSkillSlotCount; ++Index)
	{
		if (Index != SlotIndex && Loadout.SkillIDs[Index] == SkillID)
		{
			Loadout.SkillIDs[Index] = 0;
		}
	}

	Loadout.SkillIDs[SlotIndex] = SkillID;
	Save();
	OnSkillLoadoutChanged.Broadcast();
	return true;
}

bool ULSSaveSubsystem::ClearEquippedSkillSlot(const int32 CharacterID, const int32 SlotIndex)
{
	if (!SaveData)
	{
		return false;
	}

	if (SlotIndex < 0 || SlotIndex >= EquippedSkillSlotCount)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] ClearEquippedSkillSlot invalid slot index %d"), SlotIndex);
		return false;
	}

	FLSSkillLoadout& Loadout = EnsureSkillLoadout(CharacterID);
	if (Loadout.SkillIDs[SlotIndex] == 0)
	{
		return false;
	}

	Loadout.SkillIDs[SlotIndex] = 0;
	Save();
	OnSkillLoadoutChanged.Broadcast();
	return true;
}

bool ULSSaveSubsystem::TrySeedDefaultSkillLoadout(const int32 CharacterID, const TArray<int32>& DefaultSkillIDs)
{
	if (!SaveData)
	{
		return false;
	}

	FLSSkillLoadout& Loadout = EnsureSkillLoadout(CharacterID);

	// 최초 1회만 시딩한다. 이후엔 사용자가 슬롯을 다 비워도 기본값을 다시 채우지 않는다.
	if (Loadout.bInitialized)
	{
		return false;
	}

	// DefaultSkillIDs를 앞 칸부터 채운다. 0/중복은 건너뛴다. (타입 검증은 로비 UI가 담당 — 여기선 타입 무관.)
	int32 SeededCount = 0;
	int32 SlotIndex = 0;
	for (const int32 SkillID : DefaultSkillIDs)
	{
		if (SlotIndex >= EquippedSkillSlotCount)
		{
			break;
		}
		if (SkillID == 0 || Loadout.SkillIDs.Contains(SkillID))
		{
			continue;
		}
		Loadout.SkillIDs[SlotIndex] = SkillID;
		++SlotIndex;
		++SeededCount;
	}

	// 실제로 채운 게 없으면(DA의 DefaultEquippedSkillIDs 미설정 등) 초기화 완료로 치지 않는다.
	// 그래야 DA를 채운 뒤 다시 열었을 때 시딩된다(빈 값으로 플래그가 latch되는 함정 방지).
	if (SeededCount == 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] TrySeedDefaultSkillLoadout(char %d): 시딩할 기본 스킬이 없음. DA의 DefaultEquippedSkillIDs를 확인하세요."), CharacterID);
		return false;
	}

	Loadout.bInitialized = true;
	Save();
	OnSkillLoadoutChanged.Broadcast();
	UE_LOG(LogLS, Log, TEXT("[Save] TrySeedDefaultSkillLoadout(char %d): 기본 스킬 %d개 시딩"), CharacterID, SeededCount);
	return true;
}

FLSSkillLoadout& ULSSaveSubsystem::EnsureSkillLoadout(const int32 CharacterID)
{
	// 호출부가 SaveData 유효성을 보장한다.
	FLSSkillLoadout& Loadout = SaveData->SkillLoadoutsByCharacter.FindOrAdd(CharacterID);

	while (Loadout.SkillIDs.Num() < EquippedSkillSlotCount)
	{
		Loadout.SkillIDs.Add(0);
	}
	if (Loadout.SkillIDs.Num() > EquippedSkillSlotCount)
	{
		Loadout.SkillIDs.SetNum(EquippedSkillSlotCount);
	}
	return Loadout;
}

const TArray<FLSSessionItem>& ULSSaveSubsystem::GetEquipmentSlots() const
{
	static TArray<FLSSessionItem> Empty;
	return SaveData ? SaveData->EquipmentSlots : Empty;
}

int32 ULSSaveSubsystem::GetMaxInventorySlotCount() const
{
	return SaveDefaultMaxInventorySlotCount + GetCarryingProtocolSlotBonus(TEXT("Inventory"));
}

int32 ULSSaveSubsystem::GetMaxSafeStashSlotCount() const
{
	constexpr int32 SaveMaxSafeStashSlotCount = 4;
	return FMath::Clamp(GetCarryingProtocolSlotBonus(TEXT("Protected_Inventory")), 0, SaveMaxSafeStashSlotCount);
}

int32 ULSSaveSubsystem::GetMaxWarehouseSlotCount() const
{
	return FMath::Max(1, GetDefault<ULSSaveSettings>()->MaxWarehouseSlotCount);
}

bool ULSSaveSubsystem::HasWarehouseOverflow() const
{
	if (!SaveData)
	{
		return false;
	}

	for (int32 SlotIndex = GetMaxWarehouseSlotCount(); SlotIndex < SaveData->WarehouseItems.Num(); ++SlotIndex)
	{
		if (LSInventorySlotUtils::IsFilled(SaveData->WarehouseItems[SlotIndex]))
		{
			return true;
		}
	}
	return false;
}

float ULSSaveSubsystem::GetChipSignalGaugePercent() const
{
	return SaveData ? FMath::Clamp(SaveData->ChipSignalGaugePercent, 0.0f, 1.0f) : 1.0f;
}

void ULSSaveSubsystem::SetChipSignalGaugePercent(const float Percent)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot set chip signal gauge because SaveData is missing."));
		return;
	}

	const float ClampedPercent = FMath::Clamp(Percent, 0.0f, 1.0f);
	if (FMath::IsNearlyEqual(SaveData->ChipSignalGaugePercent, ClampedPercent))
	{
		return;
	}

	SaveData->ChipSignalGaugePercent = ClampedPercent;
	Save();
	OnChipLoadoutChanged.Broadcast();
}

bool ULSSaveSubsystem::EquipChipFromStoredSlot(const ELSInventorySlotArea SourceArea, const int32 SourceIndex, const int32 EquipmentIndex)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot equip chip because SaveData is missing."));
		return false;
	}

	if (EquipmentIndex < 0 || EquipmentIndex >= ChipEquipmentSlotCount)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot equip chip because equipment index is invalid. Index=%d"), EquipmentIndex);
		return false;
	}

	TArray<FLSSessionItem>* SourceSlots = GetMutableStoredSlots(SourceArea);
	if (!SourceSlots || !SourceSlots->IsValidIndex(SourceIndex) || !LSInventorySlotUtils::IsFilled((*SourceSlots)[SourceIndex]))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot equip chip because source slot is invalid. Area=%d Index=%d"),
			static_cast<int32>(SourceArea),
			SourceIndex);
		return false;
	}

	FLSSessionItem& SourceSlot = (*SourceSlots)[SourceIndex];
	if (!SourceSlot.ItemRowName.ToString().StartsWith(TEXT("Chip_")))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot equip non-chip item '%s'."), *SourceSlot.ItemRowName.ToString());
		return false;
	}

	EnsureChipEquipmentSlots();
	FLSSessionItem& EquipmentSlot = SaveData->ChipEquipmentSlots[EquipmentIndex];
	if (LSInventorySlotUtils::IsFilled(EquipmentSlot) && !EquipmentSlot.ItemRowName.ToString().StartsWith(TEXT("Chip_")))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot swap equipment slot %d because it contains non-chip item '%s'."),
			EquipmentIndex,
			*EquipmentSlot.ItemRowName.ToString());
		return false;
	}

	if (LSInventorySlotUtils::IsFilled(EquipmentSlot))
	{
		Swap(SourceSlot, EquipmentSlot);
	}
	else
	{
		EquipmentSlot = SourceSlot;
		SourceSlot = LSInventorySlotUtils::MakeEmptyItem();
	}

	Save();
	OnChipLoadoutChanged.Broadcast();
	return true;
}

bool ULSSaveSubsystem::DropChipEquipmentSlot(const int32 FromEquipmentIndex, const int32 ToEquipmentIndex)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop chip equipment slot because SaveData is missing."));
		return false;
	}

	if (FromEquipmentIndex < 0 || FromEquipmentIndex >= ChipEquipmentSlotCount || ToEquipmentIndex < 0 || ToEquipmentIndex >= ChipEquipmentSlotCount)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop chip equipment slot because index is invalid. From=%d To=%d"),
			FromEquipmentIndex,
			ToEquipmentIndex);
		return false;
	}

	EnsureChipEquipmentSlots();
	const FLSSessionItem& FromSlot = SaveData->ChipEquipmentSlots[FromEquipmentIndex];
	if (!LSInventorySlotUtils::IsFilled(FromSlot))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop chip equipment slot because source is empty. From=%d To=%d"),
			FromEquipmentIndex,
			ToEquipmentIndex);
		return false;
	}

	if (!FromSlot.ItemRowName.ToString().StartsWith(TEXT("Chip_")))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop non-chip equipment item '%s'."), *FromSlot.ItemRowName.ToString());
		return false;
	}

	const FLSSessionItem& ToSlot = SaveData->ChipEquipmentSlots[ToEquipmentIndex];
	if (LSInventorySlotUtils::IsFilled(ToSlot) && !ToSlot.ItemRowName.ToString().StartsWith(TEXT("Chip_")))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop to equipment slot %d because it contains non-chip item '%s'."),
			ToEquipmentIndex,
			*ToSlot.ItemRowName.ToString());
		return false;
	}

	const bool bDropped = LSInventorySlotUtils::DropSlot(
		SaveData->ChipEquipmentSlots,
		FromEquipmentIndex,
		SaveData->ChipEquipmentSlots,
		ToEquipmentIndex,
		ChipEquipmentSlotCount);
	if (bDropped)
	{
		Save();
		OnChipLoadoutChanged.Broadcast();
	}
	return bDropped;
}

bool ULSSaveSubsystem::UnequipChipToWarehouse(const int32 EquipmentIndex)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot unequip chip because SaveData is missing."));
		return false;
	}

	if (EquipmentIndex < 0 || EquipmentIndex >= ChipEquipmentSlotCount)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot unequip chip because equipment index is invalid. Index=%d"), EquipmentIndex);
		return false;
	}

	EnsureChipEquipmentSlots();
	FLSSessionItem& EquipmentSlot = SaveData->ChipEquipmentSlots[EquipmentIndex];
	if (!LSInventorySlotUtils::IsFilled(EquipmentSlot))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot unequip chip because equipment slot is empty. Index=%d"), EquipmentIndex);
		return false;
	}

	if (!EquipmentSlot.ItemRowName.ToString().StartsWith(TEXT("Chip_")))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot unequip non-chip item '%s'."), *EquipmentSlot.ItemRowName.ToString());
		return false;
	}

	if (HasWarehouseOverflow())
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot unequip chip because warehouse contains overflow items. Index=%d"), EquipmentIndex);
		return false;
	}

	FLSSessionItem RemainingItem;
	const bool bMoved = LSInventorySlotUtils::TryAddItemsToSlotArray(
		SaveData->WarehouseItems,
		EquipmentSlot.ItemRowName,
		EquipmentSlot.Amount,
		GetMaxWarehouseSlotCount(),
		EquipmentSlot.ChipStats,
		RemainingItem);
	if (!bMoved || LSInventorySlotUtils::IsFilled(RemainingItem))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot unequip chip because warehouse add failed. Index=%d Row=%s"),
			EquipmentIndex,
			*EquipmentSlot.ItemRowName.ToString());
		return false;
	}

	EquipmentSlot = LSInventorySlotUtils::MakeEmptyItem();
	Save();
	OnChipLoadoutChanged.Broadcast();
	return true;
}

bool ULSSaveSubsystem::MoveEquipmentSlot(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot move equipment slot because SaveData is missing."));
		return false;
	}

	const bool bFromEquipment = FromArea == ELSInventorySlotArea::Equipment;
	const bool bToEquipment = ToArea == ELSInventorySlotArea::Equipment;

	EnsureEquipmentSlots();

	TArray<FLSSessionItem>* FromSlots = GetMutableStoredSlots(FromArea);
	TArray<FLSSessionItem>* ToSlots = GetMutableStoredSlots(ToArea);
	if (!FromSlots || !ToSlots)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot move equipment slot. FromArea=%d From=%d ToArea=%d To=%d"),
			static_cast<int32>(FromArea), FromIndex, static_cast<int32>(ToArea), ToIndex);
		return false;
	}

	// 잠긴 보호 슬롯은 장비 이동의 원본/대상으로 쓸 수 없다.
	if (FromArea == ELSInventorySlotArea::Safe && FromIndex >= GetMaxSafeStashSlotCount())
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot move equipment because source safe slot is locked. Index=%d"), FromIndex);
		return false;
	}
	if (ToArea == ELSInventorySlotArea::Safe && ToIndex >= GetMaxSafeStashSlotCount())
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot move equipment because target safe slot is locked. Index=%d"), ToIndex);
		return false;
	}

	if (ToArea == ELSInventorySlotArea::Warehouse && FromArea != ELSInventorySlotArea::Warehouse && HasWarehouseOverflow())
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot move equipment because warehouse contains overflow items."));
		return false;
	}

	const int32 ToMaxSlotCount =
		ToArea == ELSInventorySlotArea::Inventory ? GetMaxInventorySlotCount() :
		ToArea == ELSInventorySlotArea::Safe ? GetMaxSafeStashSlotCount() :
		ToArea == ELSInventorySlotArea::Warehouse ? GetMaxWarehouseSlotCount() :
		EquipmentSlotCount;

	// 영역 쌍/타입 검증과 배치/스왑은 레이드 세션과 공유하는 공용 코어가 처리한다.
	const bool bMoved = LSInventorySlotUtils::MoveEquipmentSlotBetweenArrays(*FromSlots, FromIndex, bFromEquipment, *ToSlots, ToIndex, bToEquipment, ToMaxSlotCount);
	if (bMoved)
	{
		Save();
		// 장착 구성이 바뀌었으니 장비 전투 스탯을 다시 적용하도록 알린다.
		OnEquipmentChanged.Broadcast();
	}
	return bMoved;
}

void ULSSaveSubsystem::SortInventory()
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot sort inventory because SaveData is missing."));
		return;
	}

	LSInventorySlotUtils::SortAndCompactSlotArray(SaveData->Inventory);
	UE_LOG(LogLS, Log, TEXT("[Save] Inventory sorted and compacted. Total slots: %d"), SaveData->Inventory.Num());
	Save();
}

void ULSSaveSubsystem::SortWarehouse()
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot sort warehouse because SaveData is missing."));
		return;
	}

	LSInventorySlotUtils::SortAndCompactSlotArray(SaveData->WarehouseItems);
	UE_LOG(LogLS, Log, TEXT("[Save] Warehouse sorted and compacted. Total slots: %d"), SaveData->WarehouseItems.Num());
	Save();
}

bool ULSSaveSubsystem::DropStoredSlot(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop stored slot because SaveData is missing."));
		return false;
	}

	TArray<FLSSessionItem>* FromSlots = GetMutableStoredSlots(FromArea);
	TArray<FLSSessionItem>* ToSlots = GetMutableStoredSlots(ToArea);
	if (!FromSlots || !ToSlots || !FromSlots->IsValidIndex(FromIndex) || !LSInventorySlotUtils::IsFilled((*FromSlots)[FromIndex]) || ToIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop stored slot. FromArea=%d From=%d ToArea=%d To=%d"),
			static_cast<int32>(FromArea), FromIndex, static_cast<int32>(ToArea), ToIndex);
		return false;
	}

	if (FromSlots == ToSlots && FromIndex == ToIndex)
	{
		return true;
	}

	if (FromArea == ELSInventorySlotArea::Safe && FromIndex >= GetMaxSafeStashSlotCount())
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop stored slot because source safe slot is locked. Index=%d"), FromIndex);
		return false;
	}

	if (ToArea == ELSInventorySlotArea::Warehouse && FromArea != ELSInventorySlotArea::Warehouse && HasWarehouseOverflow())
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop stored slot because warehouse contains overflow items."));
		return false;
	}

	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory
		? GetMaxInventorySlotCount()
		: (ToArea == ELSInventorySlotArea::Safe
			? GetMaxSafeStashSlotCount()
			: (ToArea == ELSInventorySlotArea::Warehouse ? GetMaxWarehouseSlotCount() : INDEX_NONE));
	const bool bChanged = LSInventorySlotUtils::DropSlot(*FromSlots, FromIndex, *ToSlots, ToIndex, ToMaxSlotCount);
	if (bChanged)
	{
		Save();
	}
	return bChanged;
}

bool ULSSaveSubsystem::TransferStoredSlotToArea(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot transfer stored slot because SaveData is missing."));
		return false;
	}

	TArray<FLSSessionItem>* FromSlots = GetMutableStoredSlots(FromArea);
	TArray<FLSSessionItem>* ToSlots = GetMutableStoredSlots(ToArea);
	if (!FromSlots || !ToSlots || !FromSlots->IsValidIndex(FromIndex) || !LSInventorySlotUtils::IsFilled((*FromSlots)[FromIndex]))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot transfer stored slot. FromArea=%d From=%d ToArea=%d"),
			static_cast<int32>(FromArea), FromIndex, static_cast<int32>(ToArea));
		return false;
	}

	if (FromSlots == ToSlots)
	{
		return false;
	}

	if (FromArea == ELSInventorySlotArea::Safe && FromIndex >= GetMaxSafeStashSlotCount())
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot transfer stored slot because source safe slot is locked. Index=%d"), FromIndex);
		return false;
	}

	if (ToArea == ELSInventorySlotArea::Warehouse && HasWarehouseOverflow())
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot transfer stored slot because warehouse contains overflow items."));
		return false;
	}

	FLSSessionItem& FromSlot = (*FromSlots)[FromIndex];
	FLSSessionItem RemainingItem;
	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory
		? GetMaxInventorySlotCount()
		: (ToArea == ELSInventorySlotArea::Safe
			? GetMaxSafeStashSlotCount()
			: (ToArea == ELSInventorySlotArea::Warehouse ? GetMaxWarehouseSlotCount() : MAX_int32));
	if (!LSInventorySlotUtils::TryAddItemsToSlotArray(*ToSlots, FromSlot.ItemRowName, FromSlot.Amount, ToMaxSlotCount, FromSlot.ChipStats, RemainingItem))
	{
		return false;
	}

	FromSlot = RemainingItem;
	Save();
	return true;
}

bool ULSSaveSubsystem::TransferAllInventoryToWarehouse(bool& bOutStoppedBecauseFull)
{
	bOutStoppedBecauseFull = false;
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot store all inventory items because SaveData is missing."));
		return false;
	}

	if (HasWarehouseOverflow())
	{
		bOutStoppedBecauseFull = true;
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot store all inventory items because warehouse contains overflow items."));
		return false;
	}

	const int32 WarehouseMaxSlotCount = GetMaxWarehouseSlotCount();

	bool bChanged = false;
	for (int32 InventoryIndex = 0; InventoryIndex < SaveData->Inventory.Num(); ++InventoryIndex)
	{
		FLSSessionItem& InventorySlot = SaveData->Inventory[InventoryIndex];
		if (!LSInventorySlotUtils::IsFilled(InventorySlot))
		{
			continue;
		}

		FLSSessionItem RemainingItem;
		const bool bAddedAny = LSInventorySlotUtils::TryAddItemsToSlotArray(
			SaveData->WarehouseItems,
			InventorySlot.ItemRowName,
			InventorySlot.Amount,
			WarehouseMaxSlotCount,
			InventorySlot.ChipStats,
			RemainingItem);

		if (!bAddedAny)
		{
			bOutStoppedBecauseFull = true;
			UE_LOG(LogLS, Warning, TEXT("[Save] Warehouse is full while storing all inventory items. InventoryIndex=%d Row=%s Amount=%d"),
				InventoryIndex,
				*InventorySlot.ItemRowName.ToString(),
				InventorySlot.Amount);
			break;
		}

		bChanged = true;
		InventorySlot = RemainingItem;
		if (LSInventorySlotUtils::IsFilled(RemainingItem))
		{
			bOutStoppedBecauseFull = true;
			UE_LOG(LogLS, Warning, TEXT("[Save] Warehouse became full while storing all inventory items. InventoryIndex=%d Row=%s RemainingAmount=%d"),
				InventoryIndex,
				*RemainingItem.ItemRowName.ToString(),
				RemainingItem.Amount);
			break;
		}
	}

	if (bChanged)
	{
		Save();
	}

	return bChanged;
}

bool ULSSaveSubsystem::DropExternalItemToStoredSlot(FLSSessionItem& InOutExternalItem, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	// 창고는 룻박스 직접 적재 대상이 아니다(인벤토리/금고만 허용).
	if (ToArea == ELSInventorySlotArea::Warehouse)
	{
		return false;
	}

	TArray<FLSSessionItem>* ToSlots = GetMutableStoredSlots(ToArea);
	if (!ToSlots)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot drop external item because area is invalid. Area=%d Index=%d"),
			static_cast<int32>(ToArea), ToIndex);
		return false;
	}

	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory
		? GetMaxInventorySlotCount()
		: GetMaxSafeStashSlotCount();
	const bool bChanged = LSInventorySlotUtils::DropExternalItemToSlot(InOutExternalItem, *ToSlots, ToIndex, ToMaxSlotCount);
	if (bChanged)
	{
		Save();
	}
	return bChanged;
}

bool ULSSaveSubsystem::GetStoredSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, FLSSessionItem& OutItem) const
{
	OutItem = FLSSessionItem();
	const TArray<FLSSessionItem>* Slots = GetStoredSlots(SlotArea);
	if (!Slots || !Slots->IsValidIndex(SlotIndex) || !LSInventorySlotUtils::IsFilled((*Slots)[SlotIndex]))
	{
		return false;
	}

	OutItem = (*Slots)[SlotIndex];
	return true;
}

bool ULSSaveSubsystem::ClearStoredSlot(const ELSInventorySlotArea SlotArea, const int32 SlotIndex)
{
	TArray<FLSSessionItem>* Slots = GetMutableStoredSlots(SlotArea);
	if (!Slots || !Slots->IsValidIndex(SlotIndex) || !LSInventorySlotUtils::IsFilled((*Slots)[SlotIndex]))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot clear stored slot. Area=%d Index=%d"), static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	(*Slots)[SlotIndex] = LSInventorySlotUtils::MakeEmptyItem();
	Save();
	return true;
}

bool ULSSaveSubsystem::ReplaceStoredSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, const FLSSessionItem& NewItem, FLSSessionItem& OutPreviousItem)
{
	OutPreviousItem = FLSSessionItem();
	if (SlotIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace stored slot because index is invalid. Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	TArray<FLSSessionItem>* Slots = GetMutableStoredSlots(SlotArea);
	if (!Slots)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace stored slot because area is invalid. Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	const int32 MaxSlotCount = SlotArea == ELSInventorySlotArea::Inventory
		? GetMaxInventorySlotCount()
		: (SlotArea == ELSInventorySlotArea::Safe
			? GetMaxSafeStashSlotCount()
			: (SlotArea == ELSInventorySlotArea::Warehouse ? GetMaxWarehouseSlotCount() : INDEX_NONE));
	if (SlotArea == ELSInventorySlotArea::Warehouse && LSInventorySlotUtils::IsFilled(NewItem) && HasWarehouseOverflow())
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace warehouse slot because warehouse contains overflow items. Index=%d"), SlotIndex);
		return false;
	}
	if (MaxSlotCount != INDEX_NONE && SlotIndex >= MaxSlotCount)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot replace stored slot because index exceeds max. Area=%d Index=%d Max=%d"),
			static_cast<int32>(SlotArea), SlotIndex, MaxSlotCount);
		return false;
	}

	LSInventorySlotUtils::EnsureSlotIndex(*Slots, SlotIndex);
	OutPreviousItem = (*Slots)[SlotIndex];
	(*Slots)[SlotIndex] = NewItem;
	Save();
	return true;
}

void ULSSaveSubsystem::BeginRaidSave(const TArray<FLSSessionItem>& Loadout)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot begin raid save because SaveData is missing."));
		return;
	}

	SaveData->bRaidSaveActive = true;
	SaveData->ActiveRaidLoadout = Loadout;
	SaveData->ActiveRaidConsumedItems.Reset();
	Save();
}

void ULSSaveSubsystem::UpdateRaidConsumedItems(const TArray<FLSSessionItem>& ConsumedItems)
{
	if (!SaveData || !SaveData->bRaidSaveActive)
	{
		return;
	}

	SaveData->ActiveRaidConsumedItems = ConsumedItems;
	Save();
}

bool ULSSaveSubsystem::IsRaidSaveActive() const
{
	return SaveData && SaveData->bRaidSaveActive;
}

void ULSSaveSubsystem::ClearRaidSave()
{
	if (!SaveData)
	{
		return;
	}

	SaveData->bRaidSaveActive = false;
	SaveData->ActiveRaidLoadout.Reset();
	SaveData->ActiveRaidConsumedItems.Reset();
	Save();
}

FString ULSSaveSubsystem::GetResolvedSlotName() const
{
#if WITH_EDITOR
	const UGameInstance* GameInstance = GetGameInstance();
	const UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	const UPackage* WorldPackage = World ? World->GetPackage() : nullptr;
	const int32 PIEInstanceId = WorldPackage ? WorldPackage->GetPIEInstanceID() : INDEX_NONE;
	if (PIEInstanceId != INDEX_NONE)
	{
		return FString::Printf(TEXT("%s_PIE_%d"), *SlotName, PIEInstanceId);
	}
#endif

	return SlotName;
}

FString ULSSaveSubsystem::GetResolvedDebugFileName() const
{
	const FString ResolvedSlotName = GetResolvedSlotName();
	if (ResolvedSlotName == SlotName)
	{
		return DebugFileName;
	}

	return FString::Printf(TEXT("%s_Debug.json"), *ResolvedSlotName);
}

bool ULSSaveSubsystem::HasExistingSave() const
{
	const FString ResolvedSlotName = GetResolvedSlotName();
	return UGameplayStatics::DoesSaveGameExist(ResolvedSlotName, 0)
		|| UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

void ULSSaveSubsystem::StartNewGame()
{
	// 기존 세이브를 모두 지우고 빈 데이터로 새로 시작한다. 즉시 재저장하지 않으므로
	// 이후 게임 플레이가 저장을 일으키기 전까지는 세이브 파일이 없는 상태가 된다.
	DeleteAllSaveFiles();

	SaveData = Cast<ULSSaveGame>(UGameplayStatics::CreateSaveGameObject(ULSSaveGame::StaticClass()));
	EnsureChipEquipmentSlots();
	EnsureEquipmentSlots();
	EnsureGoldInitialized();
	ApplyStarterItems();
	UE_LOG(LogLS, Log, TEXT("[Save] New game started - all save files deleted for a fresh start"));
}

void ULSSaveSubsystem::ApplyStarterItems()
{
	if (!SaveData)
	{
		return;
	}

	const ULSSaveSettings* Settings = GetDefault<ULSSaveSettings>();
	if (!Settings)
	{
		return;
	}

	ApplyLowestGradeChipStarterItems();
	ApplyConfiguredStarterItems();
}

void ULSSaveSubsystem::ApplyConfiguredStarterItems()
{
	const ULSSaveSettings* Settings = GetDefault<ULSSaveSettings>();
	if (!Settings || Settings->StarterItems.IsEmpty())
	{
		return;
	}

	for (const FLSStarterItemConfig& StarterItem : Settings->StarterItems)
	{
		if (StarterItem.ItemRowName.IsNone() || StarterItem.Amount <= 0)
		{
			UE_LOG(LogLS, Warning, TEXT("[Save] Starter item skipped because row or amount is invalid. Row=%s Amount=%d"),
				*StarterItem.ItemRowName.ToString(),
				StarterItem.Amount);
			continue;
		}

		AddStarterItemToArea(
			StarterItem.ItemRowName,
			StarterItem.Amount,
			StarterItem.TargetArea,
			TArray<FLSChipResolvedStat>(),
			TEXT("Configured"));
	}
}

void ULSSaveSubsystem::ApplyLowestGradeChipStarterItems()
{
	const ULSSaveSettings* SaveSettings = GetDefault<ULSSaveSettings>();
	const ULSDropSettings* DropSettings = GetDefault<ULSDropSettings>();
	if (!SaveSettings || !SaveSettings->bGrantLowestGradeChipsOnNewGame || !DropSettings)
	{
		return;
	}

	const UDataTable* ChipTable = DropSettings->ChipTable.LoadSynchronous();
	if (!ChipTable)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot grant starter chips because ChipTable is missing."));
		return;
	}

	EnsureChipEquipmentSlots();

	// 기본 지급 Supply 칩은 하드웨어 장착칸 10·9·8·7번(인덱스 9·8·7·6)에 뒤에서부터 채운다.
	int32 TargetEquipmentIndex = ChipEquipmentSlotCount - 1;

	constexpr TCHAR LowestChipGrade[] = TEXT("Supply");
	for (const TPair<FName, uint8*>& Pair : ChipTable->GetRowMap())
	{
		if (LSInventorySlotUtils::ResolveItemGradeFromRowName(Pair.Key) != LowestChipGrade)
		{
			continue;
		}

		if (TargetEquipmentIndex < 0)
		{
			UE_LOG(LogLS, Warning, TEXT("[Save] Starter chip '%s' skipped because hardware slots are full."), *Pair.Key.ToString());
			break;
		}

		FLSSessionItem& EquipmentSlot = SaveData->ChipEquipmentSlots[TargetEquipmentIndex];
		EquipmentSlot.ItemRowName = Pair.Key;
		EquipmentSlot.Amount = 1;
		EquipmentSlot.ChipStats = LSChipStats::RollChipStats(Pair.Key);
		--TargetEquipmentIndex;
	}
}

void ULSSaveSubsystem::AddStarterItemToArea(
	const FName ItemRowName,
	const int32 Amount,
	const ELSInventorySlotArea TargetArea,
	const TArray<FLSChipResolvedStat>& ChipStats,
	const TCHAR* SourceLabel)
{
	TArray<FLSSessionItem>* TargetSlots = GetMutableStoredSlots(TargetArea);
	if (!TargetSlots)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Starter item skipped because target area is invalid. Source=%s Row=%s Area=%d"),
			SourceLabel,
			*ItemRowName.ToString(),
			static_cast<int32>(TargetArea));
		return;
	}

	if (TargetArea == ELSInventorySlotArea::Warehouse && HasWarehouseOverflow())
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Starter item skipped because warehouse contains overflow items. Source=%s Row=%s"),
			SourceLabel, *ItemRowName.ToString());
		return;
	}

	FLSSessionItem RemainingItem;
	LSInventorySlotUtils::TryAddItemsToSlotArray(
		*TargetSlots,
		ItemRowName,
		Amount,
		GetStarterTargetMaxSlotCount(TargetArea),
		ChipStats,
		RemainingItem);
	if (LSInventorySlotUtils::IsFilled(RemainingItem))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Starter item was partially granted. Source=%s Row=%s Remaining=%d Area=%d"),
			SourceLabel,
			*RemainingItem.ItemRowName.ToString(),
			RemainingItem.Amount,
			static_cast<int32>(TargetArea));
	}
}

int32 ULSSaveSubsystem::GetStarterTargetMaxSlotCount(const ELSInventorySlotArea TargetArea) const
{
	switch (TargetArea)
	{
	case ELSInventorySlotArea::Inventory:
		return GetMaxInventorySlotCount();
	case ELSInventorySlotArea::Safe:
		return GetMaxSafeStashSlotCount();
	case ELSInventorySlotArea::Warehouse:
		return GetMaxWarehouseSlotCount();
	default:
		{
			return INDEX_NONE;
		}
	}
}

void ULSSaveSubsystem::DeleteAllSaveFiles() const
{
	const FString ResolvedSlotName = GetResolvedSlotName();

	UGameplayStatics::DeleteGameInSlot(ResolvedSlotName, 0);
	UGameplayStatics::DeleteGameInSlot(SlotName, 0);

	// .sav 외에 디버그 json도 함께 정리한다.
	IFileManager& FileManager = IFileManager::Get();
	const FString SaveGamesDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"));
	FileManager.Delete(*FPaths::Combine(SaveGamesDir, GetResolvedDebugFileName()), false, false, true);
	FileManager.Delete(*FPaths::Combine(SaveGamesDir, DebugFileName), false, false, true);

	UE_LOG(LogLS, Log, TEXT("[Save] Deleted save files for slots %s and %s"), *ResolvedSlotName, *SlotName);
}

void ULSSaveSubsystem::Load()
{
	const FString ResolvedSlotName = GetResolvedSlotName();
	const FString LoadSlotName = UGameplayStatics::DoesSaveGameExist(ResolvedSlotName, 0) ? ResolvedSlotName : SlotName;

	if (UGameplayStatics::DoesSaveGameExist(LoadSlotName, 0))
	{
		SaveData = Cast<ULSSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadSlotName, 0));
		if (SaveData)
		{
			MigrateInventory();
			LSInventorySlotUtils::NormalizeSlotArray(SaveData->Inventory);
			LSInventorySlotUtils::NormalizeSlotArray(SaveData->WarehouseItems);
			LSInventorySlotUtils::NormalizeSlotArray(SaveData->SafeStash);
			if (HasWarehouseOverflow())
			{
				UE_LOG(LogLS, Warning, TEXT("[Save] Loaded warehouse contains slots beyond capacity. Items are preserved, but new warehouse entries are blocked. Slots=%d Max=%d"),
					SaveData->WarehouseItems.Num(), GetMaxWarehouseSlotCount());
			}
			EnsureChipEquipmentSlots();
			EnsureEquipmentSlots();
			EnsureGoldInitialized();
			ResolveInterruptedRaid();
			Save();
		}
		UE_LOG(LogLS, Log, TEXT("[Save] Loaded slot %s into %s. Inventory slots: %d, warehouse slots: %d, safe slots: %d"),
			*LoadSlotName,
			*ResolvedSlotName,
			SaveData ? SaveData->Inventory.Num() : 0,
			SaveData ? SaveData->WarehouseItems.Num() : 0,
			SaveData ? SaveData->SafeStash.Num() : 0);
		return;
	}

	SaveData = Cast<ULSSaveGame>(UGameplayStatics::CreateSaveGameObject(ULSSaveGame::StaticClass()));
	EnsureChipEquipmentSlots();
	EnsureEquipmentSlots();
	EnsureGoldInitialized();
	UE_LOG(LogLS, Log, TEXT("[Save] Created new save object for slot %s"), *ResolvedSlotName);
}

void ULSSaveSubsystem::EnsureGoldInitialized()
{
	if (!SaveData || SaveData->bGoldInitialized)
	{
		return;
	}

	const ULSSaveSettings* Settings = GetDefault<ULSSaveSettings>();
	SaveData->Gold = Settings ? Settings->NewGameGold : 0;
	SaveData->bGoldInitialized = true;
	UE_LOG(LogLS, Log, TEXT("[Save] Gold initialized to %d."), SaveData->Gold);
}

int32 ULSSaveSubsystem::GetGold() const
{
	return SaveData ? SaveData->Gold : 0;
}

void ULSSaveSubsystem::AddGold(const int32 Amount)
{
	if (!SaveData || Amount <= 0)
	{
		return;
	}

	SaveData->Gold += Amount;
	Save();
	OnGoldChanged.Broadcast();
}

bool ULSSaveSubsystem::TrySpendGold(const int32 Amount)
{
	if (!SaveData || Amount <= 0 || SaveData->Gold < Amount)
	{
		return false;
	}

	SaveData->Gold -= Amount;
	Save();
	OnGoldChanged.Broadcast();
	return true;
}

void ULSSaveSubsystem::MigrateInventory()
{
	if (!SaveData || SaveData->bInventoryMigrated)
	{
		return;
	}

	if (SaveData->Inventory.IsEmpty())
	{
		// Player1Inventory 있으면 우선, 없으면 Stash에서 마이그레이션
		if (!SaveData->Player1Inventory.IsEmpty())
		{
		LSInventorySlotUtils::NormalizeSlotArray(SaveData->Player1Inventory);
			SaveData->Inventory = SaveData->Player1Inventory;
		}
		else if (!SaveData->Stash.IsEmpty())
		{
		LSInventorySlotUtils::NormalizeSlotArray(SaveData->Stash);
			SaveData->Inventory = SaveData->Stash;
		}
	}

	SaveData->bInventoryMigrated = true;
	UE_LOG(LogLS, Log, TEXT("[Save] Migrated legacy data into Inventory."));
}

void ULSSaveSubsystem::ResolveInterruptedRaid()
{
	if (!SaveData || !SaveData->bRaidSaveActive)
	{
		return;
	}

	TArray<FLSSessionItem> RecoveredStash = SaveData->ActiveRaidLoadout;
	LSInventorySlotUtils::NormalizeSlotArray(RecoveredStash);

	for (const FLSSessionItem& ConsumedItem : SaveData->ActiveRaidConsumedItems)
	{
		LSInventorySlotUtils::RemoveItemsFromSlotArray(RecoveredStash, ConsumedItem.ItemRowName, ConsumedItem.Amount);
	}

	SaveData->Inventory = RecoveredStash;
	SaveData->bRaidSaveActive = false;
	SaveData->ActiveRaidLoadout.Reset();
	SaveData->ActiveRaidConsumedItems.Reset();

	UE_LOG(LogLS, Log, TEXT("[Save] Interrupted raid resolved. Recovered inventory slots: %d"), SaveData->Inventory.Num());
}

void ULSSaveSubsystem::Save()
{
	if (!SaveData)
	{
		return;
	}

	const FString ResolvedSlotName = GetResolvedSlotName();
	UGameplayStatics::SaveGameToSlot(SaveData, ResolvedSlotName, 0);
#if !UE_BUILD_SHIPPING
	SaveDebugJson();
#endif
	UE_LOG(LogLS, Log, TEXT("[Save] Save completed: %s"), *ResolvedSlotName);
}

void ULSSaveSubsystem::EnsureChipEquipmentSlots()
{
	if (!SaveData)
	{
		return;
	}

	while (SaveData->ChipEquipmentSlots.Num() < ChipEquipmentSlotCount)
	{
		SaveData->ChipEquipmentSlots.Add(LSInventorySlotUtils::MakeEmptyItem());
	}

	if (SaveData->ChipEquipmentSlots.Num() > ChipEquipmentSlotCount)
	{
		SaveData->ChipEquipmentSlots.SetNum(ChipEquipmentSlotCount);
	}
}

void ULSSaveSubsystem::EnsureEquipmentSlots()
{
	if (!SaveData)
	{
		return;
	}

	while (SaveData->EquipmentSlots.Num() < EquipmentSlotCount)
	{
		SaveData->EquipmentSlots.Add(LSInventorySlotUtils::MakeEmptyItem());
	}

	if (SaveData->EquipmentSlots.Num() > EquipmentSlotCount)
	{
		SaveData->EquipmentSlots.SetNum(EquipmentSlotCount);
	}
}

int32 ULSSaveSubsystem::GetCarryingProtocolSlotBonus(const FName EnableName) const
{
	return SaveData ? ComputeCarryingProtocolSlotBonus(SaveData->ChipEquipmentSlots, EnableName) : 0;
}

int32 ULSSaveSubsystem::ComputeCarryingProtocolSlotBonus(const TArray<FLSSessionItem>& EquipmentSlots, const FName EnableName) const
{
	if (!SaveData || EnableName.IsNone())
	{
		return 0;
	}

	UGameInstance* GameInstance = GetGameInstance();
	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	if (!GameDataSubsystem)
	{
		return 0;
	}

	const int32 InactiveSlotCount = LSChipStats::ResolveInactiveSignalSlotCount(GetChipSignalGaugePercent());
	const TArray<FLSSessionItem> ActiveEquipmentItems = LSChipStats::BuildSignalActiveEquipmentItems(EquipmentSlots, InactiveSlotCount);
	const int32 CurrentCarrying = LSChipStats::AggregateChipProtocolTotals(ActiveEquipmentItems, this).Carrying;
	const int32 PreviousCarrying = LSChipStats::AggregateChipProtocolTotals(EquipmentSlots, this).Carrying;
	return GameDataSubsystem->GetVisibleProtocolEnableValueSum(
		ELSProtocolType::Carrying,
		EnableName,
		CurrentCarrying,
		PreviousCarrying,
		TEXT("SaveCarryingSlotBonus"));
}

bool ULSSaveSubsystem::WouldChipEquipmentDropInventoryItems(const TArray<FLSSessionItem>& HypotheticalSlots) const
{
	if (!SaveData)
	{
		return false;
	}

	// 가정 장착 배열 기준 "예상 최대 인벤토리 슬롯 수". 실제 반영 후 GetMaxInventorySlotCount()와 동일하므로 초과 드롭 여부를 정확히 예측한다.
	const int32 PredictedMaxInventorySlotCount = ComputePredictedMaxInventorySlotCount(HypotheticalSlots);

	// 예상 최대 슬롯 수 이상 인덱스에 채워진 인벤토리 칸이 하나라도 있으면, 반영 시 그 아이템들이 월드로 드롭된다.
	for (int32 SlotIndex = FMath::Max(0, PredictedMaxInventorySlotCount); SlotIndex < SaveData->Inventory.Num(); ++SlotIndex)
	{
		if (LSInventorySlotUtils::IsFilled(SaveData->Inventory[SlotIndex]))
		{
			return true;
		}
	}
	return false;
}

int32 ULSSaveSubsystem::ComputePredictedMaxInventorySlotCount(const TArray<FLSSessionItem>& HypotheticalSlots) const
{
	return SaveDefaultMaxInventorySlotCount + ComputeCarryingProtocolSlotBonus(HypotheticalSlots, TEXT("Inventory"));
}

int32 ULSSaveSubsystem::FindEmptyInventorySlotForUnequip(const int32 PreferredSlotIndex, const int32 MaxSlotCount) const
{
	if (!SaveData || MaxSlotCount <= 0)
	{
		return INDEX_NONE;
	}

	const TArray<FLSSessionItem>& Inventory = SaveData->Inventory;
	const auto IsSlotEmpty = [&Inventory](const int32 SlotIndex)
	{
		return !Inventory.IsValidIndex(SlotIndex) || !LSInventorySlotUtils::IsFilled(Inventory[SlotIndex]);
	};

	if (PreferredSlotIndex >= 0 && PreferredSlotIndex < MaxSlotCount && IsSlotEmpty(PreferredSlotIndex))
	{
		return PreferredSlotIndex;
	}

	for (int32 SlotIndex = 0; SlotIndex < MaxSlotCount; ++SlotIndex)
	{
		if (IsSlotEmpty(SlotIndex))
		{
			return SlotIndex;
		}
	}
	return INDEX_NONE;
}

FLSSessionItem* ULSSaveSubsystem::ResolveFilledChipEquipmentSlot(const int32 EquipmentIndex, const TCHAR* ContextLabel)
{
	if (!SaveData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot %s because SaveData is missing."), ContextLabel);
		return nullptr;
	}

	if (EquipmentIndex < 0 || EquipmentIndex >= ChipEquipmentSlotCount)
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot %s because equipment index is invalid. Index=%d"), ContextLabel, EquipmentIndex);
		return nullptr;
	}

	EnsureChipEquipmentSlots();
	FLSSessionItem& EquipmentSlot = SaveData->ChipEquipmentSlots[EquipmentIndex];
	if (!LSInventorySlotUtils::IsFilled(EquipmentSlot))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot %s because equipment slot is empty. Index=%d"), ContextLabel, EquipmentIndex);
		return nullptr;
	}

	if (!EquipmentSlot.ItemRowName.ToString().StartsWith(TEXT("Chip_")))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Cannot %s because item '%s' is not a chip."), ContextLabel, *EquipmentSlot.ItemRowName.ToString());
		return nullptr;
	}

	return &EquipmentSlot;
}

bool ULSSaveSubsystem::UnequipChipToInventory(const int32 EquipmentIndex, const int32 PreferredSlotIndex, int32& OutPlacedSlotIndex)
{
	OutPlacedSlotIndex = INDEX_NONE;
	FLSSessionItem* EquipmentSlotPtr = ResolveFilledChipEquipmentSlot(EquipmentIndex, TEXT("unequip chip to inventory"));
	if (!EquipmentSlotPtr)
	{
		return false;
	}
	FLSSessionItem& EquipmentSlot = *EquipmentSlotPtr;

	// 해제하면 적재(Carrying) 보너스가 줄어 최대 인벤토리 슬롯 수가 줄 수 있다. 해제 후 예상 용량 밖 칸에
	// 넣으면 반영 직후 그 칩이 되드롭되므로, 가정 배열로 계산한 예상 용량 안에서만 자리를 찾는다.
	TArray<FLSSessionItem> HypotheticalSlots = SaveData->ChipEquipmentSlots;
	HypotheticalSlots[EquipmentIndex] = LSInventorySlotUtils::MakeEmptyItem();
	const int32 PredictedMaxSlotCount = ComputePredictedMaxInventorySlotCount(HypotheticalSlots);
	const int32 TargetSlotIndex = FindEmptyInventorySlotForUnequip(PreferredSlotIndex, PredictedMaxSlotCount);
	if (TargetSlotIndex == INDEX_NONE)
	{
		// 자리 없음 — 상태를 바꾸지 않고 실패를 알린다(호출자가 창고 폴백).
		return false;
	}

	// 스택 병합 없이 빈 칸에 그대로 배치한다(칩은 ChipStats를 가진 개별 아이템이고, 배치 인덱스가 항상 정확해야 UI가 diff 없이 갱신 가능).
	TArray<FLSSessionItem>& Inventory = GetMutableInventory();
	while (Inventory.Num() <= TargetSlotIndex)
	{
		Inventory.Add(LSInventorySlotUtils::MakeEmptyItem());
	}
	Inventory[TargetSlotIndex] = EquipmentSlot;
	EquipmentSlot = LSInventorySlotUtils::MakeEmptyItem();

	Save();
	OnChipLoadoutChanged.Broadcast();
	OutPlacedSlotIndex = TargetSlotIndex;
	return true;
}

bool ULSSaveSubsystem::WouldUnequipChipDropInventoryItems(const int32 EquipmentIndex) const
{
	if (!SaveData || !SaveData->ChipEquipmentSlots.IsValidIndex(EquipmentIndex))
	{
		return false;
	}

	// 해제 대상 칸을 비운 가정 배열로 판정한다.
	TArray<FLSSessionItem> HypotheticalSlots = SaveData->ChipEquipmentSlots;
	HypotheticalSlots[EquipmentIndex] = LSInventorySlotUtils::MakeEmptyItem();
	return WouldChipEquipmentDropInventoryItems(HypotheticalSlots);
}

bool ULSSaveSubsystem::WouldSwapChipDropInventoryItems(const ELSInventorySlotArea SourceArea, const int32 SourceIndex, const int32 EquipmentIndex) const
{
	if (!SaveData || !SaveData->ChipEquipmentSlots.IsValidIndex(EquipmentIndex))
	{
		return false;
	}

	const TArray<FLSSessionItem>* SourceSlots = GetStoredSlots(SourceArea);
	if (!SourceSlots || !SourceSlots->IsValidIndex(SourceIndex) || !LSInventorySlotUtils::IsFilled((*SourceSlots)[SourceIndex]))
	{
		return false;
	}

	// 스왑 대상 칸에 새(저장) 칩을 얹은 가정 배열로 판정한다. 새 칩의 적재 프로토콜이 기존 장착 칩보다 낮으면 용량이 줄 수 있다.
	TArray<FLSSessionItem> HypotheticalSlots = SaveData->ChipEquipmentSlots;
	HypotheticalSlots[EquipmentIndex] = (*SourceSlots)[SourceIndex];
	return WouldChipEquipmentDropInventoryItems(HypotheticalSlots);
}

TArray<FLSSessionItem>& ULSSaveSubsystem::GetMutableInventory()
{
	return SaveData->Inventory;
}

TArray<FLSSessionItem>* ULSSaveSubsystem::GetMutableStoredSlots(const ELSInventorySlotArea SlotArea)
{
	if (!SaveData)
	{
		return nullptr;
	}

	switch (SlotArea)
	{
	case ELSInventorySlotArea::Inventory:
		return &SaveData->Inventory;
	case ELSInventorySlotArea::Safe:
		return &SaveData->SafeStash;
	case ELSInventorySlotArea::Warehouse:
		return &SaveData->WarehouseItems;
	case ELSInventorySlotArea::Equipment:
		return &SaveData->EquipmentSlots;
	default:
		return nullptr;
	}
}

const TArray<FLSSessionItem>* ULSSaveSubsystem::GetStoredSlots(const ELSInventorySlotArea SlotArea) const
{
	if (!SaveData)
	{
		return nullptr;
	}

	switch (SlotArea)
	{
	case ELSInventorySlotArea::Inventory:
		return &SaveData->Inventory;
	case ELSInventorySlotArea::Safe:
		return &SaveData->SafeStash;
	case ELSInventorySlotArea::Warehouse:
		return &SaveData->WarehouseItems;
	case ELSInventorySlotArea::Equipment:
		return &SaveData->EquipmentSlots;
	default:
		return nullptr;
	}
}

void ULSSaveSubsystem::SaveDebugJson() const
{
	if (!SaveData)
	{
		return;
	}

	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(TEXT("slotName"), GetResolvedSlotName());
	RootObject->SetBoolField(TEXT("raidSaveActive"), SaveData->bRaidSaveActive);
	RootObject->SetNumberField(TEXT("chipSignalGaugePercent"), GetChipSignalGaugePercent());

	auto AddSessionItemFields = [](const TSharedRef<FJsonObject>& ItemObject, const FLSSessionItem& Item, const int32 SlotIndex)
	{
		ItemObject->SetNumberField(TEXT("slotIndex"), SlotIndex);
		ItemObject->SetStringField(TEXT("itemRowName"), Item.ItemRowName.ToString());
		ItemObject->SetNumberField(TEXT("amount"), Item.Amount);

		if (!Item.ChipStats.IsEmpty())
		{
			TArray<TSharedPtr<FJsonValue>> ChipStatsArray;
			ChipStatsArray.Reserve(Item.ChipStats.Num());
			for (const FLSChipResolvedStat& Stat : Item.ChipStats)
			{
				TSharedRef<FJsonObject> StatObject = MakeShared<FJsonObject>();
				StatObject->SetStringField(TEXT("statKey"), Stat.StatKey.ToString());
				StatObject->SetNumberField(TEXT("value"), Stat.Value);
				ChipStatsArray.Add(MakeShared<FJsonValueObject>(StatObject));
			}
			ItemObject->SetArrayField(TEXT("chipStats"), ChipStatsArray);
		}
	};

	auto AddSlotArrayField = [&RootObject, &AddSessionItemFields](const TCHAR* FieldName, const TArray<FLSSessionItem>& Items)
	{
		TArray<TSharedPtr<FJsonValue>> JsonArray;
		JsonArray.Reserve(Items.Num());
		for (const FLSSessionItem& Item : Items)
		{
			TSharedRef<FJsonObject> ItemObject = MakeShared<FJsonObject>();
			AddSessionItemFields(ItemObject, Item, JsonArray.Num());
			JsonArray.Add(MakeShared<FJsonValueObject>(ItemObject));
		}

		RootObject->SetArrayField(FieldName, JsonArray);
	};

	AddSlotArrayField(TEXT("inventory"), SaveData->Inventory);
	AddSlotArrayField(TEXT("warehouseItems"), SaveData->WarehouseItems);
	AddSlotArrayField(TEXT("safeStash"), SaveData->SafeStash);
	AddSlotArrayField(TEXT("chipEquipmentSlots"), SaveData->ChipEquipmentSlots);

	TSharedRef<FJsonObject> SkillLoadoutsObject = MakeShared<FJsonObject>();
	for (const TPair<int32, FLSSkillLoadout>& Pair : SaveData->SkillLoadoutsByCharacter)
	{
		TSharedRef<FJsonObject> LoadoutObject = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> SkillIDArray;
		SkillIDArray.Reserve(Pair.Value.SkillIDs.Num());
		for (const int32 SkillID : Pair.Value.SkillIDs)
		{
			SkillIDArray.Add(MakeShared<FJsonValueNumber>(SkillID));
		}
		LoadoutObject->SetArrayField(TEXT("skillIDs"), SkillIDArray);
		LoadoutObject->SetBoolField(TEXT("initialized"), Pair.Value.bInitialized);
		SkillLoadoutsObject->SetObjectField(FString::FromInt(Pair.Key), LoadoutObject);
	}
	RootObject->SetObjectField(TEXT("skillLoadoutsByCharacter"), SkillLoadoutsObject);

	TArray<TSharedPtr<FJsonValue>> ActiveRaidLoadoutArray;
	ActiveRaidLoadoutArray.Reserve(SaveData->ActiveRaidLoadout.Num());
	for (const FLSSessionItem& Item : SaveData->ActiveRaidLoadout)
	{
		TSharedRef<FJsonObject> ItemObject = MakeShared<FJsonObject>();
		AddSessionItemFields(ItemObject, Item, ActiveRaidLoadoutArray.Num());
		ActiveRaidLoadoutArray.Add(MakeShared<FJsonValueObject>(ItemObject));
	}

	RootObject->SetArrayField(TEXT("activeRaidLoadout"), ActiveRaidLoadoutArray);

	TArray<TSharedPtr<FJsonValue>> ActiveRaidConsumedItemsArray;
	ActiveRaidConsumedItemsArray.Reserve(SaveData->ActiveRaidConsumedItems.Num());
	for (const FLSSessionItem& Item : SaveData->ActiveRaidConsumedItems)
	{
		TSharedRef<FJsonObject> ItemObject = MakeShared<FJsonObject>();
		AddSessionItemFields(ItemObject, Item, ActiveRaidConsumedItemsArray.Num());
		ActiveRaidConsumedItemsArray.Add(MakeShared<FJsonValueObject>(ItemObject));
	}

	RootObject->SetArrayField(TEXT("activeRaidConsumedItems"), ActiveRaidConsumedItemsArray);

	FString OutputString;
	const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutputString);

	if (!FJsonSerializer::Serialize(RootObject, Writer))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Debug JSON serialize failed"));
		return;
	}

	const FString DebugFilePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), GetResolvedDebugFileName());
	if (!FFileHelper::SaveStringToFile(OutputString, *DebugFilePath))
	{
		UE_LOG(LogLS, Warning, TEXT("[Save] Debug JSON write failed: %s"), *DebugFilePath);
		return;
	}

	UE_LOG(LogLS, Log, TEXT("[Save] Debug JSON written: %s"), *DebugFilePath);
}
