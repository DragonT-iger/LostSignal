#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSTitleMenuWidget.generated.h"

class ULSTitleMenuButtonWidget;
class ULSConfirmDialogWidget;

// 타이틀 화면 우측 메뉴 패널. Continue/New/Settings/Crew/Exit 버튼(WBP_TitleMenuButton)을 바인딩한다.
// Settings/Crew는 아직 미구현이라 클릭 시 경고 로그만 남긴다.
// New/Exit는 확인 다이얼로그(WBP_ConfirmDialog)를 띄우고, 확인을 눌러야 실제로 진행한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSTitleMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Title")
	TObjectPtr<ULSTitleMenuButtonWidget> ContinueButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Title")
	TObjectPtr<ULSTitleMenuButtonWidget> NewButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Title")
	TObjectPtr<ULSTitleMenuButtonWidget> SettingsButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Title")
	TObjectPtr<ULSTitleMenuButtonWidget> CrewButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Title")
	TObjectPtr<ULSTitleMenuButtonWidget> ExitButton;

	// BP(WBP_TitleMenu)에서 WBP_ConfirmDialog를 매핑한다. New/Exit 확인용.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Title")
	TSubclassOf<ULSConfirmDialogWidget> ConfirmDialogClass;

private:
	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleNewClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleCrewClicked();

	UFUNCTION()
	void HandleExitClicked();

	// New 확인 시: 세이브 초기화 후 로비 진입.
	UFUNCTION()
	void HandleNewConfirmed();

	// Exit 확인 시: 게임 종료.
	UFUNCTION()
	void HandleExitConfirmed();

	// 다이얼로그 취소 시: 참조만 정리.
	UFUNCTION()
	void HandleDialogCancelled();

	// 세이브 서브시스템 조회. 없으면 nullptr.
	class ULSSaveSubsystem* GetSaveSubsystem() const;

	// LS Session Settings의 LobbyLevel로 전환한다.
	void OpenLobbyLevel();

	// 확인 다이얼로그를 생성해 뷰포트에 띄운다. 이미 떠 있으면 nullptr 반환.
	ULSConfirmDialogWidget* ShowConfirmDialog(const FText& Message);

	UPROPERTY(Transient)
	TObjectPtr<ULSConfirmDialogWidget> ActiveConfirmDialog;
};
