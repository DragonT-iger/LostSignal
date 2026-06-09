#include "Data/LSProtocolTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Data/LSGameDataSubsystem.h"
#include "Data/LSProtocolUnlockRow.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLSProtocolUnlockRowGroupingTest,
	"LostSignal.Protocol.UnlockRowGrouping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLSProtocolUnlockRowGroupingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	ELSProtocolType ProtocolType = ELSProtocolType::Survival;
	TestTrue(TEXT("Survival row prefix"), LSProtocol::ResolveProtocolTypeFromRowName(FName(TEXT("Protocol_Survival_1")), ProtocolType));
	TestEqual(TEXT("Survival protocol type"), static_cast<uint8>(ProtocolType), static_cast<uint8>(ELSProtocolType::Survival));

	TestTrue(TEXT("Carrying row prefix"), LSProtocol::ResolveProtocolTypeFromRowName(FName(TEXT("Protocol_Carrying_1")), ProtocolType));
	TestEqual(TEXT("Carrying protocol type"), static_cast<uint8>(ProtocolType), static_cast<uint8>(ELSProtocolType::Carrying));
	TestTrue(TEXT("Carrying info retention row prefix"), LSProtocol::ResolveProtocolTypeFromRowName(FName(TEXT("Protocol_Carrying_18")), ProtocolType));
	TestEqual(TEXT("Carrying info retention protocol type"), static_cast<uint8>(ProtocolType), static_cast<uint8>(ELSProtocolType::Carrying));

	TestTrue(TEXT("Battle row prefix"), LSProtocol::ResolveProtocolTypeFromRowName(FName(TEXT("Protocol_Battle_1")), ProtocolType));
	TestEqual(TEXT("Battle protocol type"), static_cast<uint8>(ProtocolType), static_cast<uint8>(ELSProtocolType::Battle));

	TestTrue(TEXT("Navigation row prefix"), LSProtocol::ResolveProtocolTypeFromRowName(FName(TEXT("Protocol_Navigation_1")), ProtocolType));
	TestEqual(TEXT("Navigation protocol type"), static_cast<uint8>(ProtocolType), static_cast<uint8>(ELSProtocolType::Navigation));

	TestFalse(TEXT("Unknown row prefix"), LSProtocol::ResolveProtocolTypeFromRowName(FName(TEXT("Protocol_Unknown_1")), ProtocolType));
	TestEqual(TEXT("Stemina typo is normalized"), LSProtocol::NormalizeProtocolEnableName(FName(TEXT("Stemina_Bar"))), FName(TEXT("Stamina_Bar")));
	TestEqual(TEXT("Quest enable name is preserved"), LSProtocol::NormalizeProtocolEnableName(FName(TEXT("Quest"))), FName(TEXT("Quest")));

	const ULSGameDataSubsystem* GameDataSubsystem = NewObject<ULSGameDataSubsystem>();
	FLSProtocolUnlockRow ProtectedRow;
	ProtectedRow.Protocol_Required_Level = 8;
	ProtectedRow.Protocol_Enable_Name = TEXT("Inventory");
	ProtectedRow.Protocol_Enable_Value = 5;
	ProtectedRow.Protocol_Protected_Level = 6;

	bool bProtected = false;
	TestTrue(TEXT("Required level unlock is visible"), GameDataSubsystem->IsProtocolUnlockVisible(ProtectedRow, 8, 8, &bProtected));
	TestFalse(TEXT("Required level unlock is not protected"), bProtected);
	TestTrue(TEXT("Previous unlock remains visible in protected range"), GameDataSubsystem->IsProtocolUnlockVisible(ProtectedRow, 6, 8, &bProtected));
	TestTrue(TEXT("Previous unlock reports protected state"), bProtected);
	TestFalse(TEXT("Previous unlock is hidden below protected range"), GameDataSubsystem->IsProtocolUnlockVisible(ProtectedRow, 5, 8, &bProtected));

	return true;
}

#endif
