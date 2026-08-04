#include "Data/LSChipStats.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/WidgetComponent.h"
#include "Data/LSDropSettings.h"
#include "Gameplay/LSWorldDroppedItem.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "Misc/AutomationTest.h"
#include "Session/LSSessionSubsystem.h"
#include "UI/LSInteractHintWidget.h"

namespace
{
TArray<FLSSessionItem> MakeChipSignalDrainTestSlots(const TArray<int32>& FilledSlotIndices)
{
	TArray<FLSSessionItem> Slots;
	Slots.SetNum(10);
	for (const int32 SlotIndex : FilledSlotIndices)
	{
		Slots[SlotIndex].ItemRowName = TEXT("Chip_Supply_HP");
		Slots[SlotIndex].Amount = 1;
	}
	return Slots;
}

TArray<float> CollectChipSignalDrainTestSequence(const TArray<FLSSessionItem>& Slots)
{
	TArray<float> Sequence;
	float CurrentPercent = 1.0f;
	for (int32 Step = 0; Step < 10; ++Step)
	{
		float NextPercent = 0.0f;
		if (!LSChipStats::TryResolveNextSignalGaugePercent(Slots, CurrentPercent, NextPercent))
		{
			break;
		}

		Sequence.Add(NextPercent);
		CurrentPercent = NextPercent;
		if (CurrentPercent <= 0.0f)
		{
			break;
		}
	}
	return Sequence;
}

bool VerifyChipSignalDrainTestSequence(
	FAutomationTestBase& Test,
	const TCHAR* Label,
	const TArray<float>& Actual,
	const TArray<float>& Expected)
{
	bool bSuccess = Test.TestEqual(FString::Printf(TEXT("%s 단계 수"), Label), Actual.Num(), Expected.Num());
	for (int32 Index = 0; Index < FMath::Min(Actual.Num(), Expected.Num()); ++Index)
	{
		bSuccess &= Test.TestTrue(
			FString::Printf(TEXT("%s %d단계"), Label, Index + 1),
			FMath::IsNearlyEqual(Actual[Index], Expected[Index]));
	}
	return bSuccess;
}

FLSSessionItem MakeChipSignalOverflowTestItem(const FName RowName, const int32 Amount = 1)
{
	FLSSessionItem Item;
	Item.ItemRowName = RowName;
	Item.Amount = Amount;
	return Item;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLSChipSignalDrainSequenceTest,
	"LostSignal.Chip.SignalDrainSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLSChipSignalDrainSequenceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	bool bSuccess = true;

	bSuccess &= VerifyChipSignalDrainTestSequence(*this, TEXT("10칸 장착"),
		CollectChipSignalDrainTestSequence(MakeChipSignalDrainTestSlots({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 })),
		{ 0.9f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.3f, 0.2f, 0.1f, 0.0f });
	bSuccess &= VerifyChipSignalDrainTestSequence(*this, TEXT("중간 빈칸"),
		CollectChipSignalDrainTestSequence(MakeChipSignalDrainTestSlots({ 0, 1, 2, 3, 5, 6, 7, 8, 9 })),
		{ 0.9f, 0.8f, 0.7f, 0.6f, 0.4f, 0.3f, 0.2f, 0.1f, 0.0f });
	bSuccess &= VerifyChipSignalDrainTestSequence(*this, TEXT("분산 장착"),
		CollectChipSignalDrainTestSequence(MakeChipSignalDrainTestSlots({ 2, 5, 8 })),
		{ 0.7f, 0.4f, 0.0f });
	bSuccess &= VerifyChipSignalDrainTestSequence(*this, TEXT("칩 하나"),
		CollectChipSignalDrainTestSequence(MakeChipSignalDrainTestSlots({ 6 })),
		{ 0.0f });

	float EmptyNextPercent = 1.0f;
	bSuccess &= TestFalse(TEXT("칩이 없으면 다음 단계 없음"),
		LSChipStats::TryResolveNextSignalGaugePercent(MakeChipSignalDrainTestSlots({}), 1.0f, EmptyNextPercent));
	bSuccess &= TestTrue(TEXT("칩이 없으면 게이지 0"), FMath::IsNearlyZero(EmptyNextPercent));
	return bSuccess;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLSRaidInventoryOverflowExtractionTest,
	"LostSignal.Inventory.RaidOverflowExtraction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLSRaidInventoryOverflowExtractionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	bool bSuccess = true;

	TArray<FLSSessionItem> InventoryItems;
	InventoryItems.SetNum(13);
	InventoryItems[0] = MakeChipSignalOverflowTestItem(TEXT("Item_NormalFirst"));
	InventoryItems[9] = MakeChipSignalOverflowTestItem(TEXT("Item_NormalBoundary"));
	InventoryItems[10] = MakeChipSignalOverflowTestItem(TEXT("Item_OverflowFirst"), 2);
	InventoryItems[12] = MakeChipSignalOverflowTestItem(TEXT("Item_OverflowSecond"), 3);

	TArray<FLSSessionItem> SafeItems;
	SafeItems.SetNum(6);
	SafeItems[5] = MakeChipSignalOverflowTestItem(TEXT("Item_SafeOverflow"));

	ULSRaidInventoryComponent* RaidInventory = NewObject<ULSRaidInventoryComponent>();
	bSuccess &= TestNotNull(TEXT("레이드 인벤토리 컴포넌트 생성"), RaidInventory);
	if (!RaidInventory)
	{
		return false;
	}

	RaidInventory->MirrorRaidInventoryState(InventoryItems, SafeItems, TArray<FLSSessionItem>());
	bSuccess &= TestEqual(TEXT("월드 없는 테스트의 기본 일반 슬롯 수"), RaidInventory->GetMaxInventorySlotCount(), 10);

	TArray<FLSSessionItem> ExtractedItems;
	bSuccess &= TestEqual(TEXT("채워진 일반 오버플로 두 칸 추출"), RaidInventory->ExtractOverflowInventoryItems(ExtractedItems), 2);
	bSuccess &= TestEqual(TEXT("추출 결과 개수"), ExtractedItems.Num(), 2);
	bSuccess &= TestEqual(TEXT("첫 초과 아이템 순서 보존"), ExtractedItems[0].ItemRowName, FName(TEXT("Item_OverflowFirst")));
	bSuccess &= TestEqual(TEXT("두 번째 초과 아이템 순서 보존"), ExtractedItems[1].ItemRowName, FName(TEXT("Item_OverflowSecond")));
	bSuccess &= TestEqual(TEXT("첫 초과 아이템 수량 보존"), ExtractedItems[0].Amount, 2);
	bSuccess &= TestEqual(TEXT("두 번째 초과 아이템 수량 보존"), ExtractedItems[1].Amount, 3);

	const TArray<FLSSessionItem>& RemainingInventory = RaidInventory->GetSessionInventory();
	bSuccess &= TestTrue(TEXT("정상 첫 슬롯 보존"), LSInventorySlotUtils::IsFilled(RemainingInventory[0]));
	bSuccess &= TestTrue(TEXT("정상 경계 슬롯 보존"), LSInventorySlotUtils::IsFilled(RemainingInventory[9]));
	bSuccess &= TestFalse(TEXT("첫 초과 슬롯 비움"), LSInventorySlotUtils::IsFilled(RemainingInventory[10]));
	bSuccess &= TestFalse(TEXT("두 번째 초과 슬롯 비움"), LSInventorySlotUtils::IsFilled(RemainingInventory[12]));
	bSuccess &= TestTrue(TEXT("보호 슬롯 초과분은 보존"), LSInventorySlotUtils::IsFilled(RaidInventory->GetSessionSafeInventory()[5]));
	bSuccess &= TestEqual(TEXT("재실행 시 추가 추출 없음"), RaidInventory->ExtractOverflowInventoryItems(ExtractedItems), 0);
	bSuccess &= TestEqual(TEXT("재실행 후 결과 개수 유지"), ExtractedItems.Num(), 2);
	return bSuccess;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLSWorldDroppedItemSettingsTest,
	"LostSignal.Inventory.WorldDroppedItemSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLSWorldDroppedItemSettingsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	bool bSuccess = true;

	const ULSDropSettings* DropSettings = GetDefault<ULSDropSettings>();
	bSuccess &= TestNotNull(TEXT("LS Drop Settings 로드"), DropSettings);
	if (!DropSettings)
	{
		return false;
	}

	UClass* DroppedItemClass = DropSettings->WorldDroppedItemClass.LoadSynchronous();
	bSuccess &= TestNotNull(TEXT("월드 드랍 BP 클래스 로드"), DroppedItemClass);
	if (!DroppedItemClass)
	{
		return false;
	}

	bSuccess &= TestTrue(TEXT("월드 드랍 클래스 타입"), DroppedItemClass->IsChildOf(ALSWorldDroppedItem::StaticClass()));
	ALSWorldDroppedItem* DroppedItemCDO = Cast<ALSWorldDroppedItem>(DroppedItemClass->GetDefaultObject());
	bSuccess &= TestNotNull(TEXT("월드 드랍 BP 기본 객체"), DroppedItemCDO);
	if (!DroppedItemCDO)
	{
		return false;
	}

	const UWidgetComponent* InteractWidget = Cast<UWidgetComponent>(
		DroppedItemCDO->GetDefaultSubobjectByName(TEXT("InteractWidget")));
	bSuccess &= TestNotNull(TEXT("상호작용 안내 위젯 컴포넌트"), InteractWidget);
	if (!InteractWidget)
	{
		return false;
	}

	const UClass* InteractWidgetClass = InteractWidget->GetWidgetClass();
	bSuccess &= TestNotNull(TEXT("상호작용 안내 위젯 클래스"), InteractWidgetClass);
	if (InteractWidgetClass)
	{
		bSuccess &= TestTrue(TEXT("상호작용 안내 위젯 타입"),
			InteractWidgetClass->IsChildOf(ULSInteractHintWidget::StaticClass()));
	}

	return bSuccess;
}

#endif
