#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSLoadoutPreparationWidget.generated.h"

class UWidgetSwitcher;
class ULSConfirmDialogWidget;
class ULSLobbyTabWidget;

// 개인정비 내부 탭/페이지 종류. ContentSwitcher 인덱스와 순서를 맞춘다(Chip=0, Storage=1, Supply=2, Upgrade=3).
UENUM(BlueprintType)
enum class ELSLoadoutTab : uint8
{
	Chip,     // 칩 강화/합성/장착 (칩 스테이션)
	Storage,  // 물품창고 (인벤토리 창고)
	Supply,   // 에이베리 보급소 (상점/제작) — 아직 안 만들어서 나중에 구현할 예정입니다
	Upgrade   // 업그레이드 (캐릭터/기지 강화) — 아직 안 만들어서 나중에 구현할 예정입니다
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
	void OpenTab(ELSLoadoutTab Tab);

	// 사용자가 콘텐츠를 닫을 때(ESC/TAB) 사용. 열려 있던 페이지가 디졸브 가능하면 좌우 노이즈로 사라진 뒤
	// 콘텐츠를 숨기고, 아니면 즉시 숨긴다.
	void ResetToTabs();

	// 초기화/페이지 진입처럼 연출 없이 즉시 탭 목록만 보이게 한다(디졸브 미재생).
	void CollapseContentToTabs();

	// 내부 콘텐츠(ContentSwitcher)가 열려 있는지. TAB/ESC 단계별 뒤로가기 판단용.
	bool IsContentOpen() const;

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

	// 업그레이드(캐릭터/기지 강화) 탭. 콘텐츠 미구현이라 클릭 시 미구현 안내창만 띄운다.
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

	// 내부 콘텐츠 전환. 콘텐츠가 이미 열려 있고 다른 페이지로 바꾸면, 현재 페이지를 디졸브로 내보낸 뒤 전환한다.
	// 닫힌 상태(탭 목록)에서 여는 경우엔 연출 없이 바로 보인다.
	void ShowTab(ELSLoadoutTab Tab);

	// 실제로 스위처를 해당 인덱스로 전환하고, 디졸브로 사라졌던 페이지의 노이즈/가시성을 복구한다. 탭바는 숨긴다.
	void ApplyActivePage(int32 PageIndex);

	// 콘텐츠 페이지의 디졸브 연출이 끝나면 호출된다. 대기 중인 전환 인덱스가 있으면 그 페이지로 전환, 없으면 탭 목록으로.
	UFUNCTION()
	void HandleContentDissolveFinished();

	// 탭 4개의 표시 여부를 일괄 적용한다. 콘텐츠가 열리면 탭을 숨긴다.
	void SetTabBarVisible(bool bVisible) const;

	// "아직 구현되지 않았습니다" 안내창을 띄운다. 이미 떠 있으면 중복 생성하지 않는다.
	void ShowNotImplementedNotice();

	UPROPERTY(Transient)
	TObjectPtr<ULSConfirmDialogWidget> ActiveConfirmDialog;

	// 디졸브 완료 후 전환할 대상 페이지 인덱스. INDEX_NONE이면 전환이 아니라 닫기(탭 목록 복귀)다.
	int32 PendingSwitchIndex = INDEX_NONE;
};
