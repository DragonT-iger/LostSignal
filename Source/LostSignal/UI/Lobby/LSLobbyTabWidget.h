#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSLobbyTabWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLSLobbyTabClicked);

// 로비 탭 하나(WBP_LobbyTab)의 부모 클래스. Border 안에 Button + Text 구조이지만 Border와 Text는
// 아트/에디터에서 다루므로 바인딩하지 않고 Button만 바인딩한다. 로비 메뉴(WBP_Lobby)에서 이 위젯을
// 4개(플레이/개인정비/퀘스트/캐릭터 변경) 배치한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSLobbyTabWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintAssignable, Category="LS/UI|Lobby")
	FLSLobbyTabClicked OnClicked;

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UButton> Button;

private:
	UFUNCTION()
	void HandleButtonClicked();
};
