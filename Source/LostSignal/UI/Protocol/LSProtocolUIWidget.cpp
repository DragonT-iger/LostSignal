#include "UI/Protocol/LSProtocolUIWidget.h"

#include "UI/Protocol/LSProtocolWidget.h"

void ULSProtocolUIWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshProtocolUI();
}

void ULSProtocolUIWidget::RefreshProtocolUI()
{
	// [임시 테스트] 4종 프로토콜을 임의값(Level, SynergyStage)으로 채워 표시 확인.
	// 실제 칩 합산/시너지 계산 연동 전까지의 더미 데이터다.
	if (Protocol_Survival)
	{
		Protocol_Survival->SetProtocol(3, 3);   // 생존
	}
	if (Protocol_Carrying)
	{
		Protocol_Carrying->SetProtocol(2, 1);   // 적재
	}
	if (Protocol_Battle)
	{
		Protocol_Battle->SetProtocol(5, 6);     // 전투
	}
	if (Protocol_Navigation)
	{
		Protocol_Navigation->SetProtocol(1, 0); // 탐색
	}
}
