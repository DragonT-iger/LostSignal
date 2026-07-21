// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Data/LSProtocolTypes.h"
#include "LSProtocolDebugWidget.generated.h"

class ALSPlayerControllerBase;
class UButton;
class UTextBlock;
class UVerticalBox;

// 시연/디버그용 프로토콜 조정 패널.
// 위젯 트리를 C++ 로 직접 생성하므로 WBP 에셋이 필요 없다.
// 4개 프로토콜(Survival/Carrying/Battle/Navigation)의 테스트 오버라이드 레벨(0~8)을
// 소유 PlayerController 의 기존 LSTest*Protocol / LSClearProtocolTest 함수로 조정한다. (토글: Insert)
UCLASS()
class LOSTSIGNAL_API ULSProtocolDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 현재 효과 프로토콜 레벨로 표시 숫자를 갱신한다. (패널을 다시 켤 때 PlayerController 가 호출)
	void RefreshLevelTexts();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	// 프로토콜별 레벨 표시 텍스트.
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SurvivalLevelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CarryingLevelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BattleLevelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NavigationLevelText;

	// "레이드 종료" 버튼 — 레이드 중에만 보인다. (로비에서는 숨김)
	UPROPERTY(Transient)
	TObjectPtr<UButton> EndRaidButton;

	// "테스트 맵 가기" 버튼 — 정식 레이드 진입을 타되 목적지만 TestRaidLevel 로. 로비에서만 보인다.
	UPROPERTY(Transient)
	TObjectPtr<UButton> TestMapButton;

	// 현재 신호 게이지 드레인 배속 표시. 1분 주기를 배속만큼 짧게 돌려 칩 프로토콜이 사라지는 걸 빨리 확인.
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TimeScaleText;

	// 버튼 클릭 핸들러 (UButton::OnClicked 은 dynamic delegate 이므로 UFUNCTION 필요).
	UFUNCTION()
	void HandleSurvivalMinus();
	UFUNCTION()
	void HandleSurvivalPlus();
	UFUNCTION()
	void HandleCarryingMinus();
	UFUNCTION()
	void HandleCarryingPlus();
	UFUNCTION()
	void HandleBattleMinus();
	UFUNCTION()
	void HandleBattlePlus();
	UFUNCTION()
	void HandleNavigationMinus();
	UFUNCTION()
	void HandleNavigationPlus();
	UFUNCTION()
	void HandleClear();
	UFUNCTION()
	void HandleMaxAll();
	UFUNCTION()
	void HandleAddGold();
	UFUNCTION()
	void HandleEndRaid();
	UFUNCTION()
	void HandleGoToTestMap();
	UFUNCTION()
	void HandleTimeScale1x();
	UFUNCTION()
	void HandleTimeScale2x();
	UFUNCTION()
	void HandleTimeScale5x();
	UFUNCTION()
	void HandleTimeScale10x();
	UFUNCTION()
	void HandleTimeScale20x();
	UFUNCTION()
	void HandleTimeScale30x();

private:
	static constexpr int32 MaxProtocolLevel = 8;
	static constexpr int32 DebugGoldAmount = 10000;

	void BuildPanel();
	void BuildProtocolRow(UVerticalBox* Parent, ELSProtocolType Type, const FString& DisplayName);
	// 배속 프리셋 버튼 행(x1/x2/x5/x10/x20)을 만든다.
	void BuildTimeScaleRow(UVerticalBox* Parent);
	// 글로벌 타임 딜레이션을 적용하고 표시 텍스트를 갱신한다.
	void ApplyTimeScale(float Scale);
	UButton* MakeButton(const FString& Label, int32 FontSize);
	UButton* MakeButton(const FText& Label, int32 FontSize);
	UTextBlock* MakeText(const FString& InText, int32 FontSize);
	UTextBlock* MakeText(const FText& InText, int32 FontSize);

	ALSPlayerControllerBase* ResolvePC() const;
	// 현재 레이드 진행 중인지 — 소유 PC 의 레이드 인벤토리 활성 여부로 판정.
	bool IsRaidActive() const;
	// 현재 로비인지 — 현재 게임모드가 ALSLobbyGameMode 인지로 판정. (테스트 맵 진입 버튼 게이팅)
	bool IsLobbyActive() const;
	// 레이드/로비 상태에 맞춰 "레이드 종료"·"테스트 맵 가기" 버튼 표시/숨김을 갱신한다.
	void UpdateEndRaidVisibility();
	int32 GetDisplayLevel(ELSProtocolType Type) const;
	void ApplyLevel(ELSProtocolType Type, int32 NewLevel);
	void AdjustLevel(ELSProtocolType Type, int32 Delta);
};
