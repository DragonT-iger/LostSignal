#include "Inventory/LSRaidInventoryComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/LSInventorySlotUtils.h"
#include "Misc/AutomationTest.h"

namespace
{
FLSSessionItem MakeInventoryQuickTransferTestItem(const FName RowName, const int32 Amount = 1)
{
	FLSSessionItem Item;
	Item.ItemRowName = RowName;
	Item.Amount = Amount;
	return Item;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLSInventorySafeQuickTransferTest,
	"LostSignal.Inventory.SafeQuickTransfer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLSInventorySafeQuickTransferTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FName StackableRow(TEXT("Item_ScrapMetalFragment_Supply"));
	const int32 MaxStack = LSInventorySlotUtils::ResolveItemMaxStack(StackableRow, TEXT("SafeQuickTransferTest"));
	if (!TestTrue(TEXT("테스트 재료는 스택 가능"), MaxStack > 1))
	{
		return false;
	}

	ULSRaidInventoryComponent* RaidInventory = NewObject<ULSRaidInventoryComponent>();
	if (!TestNotNull(TEXT("레이드 인벤토리 컴포넌트 생성"), RaidInventory))
	{
		return false;
	}

	TArray<FLSSessionItem> SafeItems;
	SafeItems.Add(MakeInventoryQuickTransferTestItem(StackableRow, 2));
	RaidInventory->MirrorRaidInventoryState({}, SafeItems, {});
	TestTrue(TEXT("빈 인벤토리로 보호 슬롯 이동"), RaidInventory->TransferSafeSlotToInventory(0));
	TestEqual(TEXT("빈 인벤토리 이동 수량"), RaidInventory->GetSessionInventory()[0].Amount, 2);
	TestFalse(TEXT("전체 이동 후 보호 슬롯 비움"), LSInventorySlotUtils::IsFilled(RaidInventory->GetSessionSafeInventory()[0]));

	TArray<FLSSessionItem> InventoryItems;
	InventoryItems.SetNum(10);
	InventoryItems[0] = MakeInventoryQuickTransferTestItem(StackableRow, MaxStack - 1);
	for (int32 SlotIndex = 1; SlotIndex < InventoryItems.Num(); ++SlotIndex)
	{
		InventoryItems[SlotIndex] = MakeInventoryQuickTransferTestItem(StackableRow, MaxStack);
	}
	SafeItems[0] = MakeInventoryQuickTransferTestItem(StackableRow, 2);
	RaidInventory->MirrorRaidInventoryState(InventoryItems, SafeItems, {});
	TestTrue(TEXT("기존 스택으로 부분 이동"), RaidInventory->TransferSafeSlotToInventory(0));
	TestEqual(TEXT("기존 스택 최대치까지 병합"), RaidInventory->GetSessionInventory()[0].Amount, MaxStack);
	TestEqual(TEXT("부분 이동 잔여 수량 보존"), RaidInventory->GetSessionSafeInventory()[0].Amount, 1);

	InventoryItems.SetNum(10);
	for (FLSSessionItem& Item : InventoryItems)
	{
		Item = MakeInventoryQuickTransferTestItem(StackableRow, MaxStack);
	}
	SafeItems[0] = MakeInventoryQuickTransferTestItem(StackableRow);
	RaidInventory->MirrorRaidInventoryState(InventoryItems, SafeItems, {});
	TestFalse(TEXT("가득 찬 인벤토리 이동 거부"), RaidInventory->TransferSafeSlotToInventory(0));
	TestTrue(TEXT("이동 실패 시 보호 아이템 보존"), LSInventorySlotUtils::IsFilled(RaidInventory->GetSessionSafeInventory()[0]));

	SafeItems.SetNum(5);
	SafeItems[4] = MakeInventoryQuickTransferTestItem(StackableRow);
	RaidInventory->MirrorRaidInventoryState({}, SafeItems, {});
	TestFalse(TEXT("잠긴 보호 슬롯 이동 거부"), RaidInventory->TransferSafeSlotToInventory(4));
	return TestTrue(TEXT("잠긴 보호 아이템 보존"), LSInventorySlotUtils::IsFilled(RaidInventory->GetSessionSafeInventory()[4]));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLSInventoryEquipmentQuickSwapCoreTest,
	"LostSignal.Inventory.EquipmentQuickSwapCore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLSInventoryEquipmentQuickSwapCoreTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FName NewWeapon(TEXT("Weapon_Test_New"));
	const FName EquippedWeapon(TEXT("Weapon_Test_Equipped"));
	TArray<FLSSessionItem> InventoryItems{ MakeInventoryQuickTransferTestItem(NewWeapon) };
	TArray<FLSSessionItem> EquipmentItems;
	EquipmentItems.SetNum(LSInventorySlotUtils::EquipmentSlotCount);
	EquipmentItems[static_cast<int32>(ELSEquipmentSlot::Weapon)] = MakeInventoryQuickTransferTestItem(EquippedWeapon);

	const bool bChanged = LSInventorySlotUtils::MoveEquipmentSlotBetweenArrays(
		InventoryItems,
		0,
		false,
		EquipmentItems,
		static_cast<int32>(ELSEquipmentSlot::Weapon),
		true,
		LSInventorySlotUtils::EquipmentSlotCount);

	TestTrue(TEXT("점유 장비칸 교체 성공"), bChanged);
	TestEqual(TEXT("새 장비가 장비칸으로 이동"), EquipmentItems[static_cast<int32>(ELSEquipmentSlot::Weapon)].ItemRowName, NewWeapon);
	return TestEqual(TEXT("기존 장비가 원본 인벤토리 칸으로 복귀"), InventoryItems[0].ItemRowName, EquippedWeapon);
}

#endif
