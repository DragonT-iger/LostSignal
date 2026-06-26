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
	// 프로토콜 표시용 한글 이름("생존 프로토콜" 등). 이름 텍스트의 단일 출처.
	LOSTSIGNAL_API FText GetProtocolDisplayName(ELSProtocolType ProtocolType);
}
