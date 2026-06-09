#pragma once

#include "CoreMinimal.h"
#include "LSProtocolTypes.generated.h"

UENUM(BlueprintType)
enum class ELSProtocolType : uint8
{
	Survival,
	Carrying,
	Battle,
	Navigation
};

namespace LSProtocol
{
	LOSTSIGNAL_API const TCHAR* GetProtocolRowNamePrefix(ELSProtocolType ProtocolType);
	LOSTSIGNAL_API bool ResolveProtocolTypeFromRowName(FName RowName, ELSProtocolType& OutProtocolType);
	LOSTSIGNAL_API FName NormalizeProtocolEnableName(FName EnableName);
}
