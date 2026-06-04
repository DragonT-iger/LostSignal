#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "LSProtocolUIWidget.generated.h"

class ULSProtocolWidget;

// 프로토콜 패널 (WBP_ProtocolUI 의 부모 클래스).
// 4종 프로토콜(생존/적재/전투/탐색) 한 줄씩을 묶는다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSProtocolUIWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// 4종 프로토콜 표시를 갱신한다. 현재는 임의값으로 채우는 더미 구현.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Protocol")
	void RefreshProtocolUI();

protected:
	// ---- 4종 프로토콜 칸 (WBP_Protocol) ----
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSProtocolWidget> Protocol_Survival;    // 생존

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSProtocolWidget> Protocol_Carrying;    // 적재

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSProtocolWidget> Protocol_Battle;      // 전투

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSProtocolWidget> Protocol_Navigation;  // 탐색
};
