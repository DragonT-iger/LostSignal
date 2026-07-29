#pragma once

#include "CoreMinimal.h"
#include "LSLobbyPanelTypes.generated.h"

// 로비 루트(ULSLobbyMenuWidget)가 배타적으로 여는 패널 종류. 한 번에 하나만 열린다.
//
// 이 값의 "순서"에는 의미가 없다. WidgetSwitcher 인덱스와 대응하지 않으며, 전환은 항상
// UWidgetSwitcher::SetActiveWidget(포인터)로 한다. 과거 "enum 순서 = 스위처 인덱스" 암묵 계약이
// 도달 불가한 데드 값(구 ELSLoadoutTab::Upgrade, ELSLobbyTab::Inventory)을 만들어 온 원인이라 폐기했다.
//
// 패널이 없는 상단 탭(로비/지도/설정)은 여기에 넣지 않는다. 탭 개수와 패널 개수가 다른 것이 정상이며,
// 없는 패널을 위해 값을 만들면 다시 데드 값이 된다.
UENUM(BlueprintType)
enum class ELSLobbyPanel : uint8
{
	None,          // 패널 없음(로비 기본 상태). 스위처 자체를 Collapsed 하고 기본 배경을 보여준다.
	ChipStation,   // 칩 세팅 (칩 스테이션)
	Supply,        // 정비 = 에이베리 보급소 (자판기/제작대)
	SkillLoadout,  // 캐릭터 = 스킬 로드아웃 (액티브/궁극기 선택)
	Bag,           // 가방 = 인벤토리 + 물품창고
	Quest          // 퀘스트
};
