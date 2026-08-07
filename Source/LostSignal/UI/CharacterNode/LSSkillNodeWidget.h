#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/CharacterNode/LSSkillNodeGraphTypes.h"
#include "LSSkillNodeWidget.generated.h"

class ULSSkillNodeWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLSSkillNodeClicked, ULSSkillNodeWidget*, ClickedNode);

/**
 * 노드 하나. 도형과 상태를 NativePaint 로 직접 그린다.
 *
 * 아직 BindWidget 이 없다 — 아트가 붙을 때 아이콘·이름을 필수 BindWidget 으로 추가한다.
 * BindWidgetOptional 로 "있으면 쓰고 없으면 넘기는" 구조를 만들지 않는다(프로젝트 금지 사항).
 * 그때까지는 이 클래스가 전부 그리므로 WBP 의 위젯 트리를 비워둬도 화면이 나온다.
 *
 * 배치와 크기는 ULSSkillNodeGraphWidget 이 정한다. 이 위젯은 자기 지오메트리 중앙에 그린다.
 */
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSSkillNodeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 클릭된 위젯 자신을 넘긴다(ULSCraftingRowWidget 과 같은 형태).
	UPROPERTY(BlueprintAssignable, Category="LS/UI|SkillNode")
	FLSSkillNodeClicked OnClicked;

	void SetNodeView(const FLSSkillNodeView& InView);
	void SetSelected(bool bInSelected);

	const FLSSkillNodeView& GetNodeView() const { return NodeView; }
	FName GetNodeKey() const { return NodeView.NodeKey; }

	/** 도형 반지름(픽셀). 그래프 위젯이 캔버스 슬롯 크기를 이 값으로 정한다. */
	float GetShapeRadius() const;

	/** 슬롯 한 변의 길이. 도형 지름에 외곽선·강조 두께를 더한 값이다. */
	float GetDesiredSlotExtent() const;

protected:
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 도형 반지름(픽셀). 종류별로 크기가 달라 기획 그림의 위계가 드러난다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Shape", meta=(ClampMin="1.0"))
	float CoreRadius = 26.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Shape", meta=(ClampMin="1.0"))
	float MainStatRadius = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Shape", meta=(ClampMin="1.0"))
	float SubStatRadius = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Shape", meta=(ClampMin="1.0"))
	float EnhanceRadius = 17.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Shape", meta=(ClampMin="1.0"))
	float EvolveRadius = 19.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Shape", meta=(ClampMin="0.0"))
	float OutlineThickness = 2.0f;

	// 상태별 채움색. 잠김 -> 해금가능 -> 활성 순으로 밝아진다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Color")
	FLinearColor LockedFillColor = FLinearColor(0.06f, 0.07f, 0.09f, 0.92f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Color")
	FLinearColor AvailableFillColor = FLinearColor(0.10f, 0.16f, 0.22f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Color")
	FLinearColor ActivatedFillColor = FLinearColor(0.12f, 0.55f, 0.72f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Color")
	FLinearColor LockedOutlineColor = FLinearColor(0.20f, 0.22f, 0.25f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Color")
	FLinearColor AvailableOutlineColor = FLinearColor(0.35f, 0.75f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Color")
	FLinearColor ActivatedOutlineColor = FLinearColor(0.65f, 0.92f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Color")
	FLinearColor HoveredOutlineColor = FLinearColor(1.0f, 0.85f, 0.35f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Color")
	FLinearColor SelectedOutlineColor = FLinearColor(1.0f, 0.42f, 0.08f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Color", meta=(ClampMin="0.0"))
	float SelectedOutlineThickness = 3.0f;

	// 노드 키를 도형 위에 겹쳐 그린다. 배치를 눈으로 검증할 때만 켠다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Debug")
	bool bDrawDebugKey = false;

private:
	// 종류별 도형의 변 수. 원은 근사 다각형으로 그린다.
	int32 GetShapeSideCount() const;

	FLinearColor GetFillColor() const;
	FLinearColor GetOutlineColor() const;

	FLSSkillNodeView NodeView;
	bool bIsSelected = false;

	// UWidget::IsHovered() 대신 직접 추적한다. 호버가 바뀔 때 Paint 무효화를 확실히 걸기 위한 것이다.
	bool bIsPointerOver = false;
};
