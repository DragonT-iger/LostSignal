#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSTitleMenuWidget.generated.h"

class ULSTitleMenuButtonWidget;

// 타이틀 화면 우측 메뉴 패널. Continue/New/Settings/Crew/Exit 버튼(WBP_TitleMenuButton)을 바인딩한다.
// Settings/Crew는 아직 미구현이라 클릭 시 경고 로그만 남긴다.
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

	// 세이브 서브시스템 조회. 없으면 nullptr.
	class ULSSaveSubsystem* GetSaveSubsystem() const;

	// LS Session Settings의 LobbyLevel로 전환한다.
	void OpenLobbyLevel();
};
