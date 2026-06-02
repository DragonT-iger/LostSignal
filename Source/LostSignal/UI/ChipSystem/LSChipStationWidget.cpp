#include "UI/ChipSystem/LSChipStationWidget.h"

void ULSChipStationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshChipStation();
}

void ULSChipStationWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

void ULSChipStationWidget::RefreshChipStation_Implementation()
{
	// 기본 구현은 비어 있다. 실제 칩 장착/메모리/신호 게이지 갱신은
	// 블루프린트(WBP_ChipStation) 또는 파생 클래스에서 처리한다.
}
