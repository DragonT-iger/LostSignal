#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSSoundSettingsWidget.generated.h"

class UButton;
class ULSSoundRowWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLSSoundSettingsClosed);

// 사운드 세팅 화면(WBP_Sound)의 부모 클래스. ULSSettingsWidget에서 진입할 때 세팅을 숨기고 대신 뜨는 화면이다.
// Back 버튼이나 ESC로 닫으면 OnClosed를 브로드캐스트해서 뒤의 세팅 화면이 다시 보이도록 한다.
// Master/BGM/SFX 3개 줄(WBP_SoundRow)에 각자의 버스를 지정해 초기화한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSSoundSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ESC로도 닫는다. 어떤 위젯에 포커스가 있든 이 화면이 직접 키 이벤트를 받는다.
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 세팅 화면 진입 시 구독한다. Back/ESC로 닫힐 때 브로드캐스트되어 세팅 화면을 복원시킨다.
	UPROPERTY(BlueprintAssignable, Category="LS/UI|Settings")
	FLSSoundSettingsClosed OnClosed;

	// 사운드 화면을 닫는다(OnClosed 브로드캐스트 + RemoveFromParent). Back/ESC 공용.
	void CloseSound();

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<UButton> BackButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<ULSSoundRowWidget> MasterRow;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<ULSSoundRowWidget> BGMRow;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<ULSSoundRowWidget> SFXRow;

private:
	UFUNCTION()
	void HandleBackClicked();
};
