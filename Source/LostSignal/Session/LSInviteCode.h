#pragma once

#include "CoreMinimal.h"

// 친구 방 초대 코드. IPv4 4바이트 + 포트 2바이트(총 48비트)를 Base32로 옮긴 10자 문자열이다.
//
// 서버가 필요 없다 — 코드 자체가 주소를 담는다. 대신 **같은 네트워크(또는 공인 IP 직결)에서만 동작한다.**
// NAT 너머 친구 초대는 별도 계층(Steam 등)이 필요하며 DedicatedServerBuildout.md의 E4가 소유한다.
//
// 포트를 코드에 넣는 이유: 로비는 열 때마다 리슨 서버가 되는데, 7777이 이미 잡혀 있으면 UE가 7778로
// 자동 증가시킨다. 호스트조차 자기 포트를 모르므로 주소만 알려주면 붙지 않는 일이 생긴다.
namespace LSInviteCode
{
	// 코드 본문 길이(하이픈 제외). 48비트를 5비트씩 끊으면 10자다.
	inline constexpr int32 CodeLength = 10;

	// 실패하면 빈 문자열. IPv4가 "a.b.c.d" 형식이 아니거나 포트가 범위를 벗어나면 실패한다.
	LOSTSIGNAL_API FString Encode(const FString& IPv4, int32 Port);

	// 하이픈·공백·대소문자를 무시한다. 헷갈리는 글자(I/L→1, O→0)도 받아준다.
	LOSTSIGNAL_API bool Decode(const FString& Code, FString& OutIPv4, int32& OutPort);

	// 표시용으로 5자씩 끊어 하이픈을 넣는다. (ABCDE-FGHJK)
	LOSTSIGNAL_API FString Format(const FString& Code);
}
