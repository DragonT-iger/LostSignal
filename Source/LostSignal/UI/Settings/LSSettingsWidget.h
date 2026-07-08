#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSSettingsWidget.generated.h"

class UButton;
class ULSControlSettingsWidget;
class ULSConfirmDialogWidget;
class ULSTitleMenuButtonWidget;
class ULSSoundSettingsWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLSSettingsBackToMenu);

// 세팅 화면(WBP_Settings)의 부모 클래스.
// Sound/Controls/Graphics/Language/MainMenu 5개 항목은 WBP_SettingsButton(ULSTitleMenuButtonWidget 재사용)이고,
// BackButton은 세팅 패널을 닫는 일반 UButton이다.
//  - Sound: WBP_Sound를 띄운다.
//  - Controls: WBP_ControlSettings를 띄운다.
//  - Graphics/Language: 아직 미구현이라 클릭 시 경고 로그만 남긴다.
//  - MainMenuButton("메인메뉴로 돌아가기"): 레이드 여부와 무관하게 타이틀 레벨로 나간다. 단, 레이드 중이면
//    파밍 성과를 포기하는 행동이라 확인 다이얼로그를 먼저 거치고(아이템 처리는 ALSFarmingGameMode의 Quit 규칙
//    = 출발 장비 복구), 확인 시 Quit으로 종료해 타이틀로 나간다.
//  - BackButton: 세팅 패널만 닫고 OnBackToMenu를 브로드캐스트한다(밑에 있던 화면이 다시 보인다).
//    BackButton 클릭뿐 아니라 ESC/TAB 키로도 동일하게 닫힌다(NativeOnKeyDown/NativeOnPreviewKeyDown).
// 이 위젯 자체는 타이틀/로비/레이드(ESC) 어디서든 동일하게 재사용된다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// TAB 포커스 이동보다 먼저 세팅 패널을 닫는다.
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// ESC로도 세팅 패널을 닫는다.
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 세팅 패널을 닫는다(OnBackToMenu 브로드캐스트 + RemoveFromParent).
	// BackButton / ESC / TAB / 외부(레이드 PlayerController)가 공용으로 호출한다.
	void CloseSettings();

	// "메인메뉴로 돌아가기" 버튼 표시 여부. 타이틀에서 열 때는 이미 메인메뉴라 불필요하므로 false로 숨긴다.
	// (로비/레이드에서는 기본 true 그대로 둔다.)
	void SetMainMenuButtonVisible(bool bVisible);

	UPROPERTY(BlueprintAssignable, Category="LS/UI|Settings")
	FLSSettingsBackToMenu OnBackToMenu;

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<ULSTitleMenuButtonWidget> SoundButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<ULSTitleMenuButtonWidget> ControlButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<ULSTitleMenuButtonWidget> GraphicsButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<ULSTitleMenuButtonWidget> LanguageButton;

	// "메인메뉴로 돌아가기". 타이틀 레벨로 나간다(레이드 중이면 확인 다이얼로그 경유).
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<ULSTitleMenuButtonWidget> MainMenuButton;

	// 세팅 패널을 닫는 일반 버튼(뒤로).
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<UButton> BackButton;

	// BP(WBP_Settings)에서 WBP_Sound를 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Settings")
	TSubclassOf<ULSSoundSettingsWidget> SoundSettingsWidgetClass;

	// BP(WBP_Settings)에서 WBP_ControlSettings를 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Settings")
	TSubclassOf<ULSControlSettingsWidget> ControlSettingsWidgetClass;

	// BP(WBP_Settings)에서 WBP_ConfirmDialog를 매핑한다. 레이드 중 타이틀 복귀 확인용.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Settings")
	TSubclassOf<ULSConfirmDialogWidget> ConfirmDialogClass;

private:
	UFUNCTION()
	void HandleSoundClicked();

	// 사운드 화면이 Back/ESC로 닫힐 때: 숨겨둔 세팅 화면을 다시 보이게 하고 포커스를 되돌린다.
	UFUNCTION()
	void HandleSoundClosed();

	UFUNCTION()
	void HandleControlClicked();

	UFUNCTION()
	void HandleControlClosed();

	UFUNCTION()
	void HandleGraphicsClicked();

	UFUNCTION()
	void HandleLanguageClicked();

	UFUNCTION()
	void HandleMainMenuClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleReturnToTitleConfirmed();

	UFUNCTION()
	void HandleDialogCancelled();

	// 소유 PlayerController 기준으로 현재 레이드가 진행 중인지.
	bool IsRaidActive() const;

	// 레이드 중 메인메뉴 클릭 시: 확인 다이얼로그를 띄운다. 이미 떠 있으면 nullptr 반환.
	ULSConfirmDialogWidget* ShowReturnToTitleConfirmDialog();

	// 아직 구현되지 않은 항목(Graphics/Language) 클릭 시 안내창을 띄운다.
	void ShowNotImplementedNotice();

	// ConfirmDialogClass로 메시지 다이얼로그를 생성해 서브패널에 띄운다. 델리게이트 연결은 호출자가 한다.
	// 이미 떠 있거나 클래스 미지정이면 nullptr 반환.
	ULSConfirmDialogWidget* CreateDialog(const FText& Message);

	// 타이틀 레벨로 이동(레이드가 아닐 때 직접 호출). 레이드 중에는 Quit 경로가 타이틀로 보낸다.
	void TravelToTitle();

	// MainMenuButton 표시 여부. 타이틀에서 열 때 false로 지정되어 버튼을 숨긴다.
	bool bMainMenuButtonVisible = true;

	UPROPERTY(Transient)
	TObjectPtr<ULSSoundSettingsWidget> ActiveSoundWidget;

	UPROPERTY(Transient)
	TObjectPtr<ULSControlSettingsWidget> ActiveControlWidget;

	UPROPERTY(Transient)
	TObjectPtr<ULSConfirmDialogWidget> ActiveConfirmDialog;
};
