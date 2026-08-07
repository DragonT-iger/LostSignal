#pragma once

#include "CoreMinimal.h"
#include "UI/CharacterNode/LSSkillNodeGraphTypes.h"
#include "UI/CharacterNode/LSSkillNodeLayout.h"
#include "UI/Common/LSLayoutRevealWidget.h"
#include "LSSkillNodeGraphWidget.generated.h"

class UCanvasPanel;
class ULSSkillNodeWidget;

/**
 * 캐릭터 강화 노드 그래프 전체.
 *
 * 노드 위젯을 캔버스에 만들어 배치하고, 링 배경 원과 연결선은 자식보다 아래 레이어에 직접 그린다.
 * 좌표 데이터가 없으므로 배치는 LSSkillNodeLayout 의 자동 배치를 쓴다.
 *
 * 이번 단계는 표시와 선택까지다. 활성화·비용 소비·세이브는 아직 없다.
 */
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSSkillNodeGraphWidget : public ULSLayoutRevealWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI|SkillNode")
	void SetCharacterID(int32 InCharacterID);

	// 인덱스를 다시 읽어 노드 위젯을 만들고 배치한다.
	UFUNCTION(BlueprintCallable, Category="LS/UI|SkillNode")
	void RefreshGraph();

	UFUNCTION(BlueprintPure, Category="LS/UI|SkillNode")
	FName GetSelectedNodeKey() const { return SelectedNodeKey; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	// 노드 위젯이 놓이는 캔버스. WBP 에 CanvasPanel 을 두고 이름을 NodeCanvas 로 맞춘다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|SkillNode")
	TObjectPtr<UCanvasPanel> NodeCanvas;

	// BP 에서 WBP_SkillNode 를 매핑한다. 에셋 경로를 코드에 하드코딩하지 않는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|SkillNode")
	TSubclassOf<ULSSkillNodeWidget> NodeWidgetClass;

	// 표시할 캐릭터. 로비에서 캐릭터를 전환하면 SetCharacterID 로 바꾼다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode")
	int32 CharacterID = 101;

	// 위젯 짧은 변 대비 그래프가 차지할 비율. 최외곽 노드가 잘리지 않게 여유를 둔다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Layout", meta=(ClampMin="0.1", ClampMax="1.0"))
	float FillRatio = 0.88f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Layout")
	FLinearColor ConnectionColor = FLinearColor(0.28f, 0.42f, 0.55f, 0.85f);

	// 양쪽 다 활성인 연결선. 활성 경로가 눈에 보이게 한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Layout")
	FLinearColor ActiveConnectionColor = FLinearColor(0.55f, 0.88f, 1.0f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Layout", meta=(ClampMin="0.0"))
	float ConnectionThickness = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Layout")
	FLinearColor RingColor = FLinearColor(0.14f, 0.17f, 0.21f, 0.55f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|SkillNode/Layout", meta=(ClampMin="0.0"))
	float RingThickness = 1.0f;

private:
	void RebuildNodeWidgets();
	void UpdateNodePositions(const FVector2D& LocalSize);
	void DrawRings(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
	void DrawConnections(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

	UFUNCTION()
	void HandleNodeClicked(ULSSkillNodeWidget* ClickedNode);

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSSkillNodeWidget>> NodeWidgets;

	FLSSkillNodeLayoutParams LayoutParams;

	// 노드 키 -> 정규화 좌표. 화면 크기와 무관하다.
	TMap<FName, FVector2D> NormalizedPositions;

	TArray<FLSSkillNodeView> NodeViews;

	// 활성 노드 키. 세이브가 붙기 전까지는 코어만 들어간다.
	TSet<FName> ActivatedNodeKeys;

	FName SelectedNodeKey;

	// 마지막으로 자식 위치를 계산한 크기. 크기가 바뀌면 다시 계산한다.
	FVector2D CachedLocalSize = FVector2D::ZeroVector;

	bool bLoggedMissingNodeWidgetClass = false;
};
