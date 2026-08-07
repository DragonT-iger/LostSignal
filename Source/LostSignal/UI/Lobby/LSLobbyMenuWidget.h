#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Lobby/LSLobbyPanelTypes.h"
#include "LSLobbyMenuWidget.generated.h"

class UBorder;
class UButton;
class UEditableTextBox;
class UOverlay;
class UTextBlock;
class UWidgetSwitcher;
class ULSCharacterPanelWidget;
class ULSChipStationWidget;
class ULSConfirmDialogWidget;
class ULSInventoryWidget;
class ULSLobbyQuestWidget;
class ULSLobbyStorageWidget;
class ULSLobbyTabWidget;
class ULSSaveSubsystem;
class ULSSettingsWidget;
class ULSStoreWidget;

// 로비 메뉴 루트(WBP_LobbyMenu). 상단 아이콘 바 + 배타 패널 스위처 + 우하단 상시 버튼을 소유한다.
// 로비/지도/설정은 패널이 없어 탭 수와 패널 수가 다르며, 전환은 인덱스가 아닌 페이지 포인터로 한다.
// 상세 전환 계약은 Docs/Systems/LobbyScreenStructure.md가 소유한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSLobbyMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 포커스가 메뉴 트리 밖(게임 뷰포트 등)으로 새면 TAB/ESC가 죽으므로 매 틱 회수한다.
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// TAB 키로 가방 패널을 토글한다. 포커스 이동에 먹히지 않게 터널링 단계에서 가로챈다.
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 자식이 처리하지 않은 클릭(빈 영역)이 오면 포커스를 루트로 회수한다.
	// 포커스가 뷰포트로 빠지면 TAB/ESC 키가 위젯에 더 이상 들어오지 않아 동작이 들쭉날쭉해진다.
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 패널을 여는 유일한 입구. None이면 패널을 전부 닫는다(로비 기본 상태).
	UFUNCTION(BlueprintCallable, Category="LS/UI|Lobby")
	void ShowPanel(ELSLobbyPanel Panel);

	UFUNCTION(BlueprintPure, Category="LS/UI|Lobby")
	ELSLobbyPanel GetActivePanel() const { return ActivePanel; }

	UFUNCTION(BlueprintCallable, Category="LS/UI|Lobby")
	void SetPlayerName(const FText& NewPlayerName) const;

protected:
	// --- 상단 아이콘 탭 바 (항상 표시, 좌→우 배치 순서) ---
	// 로비: 패널을 전부 닫는다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> LobbyTab;
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> ChipTab;
	// 정비 = 에이베리 보급소(자판기/제작대).
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> SupplyTab;
	// 캐릭터 = 스킬 로드아웃.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> CharacterTab;
	// 가방 = 인벤토리 + 물품창고.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> BagTab;
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> QuestTab;
	// 지도(목적지 선택). 패널 미구현이라 안내창만 띄운다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> MapTab;

	// 설정은 배타 패널이 아닌 오버레이지만 상단 탭과 같은 위젯을 사용한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSLobbyTabWidget> SettingsTab;

	// --- 패널 스위처와 페이지 클래스 ---
	// WBP_LobbyMenu에는 비어 있는 스위처만 두고, 퀘스트를 제외한 패널을 런타임에 생성해 직속 자식으로 넣는다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UWidgetSwitcher> TabSwitcher;

	// 퀘스트를 표시할 위치. WBP_LobbyMenu에서 원하는 곳에 빈 Border로 배치한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UBorder> QuestPanelHost;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	TSubclassOf<ULSChipStationWidget> ChipStationPanelClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	TSubclassOf<ULSStoreWidget> StorePanelClass;

	// 캐릭터 탭. 스킬 로드아웃과 노드 그래프는 이 패널이 서브탭으로 소유하므로 로비는 페이지 클래스를 모른다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	TSubclassOf<ULSCharacterPanelWidget> CharacterPanelClass;

	// 가방은 별도 패널 WBP 없이 인벤토리와 물품창고를 런타임 오버레이에 함께 배치한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	TSubclassOf<ULSInventoryWidget> LobbyInventoryClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	TSubclassOf<ULSLobbyStorageWidget> LobbyStorageClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	TSubclassOf<ULSLobbyQuestWidget> QuestPanelClass;

	// --- 프로필 ---
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UTextBlock> PlayerNameText;

	// 보유 골드. SaveSubsystem의 OnGoldChanged를 구독해 변경 즉시 갱신한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UTextBlock> GoldText;

	// 본인 이미지 선택용. 이미지는 버튼 자체에 넣으므로 별도 Image는 바인딩하지 않는다. 선택 로직은 추후 작업.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UButton> CharacterPortraitButton;

	// --- 우하단 상시 버튼 ---
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UButton> MissionStartButton;

	// --- 친구 방 참가 ---
	// 로비는 열릴 때 리슨 서버가 된다. 호스트는 InviteCodeText의 코드를 친구에게 불러주고,
	// 참가자는 JoinAddressTextBox에 그 코드를 넣어 합류한다. 접속 입력은 초대 코드만 받는다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UButton> JoinButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UEditableTextBox> JoinAddressTextBox;

	// 내 초대 코드. 포트까지 담고 있어서 리슨 포트가 7778로 밀려도 그대로 붙는다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UTextBlock> InviteCodeText;

	// BP(WBP_LobbyMenu)에서 WBP_Settings를 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	TSubclassOf<ULSSettingsWidget> SettingsWidgetClass;

	// 미구현 안내창용. BP(WBP_LobbyMenu) 클래스 디폴트에서 WBP_ConfirmDialog를 매핑한다.
	// 보급소 패널에도 이 클래스를 넘겨주므로(RefreshPanelOnOpen) 비어 있으면 상점 다이얼로그까지 죽는다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	TSubclassOf<ULSConfirmDialogWidget> ConfirmDialogClass;

