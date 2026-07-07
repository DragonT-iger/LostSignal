#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSLoadoutPreparationWidget.generated.h"

class UWidgetSwitcher;
class ULSConfirmDialogWidget;
class ULSLobbyTabWidget;

// 개인정비 내부 탭/페이지 종류. ContentSwitcher 인덱스와 순서를 맞춘다(Chip=0, Storage=1, Supply=2, Upgrade=3, Skill=4).
UENUM(BlueprintType)
enum class ELSLoadoutTab : uint8
{
	Chip,     // 칩 강화/합성/장착 (칩 스테이션)
	Storage,  // 물품창고 (인벤토리 창고)
	Supply,   // 에이베리 보급소 (상점/제작) — 아직 안 만들어서 나중에 구현할 예정입니다
	Upgrade,  // 업그레이드 (캐릭터/기지 강화) — 아직 안 만들어서 나중에 구현할 예정입니다
	Skill     // 스킬 로드아웃 (액티브/궁극기 3칸 선택). ContentSwitcher 마지막(인덱스 4)에 배치.
};

// 개인정비 화면(WBP_LoadoutPreparation)의 부모 클래스. 로비 개인정비 페이지 안에 배치되며, 내부 탭 4개
// (칩 강화·합성·장착/물품창고/물품 수령/캐릭터·기지 강화)를 ContentSwitcher로 전환한다.
// 탭 버튼은 로비 상단 탭과 동일한 ULSLobbyTabWidget을 재사용한다. 각 페이지의 실제 콘텐츠(칩 스테이션,
// 창고 위젯 등)는 WBP 슬롯에 아트가 배치한다.
// 동작: 처음에는 탭 4개만 보이고 ContentSwitcher는 숨긴다. 탭을 클릭하면 탭이 사라지고 해당 콘텐츠가 열린다.
// 가시성은 C++에서 제어하므로 WBP 디자이너의 Visibility 설정과 무관하게 동작한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSLoadoutPreparationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 지정한 내부 탭 콘텐츠를 바로 연다. 로비 TAB 키(물품창고 직행) 등 외부 진입용.
	void OpenTab(ELSLoadoutTab Tab) const;

	// 탭 목록만 보이는 초기 상태로 되돌린다. 콘텐츠는 숨긴다.
	void ResetToTabs() const;

	// 내부 콘텐츠(ContentSwitcher)가 열려 있는지. TAB/ESC 단계별 뒤로가기 판단용.
	bool IsContentOpen() const;

	// 미구현 안내창이 떠 있는지. 로비 루트의 포커스 회수 가드가 안내창의 키 입력(ESC/TAB)을 뺏지 않게 참조한다.
	bool HasActiveConfirmDialog() const;

	// 미구현 안내창이 떠 있으면 취소와 동일하게 닫는다. 탭 전환 시 로비/내부 탭 핸들러가 호출한다.
	void CloseActiveConfirmDialog();

protected:
	// 내부 탭 콘텐츠 전환. 인덱스는 ELSLoadoutTab 순서를 따른다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Loadout")
	TObjectPtr<UWidgetSwitcher> ContentSwitcher;

	// 칩 강화/합성/장착 탭. 칩 스테이션 페이지(인덱스 0)로 전환한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Loadout")
	TObjectPtr<ULSLobbyTabWidget> ChipTab;

	// 물품창고 탭. 인벤토리 창고 페이지(인덱스 1)로 전환한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Loadout")
	TObjectPtr<ULSLobbyTabWidget> StorageTab;

	// 에이베리 보급소(상점/제작) 탭. 콘텐츠 미구현이라 클릭 시 미구현 안내창만 띄운다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Loadout")
	TObjectPtr<ULSLobbyTabWidget> SupplyTab;

	// 업그레이드(캐릭터/기지 강화) 탭. 전용 스킬 탭 버튼이 없어서, 이 버튼으로 스킬 로드아웃 페이지(Skill)를 연다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Loadout")
	TObjectPtr<ULSLobbyTabWidget> UpgradeTab;

	// 미구현 안내창용. BP(WBP_LoadoutPreparation) 클래스 디폴트에서 WBP_ConfirmDialog를 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Loadout")
	TSubclassOf<ULSConfirmDialogWidget> ConfirmDialogClass;

private:
	UFUNCTION()
	void HandleSupplyTabClicked();

	UFUNCTION()
	void HandleStorageTabClicked();

	UFUNCTION()
	void HandleUpgradeTabClicked();

	UFUNCTION()
	void HandleChipTabClicked();

	// 미구현 안내창이 닫히면 참조를 정리하고 TAB/ESC가 다시 로비로 오도록 포커스를 회수한다.
	UFUNCTION()
	void HandleNotImplementedDialogClosed();

	// 내부 콘텐츠 전환을 ELSLoadoutTab 순서(인덱스)대로 적용한다. 탭을 숨기고 콘텐츠를 보인다.
	void ShowTab(ELSLoadoutTab Tab) const;

	// 탭 4개의 표시 여부를 일괄 적용한다. 콘텐츠가 열리면 탭을 숨긴다.
	void SetTabBarVisible(bool bVisible) const;

	// "아직 구현되지 않았습니다" 안내창을 띄운다. 이미 떠 있으면 중복 생성하지 않는다.
	void ShowNotImplementedNotice();

	UPROPERTY(Transient)
	TObjectPtr<ULSConfirmDialogWidget> ActiveConfirmDialog;
};
