#include "UI/CharacterNode/LSSkillNodeGraphWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSSkillNodeIndex.h"
#include "LostSignal.h"
#include "Rendering/DrawElements.h"
#include "UI/CharacterNode/LSSkillNodeWidget.h"

void ULSSkillNodeGraphWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!NodeCanvas)
	{
		UE_LOG(LogLS, Warning, TEXT("[SkillNode] %s: NodeCanvas가 바인드되지 않았다 - WBP에 CanvasPanel을 두고 이름을 NodeCanvas로 맞춰라"),
			*GetNameSafe(this));
	}

	RefreshGraph();
}

void ULSSkillNodeGraphWidget::SetCharacterID(const int32 InCharacterID)
{
	if (CharacterID == InCharacterID)
	{
		return;
	}

	CharacterID = InCharacterID;
	SelectedNodeKey = NAME_None;

	// 활성 집합은 캐릭터별이다. 비우지 않으면 RefreshGraph 의 "비어 있을 때만 코어를 채운다" 조건이
	// 걸려서 새 캐릭터의 코어가 활성되지 않는다(이전 캐릭터의 코어 키만 남는다).
	// 세이브가 붙으면 이 자리에서 해당 캐릭터의 진행을 읽는다.
	ActivatedNodeKeys.Reset();

	RefreshGraph();
}

void ULSSkillNodeGraphWidget::RefreshGraph()
{
	NodeViews.Reset();
	NormalizedPositions.Reset();

	const UGameInstance* GameInstance = GetGameInstance();
	const ULSGameDataSubsystem* GameData = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	if (!GameData)
	{
		UE_LOG(LogLS, Warning, TEXT("[SkillNode] %s: ULSGameDataSubsystem을 찾을 수 없다"), *GetNameSafe(this));
		return;
	}

	TArray<const FLSSkillNodeRef*> Nodes;
	GameData->GetSkillNodesForCharacter(CharacterID, Nodes, TEXT("LSSkillNodeGraphWidget"));
	if (Nodes.IsEmpty())
	{
		return;
	}

	// 세이브가 붙기 전까지 활성 집합은 코어뿐이다. 상태 판정 규칙 자체는 최종과 같다.
	if (ActivatedNodeKeys.IsEmpty())
	{
		LSSkillNodeViews::CollectAutoActivatedNodeKeys(Nodes, ActivatedNodeKeys);
	}

	LSSkillNodeViews::BuildViews(Nodes, ActivatedNodeKeys, NodeViews);
	LSSkillNodeLayout::ComputeAutoLayout(NodeViews, LayoutParams, NormalizedPositions);

	RebuildNodeWidgets();

	// 다음 틱에 배치가 돌도록 캐시를 비운다. 지금은 아직 지오메트리가 확정되지 않았을 수 있다.
	CachedLocalSize = FVector2D::ZeroVector;
}

void ULSSkillNodeGraphWidget::RebuildNodeWidgets()
{
	if (!NodeCanvas)
	{
		return;
	}

	if (!NodeWidgetClass)
	{
		if (!bLoggedMissingNodeWidgetClass)
		{
			UE_LOG(LogLS, Warning, TEXT("[SkillNode] %s: NodeWidgetClass가 미설정이라 노드를 만들 수 없다"), *GetNameSafe(this));
			bLoggedMissingNodeWidgetClass = true;
		}
		return;
	}

	// 노드 수는 캐릭터가 바뀌어도 같으므로 위젯을 재사용한다(ULSCombatBuffListWidget 풀링과 같은 방식).
	while (NodeWidgets.Num() < NodeViews.Num())
	{
		ULSSkillNodeWidget* NodeWidget = CreateWidget<ULSSkillNodeWidget>(this, NodeWidgetClass);
		if (!NodeWidget)
		{
			break;
		}

		NodeWidget->OnClicked.AddDynamic(this, &ULSSkillNodeGraphWidget::HandleNodeClicked);
		NodeCanvas->AddChildToCanvas(NodeWidget);
		NodeWidgets.Add(NodeWidget);
	}

	for (int32 Index = 0; Index < NodeWidgets.Num(); ++Index)
	{
		ULSSkillNodeWidget* NodeWidget = NodeWidgets[Index];
		if (!NodeWidget)
		{
			continue;
		}

		if (!NodeViews.IsValidIndex(Index))
		{
			NodeWidget->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		NodeWidget->SetVisibility(ESlateVisibility::Visible);
		NodeWidget->SetNodeView(NodeViews[Index]);
		NodeWidget->SetSelected(NodeViews[Index].NodeKey == SelectedNodeKey);
	}
}

void ULSSkillNodeGraphWidget::UpdateNodePositions(const FVector2D& LocalSize)
{
	for (int32 Index = 0; Index < NodeWidgets.Num(); ++Index)
	{
		ULSSkillNodeWidget* NodeWidget = NodeWidgets[Index];
		if (!NodeWidget || !NodeViews.IsValidIndex(Index))
		{
			continue;
		}

		const FVector2D* Normalized = NormalizedPositions.Find(NodeViews[Index].NodeKey);
		if (!Normalized)
		{
			continue;
		}

		UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(NodeWidget->Slot);
		if (!CanvasSlot)
		{
			continue;
		}

		// 앵커를 중앙에 두면 위치가 "중심 기준 오프셋"이 된다.
		// 앵커 분수를 X·Y 에 따로 주는 방식은 리사이즈에 자동 대응하지만 종횡비 보정이 안 되어
		// 링이 타원이 된다. 그래서 오프셋 + 크기 변화 감지로 간다.
		const float Extent = NodeWidget->GetDesiredSlotExtent();
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(FVector2D(Extent, Extent));
		CanvasSlot->SetPosition(LSSkillNodeLayout::ToLocalOffset(*Normalized, LocalSize, FillRatio));
	}
}

void ULSSkillNodeGraphWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	// ULSLayoutRevealWidget 이 여기서 공개 카운트를 돌린다. 반드시 부른다.
	Super::NativeTick(MyGeometry, InDeltaTime);

	const FVector2D LocalSize = MyGeometry.GetLocalSize();
	if (LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f)
	{
		return;
	}

	if (!LocalSize.Equals(CachedLocalSize))
	{
		CachedLocalSize = LocalSize;
		UpdateNodePositions(LocalSize);
	}
}

