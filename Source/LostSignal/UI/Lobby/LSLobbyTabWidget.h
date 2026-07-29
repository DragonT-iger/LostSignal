#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateColor.h"
#include "LSLobbyTabWidget.generated.h"

class UButton;
class ULSLobbyTabWidget;

// 클릭한 탭 자기 자신을 넘긴다. 로비 루트는 상단 아이콘 탭 8개를 단일 핸들러로 받아 분기하므로,
// 탭마다 핸들러를 따로 만들지 않는다(ULSCraftingTabWidget/ULSVendingButtonWidget과 같은 형태).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLSLobbyTabClicked, ULSLobbyTabWidget*, ClickedTab);

// 로비 상단 아이콘 탭 하나(WBP_LobbyTab / WBP_LoadoutPreparationTab)의 부모 클래스.
// Border 안에 Button + Text/Icon 구조이지만 Border와 Text는 아트가 다루므로 Button만 바인딩한다.
// 로비 메뉴(WBP_LobbyMenu)에서 이 위젯을 8개 배치한다(로비/칩 세팅/정비/캐릭터/가방/퀘스트/지도/설정).
// 설정은 배타 패널이 아닌 오버레이지만 상단 탭의 표현과 선택 상태를 공유한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSLobbyTabWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintAssignable, Category="LS/UI|Lobby")
	FLSLobbyTabClicked OnClicked;

	// 현재 열린 패널의 탭임을 표시한다. 탭 바가 항상 보이는 구조라 어느 탭이 활성인지 표시가 필요하다.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Lobby")
	void SetSelected(bool bInSelected);

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UButton> Button;

	// 상태별 틴트. 아트가 WBP_LobbyTab 클래스 디폴트에서 매핑한다.
	// 브러시(이미지/라운드박스)는 아트가 WBP에서 잡은 것을 유지하고 틴트만 덮어쓴다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	FLinearColor NormalColor = FLinearColor(FColor(0x2A, 0x30, 0x3C));

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	FLinearColor HoveredColor = FLinearColor(FColor(0x3C, 0x46, 0x56));

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	FLinearColor PressedColor = FLinearColor(FColor(0x1E, 0x23, 0x2C));

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	FLinearColor SelectedColor = FLinearColor(FColor(0x5B, 0x9B, 0xC4));

private:
	UFUNCTION()
	void HandleButtonClicked();

	// 아트 브러시는 유지하고 틴트만 상태별로 덮어쓴다. 선택 표시에 SetIsEnabled(false)를 쓰지 않는 이유는
	// 슬레이트가 비활성 위젯의 채도를 죽여 색이 탁해지기 때문이다(ChipSystem.md에서 이미 기각된 방식).
	void ApplyButtonColors() const;

	UPROPERTY(Transient)
	bool bIsSelected = false;
};
