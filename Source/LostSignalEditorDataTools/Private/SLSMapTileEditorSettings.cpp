#include "SLSMapTileEditor.h"

#include "LSMapTileEditorModel.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SLSMapTileEditorSettings"

TSharedRef<SWidget> SLSMapTileEditor::BuildSettingsPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock).Text(LOCTEXT("SettingsTitle", "배치 설정")).Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 2.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("PaintToolLabel", "칠하기 도구"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return Model->GetPaintTool() == ELSMapTilePaintTool::Brush
						? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
					{
						if (State == ECheckBoxState::Checked)
						{
							Model->SetPaintTool(ELSMapTilePaintTool::Brush);
						}
					})
					[SNew(STextBlock).Text(LOCTEXT("BrushToolLabel", "한 칸 브러시"))]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f)
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]() { return Model->GetPaintTool() == ELSMapTilePaintTool::Rectangle
						? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
					{
						if (State == ECheckBoxState::Checked)
						{
							Model->SetPaintTool(ELSMapTilePaintTool::Rectangle);
						}
					})
					[SNew(STextBlock).Text(LOCTEXT("RectangleToolLabel", "직사각형"))]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 2.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("GridSizeLabel", "셀 크기"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SNumericEntryBox<float>)
				.Value_Lambda([this]() { return Model->GetGridSize(); })
				.OnValueCommitted_Lambda([this](float Value, ETextCommit::Type)
				{
					Model->SetGridSize(Value);
					RefreshMap();
				})
				.MinValue(1.0f)
				.AllowSpin(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 2.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("YawRotationLabel", "회전 Yaw (도)"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SNumericEntryBox<float>)
				.Value_Lambda([this]() { return Model->GetPaintYawDegrees(); })
				.OnValueChanged_Lambda([this](float Value) { Model->SetPaintYawDegrees(Value); })
				.MinValue(0.0f)
				.MaxValue(359.0f)
				.Delta(90.0f)
				.AllowSpin(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return Model->ShouldAlignTopSurface() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { Model->SetAlignTopSurface(State == ECheckBoxState::Checked); })
				[SNew(STextBlock).Text(LOCTEXT("AlignTopLabel", "기존 윗면 높이 유지"))]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f)
			[
				SNew(STextBlock).Text_Lambda([this]() { return Model->GetStatusText(); }).AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ControlHelp", "한 칸 브러시: 클릭/연속 드래그\n직사각형: 드래그 범위 채우기\nR: 시계 방향 90도 회전\nAlt+좌클릭: 스포이드\n휠: 확대/축소\n가운데 버튼 드래그: 이동\nCtrl+Z: 되돌리기"))
				.AutoWrapText(true)
			]
		];
}

#undef LOCTEXT_NAMESPACE
