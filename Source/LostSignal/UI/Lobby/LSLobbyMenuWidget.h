#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSLobbyMenuWidget.generated.h"

class UButton;
class UProgressBar;
class UTextBlock;
class UWidgetSwitcher;
class ULSLobbyTabWidget;
class ULSLobbyQuestWidget;

// 로비 탭/페이지 종류. WidgetSwitcher 인덱스와 순서를 맞춘다(Play=0, Equip=1, Quest=2, Character=3, Inventory=4).
// Inventory는 상단 탭이 아니라 TAB 키/인벤토리 버튼으로 여는 창고+인벤토리 페이지다.
UENUM(BlueprintType)
enum class ELSLobbyTab : uint8
{
	Play,
	Equip,
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

	// TAB 키로 인벤토리 페이지를 토글한다. 포커스 이동에 먹히지 않게 터널링 단계에서 가로챈다.
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// "LV.1" 처럼 레벨 문자열 전체를 그대로 표시한다.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Lobby")
	void SetLevelText(const FText& NewLevelText) const;

	// 0.0~1.0 진척도. 범위를 벗어나면 클램프한다.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Lobby")
	void SetLevelProgress(float Progress) const;

	UFUNCTION(BlueprintCallable, Category="LS/UI|Lobby")
	void SetPlayerName(const FText& NewPlayerName) const;

protected:
	// 상단 탭 4개. WBP_LobbyTab을 각각 배치한다. 현재는 플레이 탭만 클릭을 구독한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> PlayTab;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> EquipTab;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> QuestTab;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> CharacterTab;

	// 탭별 콘텐츠 전환. 인덱스는 ELSLobbyTab 순서를 따른다. 현재는 플레이(0)만 채운다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UWidgetSwitcher> TabSwitcher;

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

private:
	UFUNCTION()
	void HandlePlayTabClicked();

	// 개인정비 탭. WidgetSwitcher를 개인정비(칩 스테이션) 페이지로 전환한다.
	UFUNCTION()
	void HandleEquipTabClicked();

	// 인벤토리 버튼. TAB 키와 동일하게 인벤토리(창고+인벤토리) 페이지를 토글한다.
	UFUNCTION()
	void HandleInventoryButtonClicked();

	UFUNCTION()
	void HandleMissionStartClicked();

	// 탭 콘텐츠 전환을 ELSLobbyTab 순서(인덱스)대로 적용한다.
	void ShowTab(ELSLobbyTab Tab) const;

	// 인벤토리 페이지 토글: 현재 인벤토리면 플레이로, 아니면 인벤토리로 전환.
	void ToggleInventoryTab() const;
};
