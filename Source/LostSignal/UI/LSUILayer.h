// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// UMG 뷰포트 레이어 Z-order 단일 출처.
// 같은 그룹의 위젯은 같은 Z를 쓰고, 그룹 간 깊이는 이 값으로만 결정한다.
// 새 최상위 위젯을 AddToViewport 할 때는 리터럴 대신 여기 값을 쓴다.
namespace LSUILayer
{
	// 상시 게임플레이 HUD(체력/스태미나 등). 엔진 기본 Z(0)와 동일하며, 모달 백드롭보다 아래라
	// 패널이 열리면 HUD도 함께 블러된다. (HUD를 또렷이 유지하려면 ModalBackdrop보다 큰 값으로 올린다.)
	constexpr int32 HUD = 0;

	// 인벤토리/창고/칩스테이션/루트드랍 같은 모달 패널 뒤에 깔리는 공유 블러 백드롭.
	constexpr int32 ModalBackdrop = 100;

	// 모달 패널 본체. 백드롭 위, 디버그 오버레이 아래.
	constexpr int32 ModalPanel = 200;

	// 시연용 프로토콜 디버그 패널. 항상 최상단.
	constexpr int32 ProtocolDebug = 1000;
}
