#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "LSLobbyMenuWidget.generated.h"

class UButton;
class UImage;
class UProgressBar;
class UTextBlock;
class UWidgetSwitcher;
class ULSConfirmDialogWidget;
class ULSLobbyTabWidget;
class ULSLobbyQuestWidget;
class ULSLoadoutPreparationWidget;
class ULSSettingsWidget;

// 로비 탭/페이지 종류. WBP의 TabSwitcher에는 현재 Play=0, LoadoutPreparation=1 두 페이지만 있다.
// Quest/Character는 페이지 미구현이라 클릭 시 플레이 페이지를 유지한 채 미구현 안내창만 띄운다.
// Inventory(구 창고+인벤토리 페이지)는 개인정비의 물품창고 탭으로 대체돼 더 이상 진입 경로가 없다.
UENUM(BlueprintType)
enum class ELSLobbyTab : uint8
{
	Play,
	LoadoutPreparation,
	Quest,
	Character,
	Inventory
};

// 로비 메뉴 루트 위젯(WBP_Lobby)의 부모 클래스. 상단 탭 4개, 레벨/진척도, 캐릭터 이미지/이름, 임무 시작 버튼,
// 하단 키 힌트를 묶는다. 탭 콘텐츠 전환은 WidgetSwitcher로 처리하며 현재는 플레이 탭만 연결한다.
// 레벨/이름 등 데이터 연동은 setter만 열어 두고 실제 소스 연결은 추후 작업한다.
// 임무 시작은 싱글 전용으로 GetAuthGameMode 경로를 쓴다. 멀티 전환 시 PlayerController Server RPC로 교체 예정.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSLobbyMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// TAB 키로 개인정비의 물품창고를 토글한다. 포커스 이동에 먹히지 않게 터널링 단계에서 가로챈다.
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 자식이 처리하지 않은 클릭(빈 영역)이 오면 포커스를 루트로 회수한다.
	// 포커스가 뷰포트로 빠지면 TAB/ESC 키가 위젯에 더 이상 들어오지 않아 동작이 들쭉날쭉해진다.
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// "LV.1" 처럼 레벨 문자열 전체를 그대로 표시한다.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Lobby")
	void SetLevelText(const FText& NewLevelText) const;

	// 0.0~1.0 진척도. 범위를 벗어나면 클램프한다.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Lobby")
	void SetLevelProgress(float Progress) const;

	UFUNCTION(BlueprintCallable, Category="LS/UI|Lobby")
	void SetPlayerName(const FText& NewPlayerName) const;

protected:
	// 상단 탭 4개. WBP_LobbyTab을 각각 배치하며 모두 클릭을 구독해 해당 페이지로 전환한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> PlayTab;

	// 개인정비 탭. 클릭 시 WidgetSwitcher를 개인정비(WBP_LoadoutPreparation) 페이지로 전환한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> LoadoutPreparation;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> QuestTab;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> CharacterTab;

	// 탭별 콘텐츠 전환. 인덱스는 ELSLobbyTab 순서를 따른다. 현재는 플레이(0)만 채운다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UWidgetSwitcher> TabSwitcher;

	// 개인정비 페이지(TabSwitcher 인덱스 1)에 배치된 위젯. WBP_Lobby의 인스턴스 이름과 일치해야 바인딩된다.
	// TAB 키의 물품창고 직행, ESC 단계별 뒤로가기, 페이지 재진입 시 탭 목록 리셋에 쓴다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLoadoutPreparationWidget> WBP_LoadoutPreparation;

	// 로비 배경 이미지. 개인정비 페이지에서 배경을 교체하기 위해 바인딩한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UImage> BackgroundImage;

	// 개인정비 페이지에서 사용할 배경 브러시. BP 클래스 디폴트에서 텍스처/머티리얼을 매핑한다.
	// 리소스가 비어 있으면 교체하지 않고 기본 배경을 유지한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	FSlateBrush LoadoutPreparationBackgroundBrush;

	// 퀘스트 탭(인덱스 2) 콘텐츠. 메인 1 + 서브 3 퀘스트를 묶는 패널. 데이터 연동은 추후 작업.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyQuestWidget> LobbyQuest;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UProgressBar> LevelProgressBar;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UTextBlock> PlayerNameText;

	// 본인 이미지 선택용. 이미지는 버튼 자체에 넣으므로 별도 Image는 바인딩하지 않는다. 선택 로직은 추후 작업.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UButton> CharacterPortraitButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UButton> MissionStartButton;

	// 하단 좌측 키 힌트. ESC 뒤로 / TAB 인벤토리. 배경 이미지가 달라 위젯을 나누지 않고 버튼으로 둔다.
	// 현재는 바인딩만 하며 클릭 동작은 추후 연결한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UButton> BackButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UButton> InventoryButton;

	// 세팅 화면(WBP_Settings) 진입 버튼.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UButton> SettingsButton;

	// BP(WBP_Lobby)에서 WBP_Settings를 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	TSubclassOf<ULSSettingsWidget> SettingsWidgetClass;

	// 미구현 안내창용. BP(WBP_LobbyMenu) 클래스 디폴트에서 WBP_ConfirmDialog를 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	TSubclassOf<ULSConfirmDialogWidget> ConfirmDialogClass;

