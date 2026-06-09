#include "Data/LSProtocolTypes.h"

namespace
{
bool RowNameStartsWith(const FString& RowName, const TCHAR* Prefix)
{
	return RowName.StartsWith(Prefix, ESearchCase::IgnoreCase);
}
}

namespace LSProtocol
{
const TCHAR* GetProtocolRowNamePrefix(const ELSProtocolType ProtocolType)
{
	switch (ProtocolType)
	{
	case ELSProtocolType::Survival:
		return TEXT("Protocol_Survival_");
	case ELSProtocolType::Carrying:
		return TEXT("Protocol_Carrying_");
	case ELSProtocolType::Battle:
		return TEXT("Protocol_Battle_");
	case ELSProtocolType::Navigation:
		return TEXT("Protocol_Navigation_");
	default:
		return TEXT("");
	}
}

bool ResolveProtocolTypeFromRowName(const FName RowName, ELSProtocolType& OutProtocolType)
{
	const FString RowNameString = RowName.ToString();
	if (RowNameStartsWith(RowNameString, TEXT("Protocol_Survival_")))
	{
		OutProtocolType = ELSProtocolType::Survival;
		return true;
	}
	if (RowNameStartsWith(RowNameString, TEXT("Protocol_Carrying_")))
	{
		OutProtocolType = ELSProtocolType::Carrying;
		return true;
	}
	if (RowNameStartsWith(RowNameString, TEXT("Protocol_Battle_")))
	{
		OutProtocolType = ELSProtocolType::Battle;
		return true;
	}
	if (RowNameStartsWith(RowNameString, TEXT("Protocol_Navigation_")))
	{
		OutProtocolType = ELSProtocolType::Navigation;
		return true;
	}

	return false;
}

FName NormalizeProtocolEnableName(const FName EnableName)
{
	FString NameString = EnableName.ToString();
	NameString.ReplaceInline(TEXT("Stemina"), TEXT("Stamina"), ESearchCase::IgnoreCase);
	return FName(*NameString);
}
}
