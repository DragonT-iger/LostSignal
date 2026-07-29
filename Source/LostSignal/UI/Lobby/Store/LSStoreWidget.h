#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "Layout/Margin.h"
#include "Types/SlateEnums.h"
#include "LSStoreWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class ULSCraftingWidget;
class ULSConfirmDialogWidget;
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

// 에이베리 보급소 상점 화면(WBP_Store)의 부모 클래스. 로비 상단 정비 탭의 Supply 패널에 배치된다.
// CASHIER-9 이미지/이름표는 아트가 WBP에 고정 배치하고, C++은 버튼과 대사창 텍스트만 제어한다.
// 버튼은 ButtonBox(버티컬 박스)에 배치된 WBP_StoreButton을 재사용하며 상태별로 필요한 개수만 노출한다.
// 대화 목록에 퀘스트가 여러 개 쌓여 배치 버튼보다 많이 필요할 때만 부족분을 런타임에 생성한다.
// 자판기와 제작대는 각 전용 화면을 최초 진입 시 생성해 전환한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSStoreWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 초기 상태(기능 선택 + 대기 대사)로 되돌린다. 로비에서 정비 패널을 열 때마다 호출한다.
	void ResetStore();

	// 자판기/제작대가 열려 있으면 기능 선택 화면으로 한 단계만 되돌린다.
	// 처리할 하위 화면이 없으면 로비 루트가 입력을 처리하도록 false를 반환한다.
	bool TryHandleBack();

	// 로비 루트에 매핑된 공용 확인 다이얼로그를 자판기에 전달한다.
	void SetConfirmDialogClass(TSubclassOf<ULSConfirmDialogWidget> InConfirmDialogClass);
	bool HasActiveConfirmDialog() const;
	void CloseActiveConfirmDialog();

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

	// 상태별 버튼들을 담는 버티컬 박스.
	// 아트가 WBP_Store 디자이너에 배치한 WBP_StoreButton들이 레이아웃 원본이다. C++은 이 버튼을
	// 재사용하며 표시/숨김과 라벨만 바꾼다. 디자이너 버튼을 지우면 슬롯 설정(패딩/Fill 등)을 잃고
	// 런타임 기본값(Auto/패딩 0)으로 되돌아가므로 삭제하면 안 된다.
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

	// 디자이너 배치 버튼이 모자랄 때 추가 생성할 버튼 위젯 클래스.
	// BP(WBP_Store) 클래스 디폴트에서 WBP_StoreButton을 매핑한다.
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

	// 상태 전환: 상태에 맞는 개수만큼 버튼을 노출하고 라벨을 채운다. 기본 대사/화살표 표시도 함께 적용.
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

	// ButtonBox의 버튼 중 앞에서 Count개만 보이게 하고 나머지는 숨긴다. 풀이 부족하면 그만큼만 새로 만든다.
	void RebuildButtons(int32 Count);

	// ButtonBox에 있는 WBP_StoreButton들을 풀로 수집하고, 첫 버튼의 슬롯 값을 원본으로 캐시한다(최초 1회).
	void EnsureButtonPool();

	// 새로 만든 버튼을 넣을 ButtonBox 인덱스. 화살표가 ButtonBox의 자식이어도 그 사이에 들어가게 한다.
	int32 GetButtonInsertIndex() const;

	// 캐시한 원본 슬롯 값을 버튼의 VerticalBox 슬롯에 적용한다. 신규 생성과 상태 전환 복원에 모두 쓴다.
	void ApplyButtonSlotTemplate(ULSStoreButtonWidget* StoreButton) const;

	// 버튼 클릭 델리게이트 바인딩. 재구성 후에도 안전하게 다시 부를 수 있게 중복 없이 등록한다.
	void BindStoreButton(ULSStoreButtonWidget* StoreButton);

	// 현재 스크롤 위치 기준으로 대화 목록 버튼들의 라벨/아이콘을 갱신한다.
	void RefreshTalkList();

	// 대화 항목 하드코딩 초기화. 퀘스트(수락 전) 항목을 최상단에 둔다.
	void BuildTalkEntries();

	// 수락 전 퀘스트 대화가 남아 있는지. 대화하기 버튼의 퀘스트 아이콘 표시 판단용.
	bool HasPendingQuest() const;

	// 현재 화면에 보이는 버튼들. 클릭 시 순서 판별용(숨긴 버튼은 담지 않는다).
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSStoreButtonWidget>> ActiveButtons;

	// ButtonBox 안의 버튼 전체. 디자이너 배치 버튼 + 풀이 부족할 때 런타임에 추가한 버튼을 담는다.
	// 상태 전환 시 지우지 않고 표시/숨김만 바꿔 디자이너 슬롯 설정을 보존한다.
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSStoreButtonWidget>> ButtonPool;

	// 디자이너 첫 버튼에서 캡처한 VerticalBox 슬롯 원본 값(UVerticalBoxSlot의 전체 프로퍼티).
	FSlateChildSize ButtonSlotSize;
	FMargin ButtonSlotPadding;
	TEnumAsByte<EHorizontalAlignment> ButtonSlotHAlign = HAlign_Fill;
	TEnumAsByte<EVerticalAlignment> ButtonSlotVAlign = VAlign_Fill;

	// 버튼을 보일 때 적용할 Visibility. 디자이너 값을 그대로 쓰되 숨김 값이면 Visible로 대체한다.
	ESlateVisibility ButtonVisibleState = ESlateVisibility::Visible;

	// 슬롯 원본 캡처 완료 여부. 실패했으면 슬롯을 건드리지 않고 엔진 기본값에 맡긴다.
	bool bSlotTemplateCaptured = false;

	// 버튼 풀 수집 완료 여부.
	bool bButtonPoolInitialized = false;

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

	UPROPERTY(Transient)
	TSubclassOf<ULSConfirmDialogWidget> ConfirmDialogClass;

	ELSStoreState CurrentState = ELSStoreState::FunctionSelect;
};