private:
	UFUNCTION()
	void HandlePlayTabClicked();

	// 개인정비 탭. WidgetSwitcher를 개인정비(WBP_LoadoutPreparation) 페이지로 전환한다.
	UFUNCTION()
	void HandleLoadoutPreparationClicked();

	// 퀘스트/캐릭터변경 탭은 페이지 미구현. 플레이 페이지를 유지한 채 미구현 안내창을 띄운다.
	UFUNCTION()
	void HandleQuestTabClicked();

	UFUNCTION()
	void HandleCharacterTabClicked();

	// 미구현 안내창이 닫히면 참조를 정리하고 TAB/ESC가 다시 로비로 오도록 포커스를 회수한다.
	UFUNCTION()
	void HandleNotImplementedDialogClosed();

	// 인벤토리 버튼. TAB 키와 동일하게 개인정비의 물품창고를 토글한다.
	UFUNCTION()
	void HandleInventoryButtonClicked();

	UFUNCTION()
	void HandleMissionStartClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	// Settings 화면 Back 클릭 시: 참조만 정리(위젯 자체는 이미 스스로 닫혔다).
	UFUNCTION()
	void HandleSettingsBackToMenu();

	// 탭 콘텐츠 전환을 ELSLobbyTab 순서(인덱스)대로 적용한다.
	void ShowTab(ELSLobbyTab Tab) const;

	// 현재 탭에 맞춰 배경을 교체한다. 개인정비 페이지는 전용 배경, 그 외에는 기본 배경으로 복원.
	void UpdateBackground(ELSLobbyTab Tab) const;

	// TAB 동작: 개인정비 콘텐츠가 열려 있으면 탭 목록으로, 아니면 개인정비+물품창고로 바로 진입.
	// 플레이로는 돌아가지 않는다(콘텐츠 → 탭 목록 → 물품창고 순환).
	void ToggleStoragePage() const;

	// Settings 화면을 생성해 뷰포트에 띄운다. 이미 떠 있으면 nullptr 반환.
	ULSSettingsWidget* ShowSettingsWidget();

	// "아직 구현되지 않았습니다" 안내창을 띄운다. 이미 떠 있으면 중복 생성하지 않는다.
	void ShowNotImplementedNotice();

	UPROPERTY(Transient)
	TObjectPtr<ULSSettingsWidget> ActiveSettingsWidget;

	UPROPERTY(Transient)
	TObjectPtr<ULSConfirmDialogWidget> ActiveConfirmDialog;

	// WBP에서 설정한 기본 배경 브러시. NativeConstruct에서 캐시해 다른 탭으로 돌아올 때 복원한다.
	UPROPERTY(Transient)
	FSlateBrush DefaultBackgroundBrush;
};
