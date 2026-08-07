#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSCharacterPanelWidget.generated.h"

class ULSLobbyTabWidget;
class ULSSkillLoadoutWidget;
class ULSSkillNodeGraphWidget;
class UWidgetSwitcher;

// 캐릭터 패널의 서브탭 종류.
//
// 로비 ELSLobbyPanel 과 같은 이유로 값의 "순서"에는 의미가 없다 — 스위처 인덱스와 대응시키지 않고
// 전환은 항상 UWidgetSwitcher::SetActiveWidget(포인터)로 한다. 아트가 페이지 순서를 바꿔도 깨지지 않는다.
UENUM(BlueprintType)
enum class ELSCharacterSubTab : uint8
{
	SkillLoadout,  // 스킬 로드아웃 (액티브/궁극기 선택)
	NodeGraph      // 강화 노드 그래프
};

/**
 * 로비 캐릭터 탭의 콘텐츠. 스킬 로드아웃과 강화 노드 그래프를 서브탭으로 묶는다.
 *
 * 가방 패널처럼 맨 컨테이너로 두지 않고 클래스를 만든 이유는 두 페이지가 서로 대화해야 하기 때문이다 —
 * 진화 노드는 장착 스킬을 치환하고 강화 노드는 계수를 바꾸므로, 노드를 찍으면 로드아웃 표시가 갱신돼야 한다.
 * 인벤토리와 물품창고는 둘 다 세이브만 읽어서 그 대화가 필요 없었다.
 * (그 배선 자체는 노드 활성화가 붙는 Phase 4에서 넣는다. 지금은 갱신을 유발할 사건이 없다.)
 *
 * 로비 루트와 같은 규약을 쓴다 — 스위처는 WBP 에서 비워 두고 페이지를 런타임에 생성한다.
 * 각 페이지의 큰 위젯 트리가 이 WBP 디자이너에 펼쳐지지 않게 하려는 것이다.
 */
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSCharacterPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 로비가 패널을 열 때 부른다. 활성 서브탭을 최신 데이터로 다시 그린다.
	UFUNCTION(BlueprintCallable, Category="LS/UI|CharacterPanel")
	void RefreshCharacterPanel();

	UFUNCTION(BlueprintCallable, Category="LS/UI|CharacterPanel")
	void ShowSubTab(ELSCharacterSubTab SubTab);

	UFUNCTION(BlueprintPure, Category="LS/UI|CharacterPanel")
	ELSCharacterSubTab GetActiveSubTab() const { return ActiveSubTab; }

protected:
	virtual void NativeConstruct() override;

	// 페이지가 들어갈 스위처. WBP 에서는 비워 둔다(자식이 있으면 경고).
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|CharacterPanel")
	TObjectPtr<UWidgetSwitcher> SubTabSwitcher;

	// 서브탭 버튼. 로비 상단 탭 위젯을 그대로 재사용한다 — 클릭 시 자기 자신을 넘기고 선택 표시를 갖는다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|CharacterPanel")
	TObjectPtr<ULSLobbyTabWidget> SkillLoadoutTab;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|CharacterPanel")
	TObjectPtr<ULSLobbyTabWidget> NodeGraphTab;

	// BP 에서 각 페이지 WBP 를 매핑한다. 에셋 경로를 코드에 하드코딩하지 않는다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|CharacterPanel")
	TSubclassOf<ULSSkillLoadoutWidget> SkillLoadoutPageClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|CharacterPanel")
	TSubclassOf<ULSSkillNodeGraphWidget> NodeGraphPageClass;

	// 표시할 캐릭터. 캐릭터 선택이 생기면 이 값이 단일 출처가 되고 ApplyCharacterToPages 가 전파 지점이다.
	// 스킬 로드아웃은 SkillPool 자산에 캐릭터가 구워져 있어 아직 전파 대상이 아니다(101 고정).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|CharacterPanel")
	int32 CharacterID = 101;

private:
	void CreateSubPages();

	// 캐릭터를 페이지들에 전파하는 유일한 지점. 로드아웃에 세터가 생기면 여기에 한 줄 더한다.
	void ApplyCharacterToPages();

	void RefreshSubTabPage(ELSCharacterSubTab SubTab) const;
	void UpdateSubTabSelection() const;
	UWidget* ResolveSubTabPage(ELSCharacterSubTab SubTab) const;

	UFUNCTION()
	void HandleSubTabClicked(ULSLobbyTabWidget* ClickedTab);

	UPROPERTY(Transient)
	TObjectPtr<ULSSkillLoadoutWidget> SkillLoadoutPage;

	UPROPERTY(Transient)
	TObjectPtr<ULSSkillNodeGraphWidget> NodeGraphPage;

	// 패널을 떠났다 돌아와도 유지한다. 보급소처럼 초기 상태로 되돌리는 것은 그쪽의 명시적 예외다.
	ELSCharacterSubTab ActiveSubTab = ELSCharacterSubTab::SkillLoadout;
};
