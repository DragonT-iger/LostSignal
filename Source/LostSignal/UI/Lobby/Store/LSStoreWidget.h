#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSStoreWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class ULSCraftingWidget;
class ULSStoreButtonWidget;
class ULSVendingWidget;

// 상점 화면의 우측 버튼 영역 상태. 상태별로 ButtonBox 안의 버튼을 동적으로 다시 만든다.
UENUM(BlueprintType)
enum class ELSStoreState : uint8
{
	FunctionSelect, // 자판기 / 제작대 / 대화하기 (3버튼)
	TalkList,       // 대화 목록 3개 + 위/아래 화살표
	QuestOffer,     // 수락 / 거절 (2버튼)
	TalkResult      // 확인 (1버튼) — 누르면 FunctionSelect로 복귀
};

// 하드코딩 대화 항목. 퀘스트 데이터/대화 시스템이 아직 없어 cpp에 임시로 박아 두고 추후 리팩토링한다.
USTRUCT()
struct FLSStoreTalkEntry
{
	GENERATED_BODY()

	// 대화 목록 버튼에 표시할 라벨.
	FText Label;

	// 항목 선택 시 대사창에 표시할 본문.
	FText Body;

	// 퀘스트 대화 여부. 목록 최상단 배치 + 아이콘 표시 + 선택 시 수락/거절 분기.
	bool bQuest = false;
};

// 에이베리 보급소 상점 화면(WBP_Store)의 부모 클래스. 개인정비 ContentSwitcher의 Supply 페이지에 배치된다.
// CASHIER-9 이미지/이름표는 아트가 WBP에 고정 배치하고, C++은 버튼과 대사창 텍스트만 제어한다.
// 버튼은 상태 전환 시 ButtonBox(버티컬 박스) 안에 WBP_StoreButton을 동적으로 생성/삭제해 채운다.
// 자판기와 제작대는 각 전용 화면을 최초 진입 시 생성해 전환한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSStoreWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 초기 상태(기능 선택 + 대기 대사)로 되돌린다. 개인정비에서 보급소 페이지를 열 때마다 호출한다.
	void ResetStore();

protected:
	// 기능 선택/대화 화면 전체(CASHIER-9 이미지·버튼·대사창 묶음) 컨테이너. 자판기를 열면 통째로 숨긴다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UWidget> SelectionPanel;

	// 자판기(구매/판매) 화면 위젯 클래스. BP(WBP_Store) 클래스 디폴트에서 WBP_Vending을 매핑한다.
	// WBP에 인스턴스를 배치하지 않고, 자판기를 처음 열 때 생성해 루트 캔버스에 전체 화면으로 붙인다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Store")
	TSubclassOf<ULSVendingWidget> VendingWidgetClass;

	// 제작 화면 위젯 클래스. BP(WBP_Store) 클래스 디폴트에서 WBP_Crafting을 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Store")
	TSubclassOf<ULSCraftingWidget> CraftingWidgetClass;

	// 상태별 버튼들을 담는 버티컬 박스. 내용물은 C++이 동적으로 채운다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UVerticalBox> ButtonBox;

	// 대화 목록 스크롤 화살표. 아트 스타일링이 필요해 WBP에 고정 배치하고, 표시 여부만 C++이 제어한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UButton> TalkUpButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UButton> TalkDownButton;

	// 대사창 본문. CASHIER-9 이름표는 아트 고정이라 바인딩하지 않는다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> DialogueText;

	// 동적 생성할 버튼 위젯 클래스. BP(WBP_Store) 클래스 디폴트에서 WBP_StoreButton을 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Store")
	TSubclassOf<ULSStoreButtonWidget> StoreButtonClass;

private:
	UFUNCTION()
	void HandleStoreButtonClicked(ULSStoreButtonWidget* ClickedButton);

	UFUNCTION()
	void HandleTalkUpClicked();

	UFUNCTION()
	void HandleTalkDownClicked();

	// 자판기 화면 뒤로가기. 기능 선택 화면으로 되돌린다.
	UFUNCTION()
	void HandleVendingBackRequested();

	// 상태 전환: ButtonBox를 비우고 상태에 맞는 버튼들을 다시 만든다. 기본 대사/화살표 표시도 함께 적용.
	void ShowState(ELSStoreState NewState);

	// 자판기 화면 표시 전환. true면 기능 선택 화면을 숨기고 자판기를 연다(최초 1회 생성).
	void SetVendingVisible(bool bVisible);

	// 자판기 위젯이 없으면 생성해 루트 캔버스에 전체 화면으로 붙인다. 실패하면 nullptr.
	ULSVendingWidget* EnsureVendingWidget();

	// 제작 화면 표시 전환. true면 기능 선택을 숨기고 최신 제작 데이터를 연다.
	void SetCraftingVisible(bool bVisible);

	// 제작 위젯이 없으면 생성해 루트 캔버스에 전체 화면으로 붙인다. 실패하면 nullptr.
	ULSCraftingWidget* EnsureCraftingWidget();

	// 클릭된 버튼의 ButtonBox 내 순서(0부터)를 상태별 의미로 해석해 분기한다.
	void HandleClickForState(int32 ButtonOrdinal);

	// 대화 목록 항목 선택 처리. 퀘스트는 수락/거절로, 일반 대화는 본문+확인으로 분기.
	void HandleTalkEntrySelected(int32 EntryIndex);

	// 퀘스트 수락/거절 처리 후 결과 대사와 확인 버튼을 표시한다.
	void HandleQuestOfferSelected(bool bAccepted);

	// ButtonBox 안의 버튼을 모두 지우고 Count개를 새로 만들어 ActiveButtons에 담는다.
	void RebuildButtons(int32 Count);

	// 현재 스크롤 위치 기준으로 대화 목록 버튼들의 라벨/아이콘을 갱신한다.
	void RefreshTalkList();

	// 대화 항목 하드코딩 초기화. 퀘스트(수락 전) 항목을 최상단에 둔다.
	void BuildTalkEntries();

	// 수락 전 퀘스트 대화가 남아 있는지. 대화하기 버튼의 퀘스트 아이콘 표시 판단용.
	bool HasPendingQuest() const;

	// 현재 ButtonBox에 떠 있는 동적 버튼들. 클릭 시 순서 판별용.
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSStoreButtonWidget>> ActiveButtons;

	// 목록에서 보여줄 대화 항목들. 퀘스트 수락 시 퀘스트 항목을 제거하고 다시 만든다.
	TArray<FLSStoreTalkEntry> TalkEntries;

	// 대화 목록 스크롤 시작 인덱스. 화살표로 한 칸씩 이동하며 범위를 벗어나면 고정된다.
	int32 TalkListStartIndex = 0;

	// 임시 퀘스트 진행 상태. 세이브 연동 없이 위젯 수명 동안만 유지한다(추후 퀘스트 시스템으로 이관).
	bool bQuestAccepted = false;

	// 런타임 생성한 자판기 화면. 처음 자판기를 열 때 만들어 재사용한다.
	UPROPERTY(Transient)
	TObjectPtr<ULSVendingWidget> VendingPanel;

	// 지연 생성한 제작 화면. 보급소를 닫았다 다시 열 때 재사용한다.
	UPROPERTY(Transient)
	TObjectPtr<ULSCraftingWidget> CraftingPanel;

	ELSStoreState CurrentState = ELSStoreState::FunctionSelect;
};
