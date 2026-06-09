#include "Data/LSProtocolTypes.h"

#if WITH_DEV_AUTOMATION_TESTS

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

	TestTrue(TEXT("Battle row prefix"), LSProtocol::ResolveProtocolTypeFromRowName(FName(TEXT("Protocol_Battle_1")), ProtocolType));
	TestEqual(TEXT("Battle protocol type"), static_cast<uint8>(ProtocolType), static_cast<uint8>(ELSProtocolType::Battle));

	TestTrue(TEXT("Navigation row prefix"), LSProtocol::ResolveProtocolTypeFromRowName(FName(TEXT("Protocol_Navigation_1")), ProtocolType));
	TestEqual(TEXT("Navigation protocol type"), static_cast<uint8>(ProtocolType), static_cast<uint8>(ELSProtocolType::Navigation));

	TestFalse(TEXT("Unknown row prefix"), LSProtocol::ResolveProtocolTypeFromRowName(FName(TEXT("Protocol_Unknown_1")), ProtocolType));
	TestEqual(TEXT("Stemina typo is normalized"), LSProtocol::NormalizeProtocolEnableName(FName(TEXT("Stemina_Bar"))), FName(TEXT("Stamina_Bar")));

	return true;
}

#endif
