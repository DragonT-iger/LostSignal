#include "Session/LSInviteCode.h"

namespace
{
// Crockford Base32 — 헷갈리는 I, L, O, U를 뺀 32글자.
const TCHAR* LSInviteCodeAlphabet = TEXT("0123456789ABCDEFGHJKMNPQRSTVWXYZ");
constexpr int32 LSInviteCodeBitsPerChar = 5;
constexpr int32 LSInviteCodePayloadBits = 48;	// IPv4 32비트 + 포트 16비트

// 알파벳에 없는 글자를 최대한 받아준다(손으로 옮겨 적는 코드라 오독이 흔하다).
int32 LSInviteCodeCharToValue(TCHAR Char)
{
	Char = FChar::ToUpper(Char);
	switch (Char)
	{
	case TEXT('I'): case TEXT('L'): return 1;
	case TEXT('O'): return 0;
	case TEXT('U'): return 27;	// V로 취급
	default: break;
	}

	for (int32 Index = 0; Index < 32; ++Index)
	{
		if (LSInviteCodeAlphabet[Index] == Char)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}
}

FString LSInviteCode::Encode(const FString& IPv4, const int32 Port)
{
	TArray<FString> Octets;
	IPv4.ParseIntoArray(Octets, TEXT("."));
	if (Octets.Num() != 4 || Port <= 0 || Port > 65535)
	{
		return FString();
	}

	uint64 Payload = 0;
	for (const FString& Octet : Octets)
	{
		const int32 Value = FCString::Atoi(*Octet);
		if (Value < 0 || Value > 255)
		{
			return FString();
		}
		Payload = (Payload << 8) | static_cast<uint64>(Value);
	}
	Payload = (Payload << 16) | static_cast<uint64>(Port);

	// 최상위 비트부터 5비트씩 끊는다. 48비트는 5로 나누어떨어지지 않으므로 마지막 2비트는 0으로 채운다.
	FString Code;
	Code.Reserve(CodeLength);
	for (int32 CharIndex = 0; CharIndex < CodeLength; ++CharIndex)
	{
		const int32 Shift = LSInviteCodePayloadBits - (CharIndex + 1) * LSInviteCodeBitsPerChar;
		const uint64 Chunk = (Shift >= 0) ? (Payload >> Shift) : (Payload << -Shift);
		Code.AppendChar(LSInviteCodeAlphabet[Chunk & 0x1F]);
	}

	return Code;
}

bool LSInviteCode::Decode(const FString& Code, FString& OutIPv4, int32& OutPort)
{
	FString Compact;
	Compact.Reserve(Code.Len());
	for (const TCHAR Char : Code)
	{
		if (Char != TEXT('-') && Char != TEXT(' '))
		{
			Compact.AppendChar(Char);
		}
	}

	if (Compact.Len() != CodeLength)
	{
		return false;
	}

	uint64 Payload = 0;
	for (int32 CharIndex = 0; CharIndex < CodeLength; ++CharIndex)
	{
		const int32 Value = LSInviteCodeCharToValue(Compact[CharIndex]);
		if (Value == INDEX_NONE)
		{
			return false;
		}

		const int32 Shift = LSInviteCodePayloadBits - (CharIndex + 1) * LSInviteCodeBitsPerChar;
		if (Shift >= 0)
		{
			Payload |= static_cast<uint64>(Value) << Shift;
		}
		else
		{
			// 마지막 글자의 남는 하위 비트는 인코딩에서 0으로 채웠으므로 그대로 버린다.
			Payload |= static_cast<uint64>(Value) >> -Shift;
		}
	}

	OutPort = static_cast<int32>(Payload & 0xFFFF);
	const uint64 AddressBits = (Payload >> 16) & 0xFFFFFFFF;
	OutIPv4 = FString::Printf(TEXT("%llu.%llu.%llu.%llu"),
		(AddressBits >> 24) & 0xFF,
		(AddressBits >> 16) & 0xFF,
		(AddressBits >> 8) & 0xFF,
		AddressBits & 0xFF);

	return OutPort > 0;
}

FString LSInviteCode::Format(const FString& Code)
{
	if (Code.Len() != CodeLength)
	{
		return Code;
	}

	return Code.Left(5) + TEXT("-") + Code.Mid(5);
}