void ULSSkillNodeGraphWidget::DrawRings(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, const int32 LayerId) const
{
	if (RingThickness <= 0.0f)
	{
		return;
	}

	const FVector2D LocalSize = Geometry.GetLocalSize();
	const FVector2D Center = LocalSize * 0.5f;

	TSet<int32> Rings;
	for (const FLSSkillNodeView& View : NodeViews)
	{
		if (View.Ring > 0)
		{
			Rings.Add(View.Ring);
		}
	}

	for (const int32 Ring : Rings)
	{
		const float Radius = LSSkillNodeLayout::ToLocalRadius(
			LSSkillNodeLayout::GetRingNormalizedRadius(Ring, LayoutParams), LocalSize, FillRatio);

		TArray<FVector2D> Points;
		Points.Reserve(65);
		for (int32 PointIndex = 0; PointIndex <= 64; ++PointIndex)
		{
			const float Angle = (2.0f * UE_PI) * static_cast<float>(PointIndex) / 64.0f;
			Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}

		FSlateDrawElement::MakeLines(
			OutDrawElements, LayerId, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, RingColor, true, RingThickness);
	}
}

void ULSSkillNodeGraphWidget::DrawConnections(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, const int32 LayerId) const
{
	if (ConnectionThickness <= 0.0f)
	{
		return;
	}

	const FVector2D LocalSize = Geometry.GetLocalSize();
	const FVector2D Center = LocalSize * 0.5f;

	// 연결선 하나 = 선행 관계 하나. 별도 연결선 데이터를 두지 않는다.
	for (const FLSSkillNodeView& View : NodeViews)
	{
		const FVector2D* ToNormalized = NormalizedPositions.Find(View.NodeKey);
		if (!ToNormalized)
		{
			continue;
		}

		const bool bToActivated = ActivatedNodeKeys.Contains(View.NodeKey);
		for (const FName Prerequisite : { View.Prerequisite_1, View.Prerequisite_2 })
		{
			if (Prerequisite.IsNone())
			{
				continue;
			}

			const FVector2D* FromNormalized = NormalizedPositions.Find(Prerequisite);
			if (!FromNormalized)
			{
				continue;
			}

			const TArray<FVector2D> Points = {
				Center + LSSkillNodeLayout::ToLocalOffset(*FromNormalized, LocalSize, FillRatio),
				Center + LSSkillNodeLayout::ToLocalOffset(*ToNormalized, LocalSize, FillRatio)
			};

			const bool bActiveEdge = bToActivated && ActivatedNodeKeys.Contains(Prerequisite);
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				LayerId,
				Geometry.ToPaintGeometry(),
				Points,
				ESlateDrawEffect::None,
				bActiveEdge ? ActiveConnectionColor : ConnectionColor,
				true,
				ConnectionThickness);
		}
	}
}

int32 ULSSkillNodeGraphWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	const int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	const bool bParentEnabled) const
{
	// 배경과 연결선을 먼저 그리고, 자식(노드 위젯)을 그 위 레이어에 올린다.
	// ULSCraftingRowWidget 은 반대로 Super 를 먼저 부르고 그 위에 강조를 그린다.
	DrawRings(AllottedGeometry, OutDrawElements, LayerId);
	DrawConnections(AllottedGeometry, OutDrawElements, LayerId);

	return Super::NativePaint(
		Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 1, InWidgetStyle, bParentEnabled);
}

void ULSSkillNodeGraphWidget::HandleNodeClicked(ULSSkillNodeWidget* ClickedNode)
{
	if (!ClickedNode)
	{
		return;
	}

	SelectedNodeKey = ClickedNode->GetNodeKey();
	for (ULSSkillNodeWidget* NodeWidget : NodeWidgets)
	{
		if (NodeWidget)
		{
			NodeWidget->SetSelected(NodeWidget->GetNodeKey() == SelectedNodeKey);
		}
	}
}
