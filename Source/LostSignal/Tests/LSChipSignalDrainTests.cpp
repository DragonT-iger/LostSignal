#include "Data/LSChipStats.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Session/LSSessionSubsystem.h"

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

#endif