private:
	// --- 패널 전환 (LSLobbyMenuWidget_Panels.cpp) ---
	// 패널 → 스위처 페이지 위젯. 인덱스를 쓰지 않으므로 매핑은 이 함수 하나가 소유한다.
	UWidget* ResolvePanelPage(ELSLobbyPanel Panel) const;

	// 스위처 페이지와 별도 퀘스트 호스트의 가시성을 배타적으로 적용한다.
	bool ApplyPanelVisibility(ELSLobbyPanel Panel);

	// 패널을 여는 순간 최신 데이터로 리빌드한다. 각 패널의 리프레시 계약이 모이는 허브.
	void RefreshPanelOnOpen(ELSLobbyPanel Panel);

	// 떠나는 패널이 띄워 둔 모달을 닫는다. 안 닫으면 패널만 바뀌고 다이얼로그가 화면에 남는다.
	void ClosePanelModal(ELSLobbyPanel Panel) const;

	// 현재 패널의 탭만 선택 표시한다. 설정 오버레이가 열렸으면 SettingsTab만 선택한다.
	void UpdateSelectedTab(ELSLobbyPanel Panel, bool bSettingsSelected = false) const;

	// 임무 시작 버튼은 로비 기본 상태에서만 표시한다.
	void UpdateMissionStartVisibility(bool bVisible) const;

	UFUNCTION()
	void HandleLobbyTabClicked(ULSLobbyTabWidget* ClickedTab);

	// --- 라이프사이클 / 검증 (LSLobbyMenuWidget.cpp) ---
	void BindLobbyTabs();
	void BindActionButtons();
	void CreatePanelWidgets();
	void CreateBagPanelWidget();
	void CreateQuestPanelWidget();
	UUserWidget* CreateRuntimeWidget(UClass* WidgetClass, const TCHAR* WidgetClassName);
	UUserWidget* CreatePanelWidget(UClass* PanelClass, const TCHAR* PanelClassName);
	void ValidateDisplayBindings();
	void InitializeLobbyView();
	void RefreshGoldText() const;
	ULSSaveSubsystem* GetSaveSubsystem() const;
	void HandleGoldChanged();

	// 탭 구독 보일러플레이트 축약. TabName은 미할당 경고용.
	void BindLobbyTab(ULSLobbyTabWidget* Tab, const TCHAR* TabName);
	void UnbindLobbyTab(ULSLobbyTabWidget* Tab) const;

	// 런타임에 생성한 페이지 4개가 스위처 직속 자식인지 검사한다. 퀘스트는 별도 Border가 소유한다.
	void ValidatePanelBindings() const;
	int32 ValidatePanelPages() const;

	UFUNCTION()
	void HandleMissionStartClicked();

	// --- 세션 / 멀티플레이 (LSLobbyMenuWidget_Session.cpp) ---
	// JoinAddressTextBox의 주소로 ClientTravel 한다. 호스트의 현재 맵(로비)으로 들어간다.
	UFUNCTION()
	void HandleJoinClicked();

	// 로비를 열 때 접속 결과를 한 번 알린다. 실패 사유가 남아 있으면 그걸, 없고 클라이언트로
	// 들어왔으면 접속 성공을 띄운다. 호스트(리슨/싱글)에게는 아무것도 띄우지 않는다.
	void ShowSessionNoticeOnOpen();
	void ShowSessionNotice(const FText& Message);

	// 내 초대 코드를 계산해 InviteCodeText에 표시한다. 리슨 서버가 아니면 안내 문구를 대신 넣는다.
	void RefreshInviteCode() const;

	// 초대 코드를 접속 URL(IP:포트)로 푼다. 코드가 아니면 빈 문자열.
	static FString ResolveJoinUrl(const FString& RawInput);

	UFUNCTION()
	void HandleSessionNoticeClosed();

	// --- 입력 / 포커스 / 오버레이 (LSLobbyMenuWidget_Input.cpp) ---
	// TAB 동작: 보급소 안쪽 화면이면 한 단계 뒤로, 그 외에는 가방 패널을 토글한다.
	void ToggleBagPanel();

	// 패널이 자체적으로 띄운 모달이 떠 있는지. 매 틱 포커스 회수 가드가 그 모달의 키 입력을 뺏지 않게 한다.
	bool HasActivePanelModal() const;

	// Settings 화면을 생성해 뷰포트에 띄운다. 이미 떠 있으면 nullptr 반환.
	ULSSettingsWidget* ShowSettingsWidget();

	// 설정 외의 로비 탭을 누르면 설정 오버레이를 정상 종료한 뒤 탭 전환을 이어간다.
	void CloseSettingsForTabSwitch(const ULSLobbyTabWidget* ClickedTab);

	// "아직 구현되지 않았습니다" 안내창을 띄운다. 이미 떠 있으면 중복 생성하지 않는다.
	void ShowNotImplementedNotice();

	// 안내창이 떠 있으면 닫는다. 닫은 게 있으면 true. 탭 클릭 시 먼저 호출해 탭 전환으로도 닫히게 한다.
	bool CloseNotImplementedNotice();

	// Settings 화면 Back 클릭 시: 참조만 정리(위젯 자체는 이미 스스로 닫혔다).
	UFUNCTION()
	void HandleSettingsBackToMenu();

	// 미구현 안내창이 닫히면 참조를 정리하고 TAB/ESC가 다시 로비로 오도록 포커스를 회수한다.
	UFUNCTION()
	void HandleNotImplementedDialogClosed();

	// --- 상태 ---
	// 현재 열린 패널. 스위처 인덱스를 역으로 읽어 상태를 추론하지 않고 여기 하나만 본다.
	UPROPERTY(Transient)
	ELSLobbyPanel ActivePanel = ELSLobbyPanel::None;

	UPROPERTY(Transient)
	TObjectPtr<ULSSettingsWidget> ActiveSettingsWidget;

	UPROPERTY(Transient)
	TObjectPtr<ULSConfirmDialogWidget> ActiveConfirmDialog;

	// 참가 버튼 연타 가드. ClientTravel이 진행 중인데 또 누르면 접속이 매번 새로 시작돼 끊긴다.
	UPROPERTY(Transient)
	bool bJoinInProgress = false;

	// 클래스 디폴트로 받은 WBP를 런타임에 생성한 패널 인스턴스.
	UPROPERTY(Transient)
	TObjectPtr<ULSChipStationWidget> ChipStationPanelInstance;

	UPROPERTY(Transient)
	TObjectPtr<ULSStoreWidget> StorePanelInstance;

	UPROPERTY(Transient)
	TObjectPtr<ULSCharacterPanelWidget> CharacterPanelInstance;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> BagPanelInstance;

	UPROPERTY(Transient)
	TObjectPtr<ULSInventoryWidget> LobbyInventoryInstance;

	UPROPERTY(Transient)
	TObjectPtr<ULSLobbyStorageWidget> LobbyStorageInstance;

	UPROPERTY(Transient)
	TObjectPtr<ULSLobbyQuestWidget> QuestPanelInstance;
};
