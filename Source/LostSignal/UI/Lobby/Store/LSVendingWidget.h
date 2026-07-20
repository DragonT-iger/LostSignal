#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/TimerHandle.h"
#include "Session/LSSessionSubsystem.h"
#include "LSVendingWidget.generated.h"

class UButton;
class UTextBlock;
class UWrapBox;
class ULSItemSlotWidget;
class ULSSaveSubsystem;
class ULSVendingButtonWidget;
class ULSVendingSlotWidget;

// 자판기 판매 목록의 분류(카테고리) 종류.
UENUM(BlueprintType)
enum class ELSVendingCategory : uint8
{
	Equip,      // 장비 (Weapon_/Armor_)
	Consumable, // 소모품 (Item_ 중 Item_Type 4~9)
	Chip,       // 칩 (Chip_)
	Material    // 재료 (Item_ 중 그 외)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLSVendingBackRequested);

// 자판기(구매/판매) 화면(WBP_Vending)의 부모 클래스. WBP_Store 안에 배치되어 기능 선택에서 자판기를 고르면 열린다.
// 좌측 내 아이템(가방/안전슬롯/창고), 중앙 상세+거래 버튼, 우측 자판기 목록을 모두 이 위젯이 채운다.
// 판매 목록은 DT_StoreStock(StoreStockTable)에서 읽고, 가격은 각 아이템 테이블의 Item_Cost가 단일 출처다.
// 1차 범위: 구매/판매만. 재고 수량/새로고침 타이머/잠금·퀘스트 아이템 판매 불가 팝업은 2차에서 구현한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSVendingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 수량 팝업이 열려 있을 때 F=예 / X=아니오 단축키 처리.
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// 자판기 화면을 초기 상태로 열 준비를 한다(골드/목록/내 아이템 갱신, 선택 해제).
	void OpenVending();

	// 뒤로가기 버튼. 상점 위젯이 구독해 기능 선택 화면으로 되돌린다.
	UPROPERTY(BlueprintAssignable, Category="LS/UI|Store")
	FLSVendingBackRequested OnBackRequested;

protected:
	// --- 상단 ---
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UButton> BackButton;

	// "새로고침까지 00:05:00" 카운트다운 표시. 0이 되면 재고가 Stock_Max로 리셋된다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> RefreshTimerText;

	// --- 우측: 자판기 목록 ---
	// 분류 버튼 4개. 누르면 그 카테고리 상품만 목록에 표시한다. 라벨은 C++이 넣는다(WBP_VendingButton).
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<ULSVendingButtonWidget> EquipTab;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<ULSVendingButtonWidget> ConsumableTab;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<ULSVendingButtonWidget> ChipTab;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<ULSVendingButtonWidget> MaterialTab;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UWrapBox> StockBox;

	// --- 좌측: 내 아이템 ---
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UWrapBox> BagBox;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UWrapBox> SafeBox;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UWrapBox> WarehouseBox;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> BagCountText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> SafeCountText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> WarehouseCountText;

	// --- 중앙: 선택 아이템 상세 ---
	// 상세 영역 컨테이너. 선택이 없으면 통째로 숨긴다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UWidget> DetailPanel;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<ULSItemSlotWidget> DetailItemSlot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> DetailNameText;

	// 재고/보유 수량 표시. 판매(내 아이템) 선택 시 보유 수량을 보여준다.
	// 구매(자판기 상품) 선택 시에는 숨긴다 — 재고 추적이 2차 범위라 아직 표시할 값이 없다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> DetailStockText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> DetailDescriptionText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UButton> TradeButton;

	// 거래 버튼 라벨("구매"/"판매").
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> TradeActionText;

	// 거래 버튼의 단가 표시.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> TradePriceText;

	// --- 수량 확인 팝업 (WBP 안에 고정 배치, 평소 숨김. 열기/닫기/수량은 전부 이 위젯 C++이 처리) ---
	// 팝업 전체 컨테이너.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UWidget> DialogPanel;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<ULSItemSlotWidget> DialogItemSlot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> DialogNameText;

	// 단가 표시(수량과 무관).
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> DialogPriceText;

	// "1/5" 형식의 수량 표시.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UTextBlock> DialogQuantityText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UButton> DecreaseButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UButton> IncreaseButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UButton> YesButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Store")
	TObjectPtr<UButton> NoButton;

	// 동적 생성할 아이템 칸 위젯 클래스. BP(WBP_Vending)에서 WBP_VendingSlot을 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Store")
	TSubclassOf<ULSVendingSlotWidget> VendingSlotClass;

	// 판매가 = Item_Cost * 이 비율. 기획 확정 전 임시 기본값 1.0(원가 판매).
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Store", meta=(ClampMin="0.0", ClampMax="10.0"))
	float SellPriceRatio = 1.0f;

	// 창고 표시 슬롯 수. 창고 UI(WBP_LobbyStorage)의 설정값과 맞춘다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Store", meta=(ClampMin="1"))
	int32 MaxWarehouseSlotCount = 30;

