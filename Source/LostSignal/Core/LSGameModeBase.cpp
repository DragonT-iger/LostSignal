// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/LSGameModeBase.h"
#include "Core/LSPlayerControllerBase.h"
#include "Core/LSPlayerState.h"
#include "Characters/LSCharacterBase.h"

ALSGameModeBase::ALSGameModeBase()
{
	bUseSeamlessTravel = true;

	// 레이드 입장 payload는 PlayerState에 실려 seamless travel을 건너간다(ALSPlayerState::CopyProperties).
	// 이 클래스 지정을 빼면 파밍 레벨에서 payload가 사라져 인벤토리가 빈 채로 시작된다.
	PlayerStateClass = ALSPlayerState::StaticClass();

	//PlayerControllerClass = ALSPlayerControllerBase::StaticClass();
	//DefaultPawnClass = ALSCharacterBase::StaticClass();
}
