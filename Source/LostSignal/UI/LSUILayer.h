// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// UMG 뷰포트 레이어 Z-order 단일 출처.
// 같은 그룹의 위젯은 같은 Z를 쓰고, 그룹 간 깊이는 이 값으로만 결정한다.
// 새 최상위 위젯을 AddToViewport 할 때는 리터럴 대신 여기 값을 쓴다.
namespace LSUILayer
{
	// 상시 게임플레이 HUD(체력/스태미나 등). 엔진 기본 Z(0)와 동일하다.
	constexpr int32 HUD = 0;

	// ---- 로비 레벨 전용 ----
	// 로비 메뉴 본체(WBP_Lobby).
	constexpr int32 LobbyMenu = 10;

	// 모달 패널 본체(창고/칩스테이션/루트드랍 컨테이너). 인벤토리 아래.
	constexpr int32 ModalPanel = 200;

	// 인벤토리 본체. 컨테이너 패널과 함께 떠도 항상 위에 그려져, 컨테이너 WBP의 배경이
	// 인벤토리를 덮지 않게 한다. (같은 Z면 뷰포트 삽입 순서에 휘둘려 덮이는 경우가 생김)
	constexpr int32 ModalPanelInventory = 300;

	// 모달 패널(칩스테이션/인벤토리 등)에서 띄우는 확인/알림 다이얼로그(WBP_ConfirmDialog). 모달 패널 본체 위, 세팅 아래.
	constexpr int32 ModalPanelDialog = 320;

	// 세팅 화면(WBP_Settings). 타이틀/로비/레이드(ESC) 등 여러 레벨에서 공용으로 띄우는
	// 최상위 오버레이라 그 레벨의 다른 레이어(로비 메뉴/모달 패널 포함)보다 위에 둔다.
	constexpr int32 Settings = 400;

	// 세팅 화면 안에서 뜨는 서브 패널(WBP_Sound, WBP_ConfirmDialog 등). Settings보다 위.
	constexpr int32 SettingsSubPanel = 410;

	// 시연용 프로토콜 디버그 패널.
	constexpr int32 ProtocolDebug = 1000;

	// 커서를 따라다니는 호버 툴팁. 모든 패널/디버그 위에 떠야 하므로 최상단.
	constexpr int32 Tooltip = 2000;
}