	// 자동 새로고침 주기(초). 시간이 다 되면 재고를 리셋하고 타이머를 다시 시작한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Store", meta=(ClampMin="1.0"))
	float RefreshIntervalSeconds = 300.0f;

private:
	UFUNCTION() void HandleBackClicked();
	UFUNCTION() void HandleCategoryTabClicked(ULSVendingButtonWidget* ClickedButton);
	UFUNCTION() void HandleSlotClicked(ULSVendingSlotWidget* ClickedSlot);
	UFUNCTION() void HandleSlotDropped(ULSVendingSlotWidget* SourceSlot, ULSVendingSlotWidget* TargetSlot);
	UFUNCTION() void HandleTradeClicked();
	UFUNCTION() void HandleDecreaseClicked();
	UFUNCTION() void HandleIncreaseClicked();
	UFUNCTION() void HandleYesClicked();
	UFUNCTION() void HandleNoClicked();

	// 골드 변경 구독 콜백. 상단 골드 표시를 갱신한다.
	void HandleGoldChanged();

	// 1초 주기 타이머 콜백. 카운트다운을 갱신하고 0이 되면 재고를 리셋한다.
	void HandleRefreshTimerTick();

	// 재고를 DT_StoreStock의 Stock_Max로 채우고 카운트다운을 다시 시작한다.
	void ResetStock();

	// 남은 새로고침 시간을 "00 : 05 : 00" 형식으로 표시한다.
	void UpdateRefreshTimerText() const;

	// 현재 재고. 없으면(테이블에 없는 행) 0.
	int32 GetCurrentStock(FName ItemRowName) const;

	// 수량 팝업을 현재 선택 아이템/최대 수량으로 열고 키보드 포커스를 가져온다.
	void OpenQuantityDialog(int32 InMaxQuantity);
	void CloseQuantityDialog() const;
	bool IsQuantityDialogOpen() const;
	void RefreshDialogQuantityText() const;

	void SetCategory(ELSVendingCategory NewCategory);
	void RefreshGoldText() const;
	// DT_StoreStock에서 현재 카테고리 아이템만 골라 우측 목록을 다시 만든다.
	void RebuildStockList();
	// 가방/안전슬롯/창고 세 패널을 세이브 데이터로 다시 만든다.
	void RebuildOwnedPanels();
	// 한 패널(WrapBox)을 슬롯 배열로 채운다. 빈 칸은 만들지 않고 채워진 아이템만 나열한다.
	void RebuildOwnedBox(UWrapBox* TargetBox, ELSInventorySlotArea Area, const TArray<FLSSessionItem>& Items, int32 MaxSlotCount, UTextBlock* CountText);

	// 풀링: TargetBox의 ChildIndex 자리 슬롯을 재사용하고, 없으면 만들어 붙인다. 매 리빌드마다 파괴/생성하지 않는다.
	ULSVendingSlotWidget* GetOrCreateSlotWidget(UWrapBox* TargetBox, int32 ChildIndex);

	// 풀링: UsedCount 뒤로 남는 슬롯을 뒤에서부터 제거한다(목록이 줄어든 경우).
	void TrimSlotWidgets(UWrapBox* TargetBox, int32 UsedCount) const;
	void SelectSlot(ULSVendingSlotWidget* SlotWidget);
	void ClearSelection();
	void ExecuteBuy(int32 Quantity);
	void ExecuteSell(int32 Quantity);
	ULSSaveSubsystem* GetSaveSubsystem() const;
	// Row Name 접두사와 Item_Type으로 상품 카테고리를 판정한다.
	static ELSVendingCategory ResolveCategory(FName ItemRowName);

	// 현재 분류 버튼 선택 상태. 재료가 기본(현재 상품이 재료뿐이라 첫 화면부터 목록이 보이게).
	ELSVendingCategory CurrentCategory = ELSVendingCategory::Material;

	// 현재 선택 정보. 슬롯 위젯은 리빌드로 사라질 수 있어 값을 복사해 둔다.
	bool bHasSelection = false;
	bool bSelectedStock = false;
	FName SelectedRowName;
	int32 SelectedUnitPrice = 0;
	int32 SelectedAmount = 0;
	ELSInventorySlotArea SelectedArea = ELSInventorySlotArea::Inventory;
	int32 SelectedSlotIndex = INDEX_NONE;

	// 선택 강조 해제용. 리빌드되면 무효가 되므로 약참조로 둔다.
	TWeakObjectPtr<ULSVendingSlotWidget> SelectedSlotWidget;

	// 수량 팝업 상태. ◀/▶로 1~DialogMaxQuantity 사이를 오간다.
	int32 DialogQuantity = 1;
	int32 DialogMaxQuantity = 1;

	// 상품별 현재/최대 재고. 위젯 수명 동안만 유지하는 세션 상태(저장 안 함). 새로고침 시 최대로 리셋.
	TMap<FName, int32> CurrentStockByRow;
	TMap<FName, int32> MaxStockByRow;

	// 다음 자동 새로고침까지 남은 시간(초). 1초 타이머로 감소한다.
	float RemainingRefreshSeconds = 0.0f;

	FTimerHandle RefreshTimerHandle;
};
