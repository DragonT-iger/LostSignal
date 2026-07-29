#include "UI/Inventory/LSItemSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Core/LSPlayerControllerBase.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/ChipSystem/LSChipEquipmentSlotWidget.h"
#include "UI/ChipSystem/LSChipStationWidget.h"
#include "UI/Inventory/LSInventoryDragDropOperation.h"
#include "UI/Inventory/LSInventoryWidget.h"
#include "UI/LootDrop/LSLootDropWidget.h"
#include "UI/Storage/LSLobbyStorageWidget.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
// 빈 슬롯(기본 텍스처)이 적용된 상태를 나타내는 예약 키. 실제 아이템 행 이름과 겹치지 않는다.
const FName EmptySlotIconKey(TEXT("__LSEmptySlot__"));
// 미공개 placeholder(미확인 아이콘)가 적용된 상태를 나타내는 예약 키.
const FName PlaceholderIconKey(TEXT("__LSUnconfirmed__"));
}

ULSItemSlotWidget::ULSItemSlotWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 모든 화면이 공유하는 슬롯 배경 프레임 기본값. WBP에서 다른 텍스처를 지정하면 그 값이 우선한다.
	static ConstructorHelpers::FObjectFinder<UTexture2D> InventorySlotTextureFinder(TEXT("/Game/LostSignal/UI/Texture/Figma/InventorySlot.InventorySlot"));
	if (InventorySlotTextureFinder.Succeeded())
	{
		DefaultSlotTexture = InventorySlotTextureFinder.Object;
	}
}

void ULSItemSlotWidget::SetSlotLayoutSize(const FVector2D InSize)
{
	USizeBox* RootSizeBox = Cast<USizeBox>(GetRootWidget());
	if (!RootSizeBox)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set item slot layout size because the root widget is not a SizeBox on %s."), *GetNameSafe(this));
		return;
	}

	RootSizeBox->SetWidthOverride(FMath::Max(1.f, InSize.X));
	RootSizeBox->SetHeightOverride(FMath::Max(1.f, InSize.Y));
}

void ULSItemSlotWidget::ApplyDefaultSlotLayoutSize()
{
	// 루트가 SizeBox가 아닌 슬롯은 기본 크기를 적용할 수단이 없으므로 조용히 넘어간다.
	// (명시적으로 크기를 요구하는 SetSlotLayoutSize 쪽에서 경고를 남긴다)
	USizeBox* RootSizeBox = Cast<USizeBox>(GetRootWidget());
	if (!RootSizeBox)
	{
		return;
	}

	if (!RootSizeBox->IsWidthOverride())
	{
		RootSizeBox->SetWidthOverride(FMath::Max(1.f, DefaultSlotLayoutSize.X));
	}

	if (!RootSizeBox->IsHeightOverride())
	{
		RootSizeBox->SetHeightOverride(FMath::Max(1.f, DefaultSlotLayoutSize.Y));
	}
}

void ULSItemSlotWidget::SetItem(const FName ItemRowName, const int32 Amount, const TArray<FLSChipResolvedStat>& ChipStats)
{
	if (!ItemIconImage)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemIconImage is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (!AmountText)
	{
		UE_LOG(LogLS, Warning, TEXT("AmountText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (ItemRowName.IsNone())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set item slot because ItemRowName is none on %s."), *GetNameSafe(this));
		ClearItem();
		return;
	}

	// 미공개 placeholder였다가 실제 아이템이 들어오는 전환이면 등장 pop-in을 재생한다.
	// 아래 본문(ApplyHoverVisual 등)이 정상 아이템으로 동작하도록 플래그를 먼저 끈다.
	const bool bWasPlaceholder = bIsPlaceholder;
	bIsPlaceholder = false;

	// 같은 아이템을 다시 표시하는 경우 동기 텍스처 로딩과 브러시 갱신을 건너뛴다.
	if (DisplayedIconKey != ItemRowName)
	{
		UTexture2D* IconTexture = LoadIconTextureByRowName(ItemRowName);
		const bool bUsedItemIcon = IconTexture != nullptr;
		if (!IconTexture)
		{
			IconTexture = LoadDefaultIconTexture();
			UE_LOG(LogLS, Warning, TEXT("Using default item slot icon for row '%s' on %s."), *ItemRowName.ToString(), *GetNameSafe(this));
		}

		if (IconTexture)
		{
			ItemIconImage->SetBrushFromTexture(IconTexture);
		}

		// 실제 아이템 아이콘이 적용된 경우에만 캐시한다. 기본 아이콘 대체 시에는 다음 호출에서 다시 시도한다.
		DisplayedIconKey = bUsedItemIcon ? ItemRowName : NAME_None;
	}

	CurrentGradeBackgroundColor = ResolveGradeBackgroundColor(ItemRowName);

	ApplyHoverVisual();
	ItemIconImage->SetVisibility(ESlateVisibility::Visible);
	const bool bShouldShowAmount = Amount > 0 && LSInventorySlotUtils::ResolveItemMaxStack(ItemRowName, TEXT("ULSItemSlotWidget::SetItem")) > 1;
	AmountText->SetText(bShouldShowAmount ? FText::AsNumber(Amount) : FText::GetEmpty());
	AmountText->SetVisibility(bShouldShowAmount ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	SetTooltipItem(ItemRowName, Amount, ChipStats);
	DragItemRowName = ItemRowName;
	DragAmount = Amount;
	DragChipStats = ChipStats;
	bHasItem = true;

	// placeholder → 아이템 전환에서만 등장 pop-in을 시작한다. (인벤토리 등 일반 SetItem은 팝인 없음)
	if (bWasPlaceholder)
	{
		bIsPopInAnimating = true;
		PopInElapsed = 0.f;
	}
}

void ULSItemSlotWidget::ClearItem()
{
	if (!ItemIconImage)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemIconImage is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (!AmountText)
	{
		UE_LOG(LogLS, Warning, TEXT("AmountText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	// 배경은 SlotBackgroundImage가 항상 표시하므로 빈 슬롯에서는 아이콘만 숨긴다.
	// 단, 빈 칸 기본 아이콘이 지정된 슬롯(장비칸)은 숨기는 대신 그 아이콘을 표시한다.
	// 캐시 키(DisplayedIconKey)가 아니라 브러시가 실제로 그 텍스처를 들고 있는지로 판정한다.
	// 키로 판정하면 EmptySlotIconTexture를 나중에 교체했을 때(디자이너에서 값 할당 등) 갱신을 건너뛰어
	// 이전 브러시가 그대로 드러난다.
	const bool bShowEmptySlotIcon = EmptySlotIconTexture != nullptr;
	if (bShowEmptySlotIcon && ItemIconImage->GetBrush().GetResourceObject() != EmptySlotIconTexture)
	{
		ItemIconImage->SetBrushFromTexture(EmptySlotIconTexture);
	}
	DisplayedIconKey = EmptySlotIconKey;
	// 빈 슬롯은 등급색을 쓰지 않고 전용 빈 슬롯 배경색으로 되돌린다.
	CurrentGradeBackgroundColor = EmptySlotBackgroundColor;
	// placeholder/등장 연출 상태도 정리한다(공개됐다가 looted된 빈 칸, 풀 재사용 등).
	bIsPlaceholder = false;
	bIsPopInAnimating = false;

	ApplyHoverVisual();
	ItemIconImage->SetVisibility(bShowEmptySlotIcon ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	AmountText->SetText(FText::GetEmpty());
	AmountText->SetVisibility(ESlateVisibility::Collapsed);
	bHasItem = false;
	DragItemRowName = NAME_None;
	DragAmount = 0;
	DragChipStats.Reset();
	ClearTooltipItem();
}

void ULSItemSlotWidget::SetPlaceholder()
{
	if (!ItemIconImage)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemIconImage is not bound on %s."), *GetNameSafe(this));
		return;
	}

	bIsPlaceholder = true;
	bIsPopInAnimating = false;
	bHasItem = false;
	DragItemRowName = NAME_None;
	DragAmount = 0;
	DragChipStats.Reset();
	ClearTooltipItem();

	// 미공개이므로 등급색을 쓰지 않고 빈 슬롯 배경색을 유지(슬롯 프레임은 항상 또렷).
	CurrentGradeBackgroundColor = EmptySlotBackgroundColor;
	if (SlotBackgroundImage)
	{
		SlotBackgroundImage->SetColorAndOpacity(CurrentGradeBackgroundColor);
	}

	// 미확인 아이콘 브러시 적용(미설정 시 기본 슬롯 텍스처로 폴백).
	if (DisplayedIconKey != PlaceholderIconKey)
	{
		UTexture2D* IconTexture = UnconfirmedIconTexture ? UnconfirmedIconTexture.Get() : DefaultSlotTexture.Get();
		if (!IconTexture)
		{
			UE_LOG(LogLS, Warning, TEXT("UnconfirmedIconTexture/DefaultSlotTexture not set for loot placeholder on %s."), *GetNameSafe(this));
		}
		else
		{
			ItemIconImage->SetBrushFromTexture(IconTexture);
		}
		DisplayedIconKey = PlaceholderIconKey;
	}
	ItemIconImage->SetVisibility(ESlateVisibility::Visible);

	// 펄스 시작값(다음 NativeTick부터 sin으로 갱신).
	ItemIconImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, PlaceholderPulseMinAlpha));

	if (AmountText)
	{
		AmountText->SetText(FText::GetEmpty());
		AmountText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// pop-in/호버 강조와 무관하게 슬롯 자체는 정상 크기·불투명(아이콘만 펄스).
	SetRenderScale(FVector2D::UnitVector);
	SetRenderOpacity(1.f);
}

void ULSItemSlotWidget::SetSlotLocked(const bool bInLocked)
{
	bIsLocked = bInLocked;
	ApplyHoverVisual();
}

void ULSItemSlotWidget::SetAmountTextVisible(const bool bVisible) const
{
	if (AmountText)
	{
		AmountText->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void ULSItemSlotWidget::SetEquipCandidateHighlight(const bool bInIsCandidate)
{
	if (bIsEquipCandidate == bInIsCandidate)
	{
		return;
	}

	bIsEquipCandidate = bInIsCandidate;
	// 후보 해제 시 펄스로 늘어난 스케일을 즉시 원복한다(NativeTick이 더 이상 돌지 않으므로).
	// 호버 중이면 이어지는 ApplyHoverVisual이 호버 강조 스케일로 다시 맞춘다.
	if (!bIsEquipCandidate)
	{
		SetRenderScale(FVector2D::UnitVector);
	}
	ApplyHoverVisual();
}

void ULSItemSlotWidget::SetInvalidEquipDropHighlight(const bool bInIsInvalid)
{
	if (bIsInvalidEquipDropTarget == bInIsInvalid)
	{
		return;
	}

	bIsInvalidEquipDropTarget = bInIsInvalid;
	ApplyHoverVisual();
}

void ULSItemSlotWidget::PlayEquipDropRejectedFeedback()
{
	bIsDragTarget = false;
	bIsInvalidEquipDropTarget = false;
	bIsEquipRejectAnimating = true;
	EquipRejectElapsed = 0.f;
	SetRenderTranslation(FVector2D::ZeroVector);
	ApplyHoverVisual();
}

void ULSItemSlotWidget::SetDisplayOnlySlotContext()
{
	InventoryWidget.Reset();
	LootDropWidget.Reset();
	LobbyStorageWidget.Reset();
	ChipStationWidget.Reset();
	ChipEquipmentSlotWidget.Reset();
	SlotArea = ELSInventorySlotArea::Inventory;
	SlotIndex = INDEX_NONE;
	EquipmentSlotIndex = INDEX_NONE;
	bHasItem = false;
	bIsLocked = false;
}

void ULSItemSlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	ApplyDefaultSlotLayoutSize();
	ApplySlotBackground();

	if (!bHasItem)
	{
		ClearItem();
	}
}

void ULSItemSlotWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 연출이 없는 평상시 슬롯은 비용이 들지 않도록 조기 종료한다(모든 슬롯이 공유하는 클래스).
	if (!bIsPopInAnimating && !bIsPlaceholder && !bIsEquipCandidate && !bIsEquipRejectAnimating)
	{
		return;
	}

	// 등장 pop-in: 스케일(PopInStartScale→1.0) + 페이드(0→1). LSChipStatWidget과 동일한 이징.
	if (bIsPopInAnimating)
	{
		PopInElapsed += InDeltaTime;
		const float Duration = FMath::Max(KINDA_SMALL_NUMBER, PopInDuration);
		const float LinearAlpha = FMath::Clamp(PopInElapsed / Duration, 0.f, 1.f);
		const float Alpha = FMath::InterpEaseInOut(0.f, 1.f, LinearAlpha, 2.f);

		const float Scale = FMath::Lerp(PopInStartScale, 1.f, Alpha);
		SetRenderScale(FVector2D(Scale, Scale));
		SetRenderOpacity(Alpha);

		if (LinearAlpha >= 1.f)
		{
			SetRenderScale(FVector2D::UnitVector);
			SetRenderOpacity(1.f);
			bIsPopInAnimating = false;
		}
		return;
	}

	// 미공개 placeholder 펄스: 미확인 아이콘 알파만 sin으로 진동(슬롯 프레임 배경은 또렷하게 유지).
	if (bIsPlaceholder && ItemIconImage)
	{
		const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		const float PulseT = (FMath::Sin(Time * PlaceholderPulseSpeed * 2.f * PI) + 1.f) * 0.5f;
		const float PulseAlpha = FMath::Lerp(PlaceholderPulseMinAlpha, PlaceholderPulseMaxAlpha, PulseT);
		ItemIconImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, PulseAlpha));
		return;
	}

	if (bIsEquipRejectAnimating)
	{
		TickEquipDropRejectedFeedback(InDeltaTime);
		return;
	}

	// 장착 후보 펄스: 드래그 중 이 아이템이 들어갈 장비칸을 스케일 진동으로 강조한다.
	// 커서가 슬롯 위에 있으면(호버/드래그 타겟) 그쪽 강조가 우선이라 펄스는 양보한다.
	if (bIsEquipCandidate && !bIsHovered && !bIsDragTarget)
	{
		const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		const float PulseT = (FMath::Sin(Time * EquipCandidatePulseSpeed * 2.f * PI) + 1.f) * 0.5f;
		const float Scale = FMath::Lerp(EquipCandidatePulseMinScale, EquipCandidatePulseMaxScale, PulseT);
		SetRenderScale(FVector2D(Scale, Scale));
	}
}

void ULSItemSlotWidget::TickEquipDropRejectedFeedback(const float InDeltaTime)
{
	EquipRejectElapsed += InDeltaTime;
	const float Duration = FMath::Max(KINDA_SMALL_NUMBER, EquipRejectDuration);
	const float LinearAlpha = FMath::Clamp(EquipRejectElapsed / Duration, 0.f, 1.f);
	const float Oscillation = FMath::Sin(LinearAlpha * FMath::Max(1, EquipRejectShakeCount) * 2.f * PI);
	const float OffsetX = Oscillation * EquipRejectShakeAmplitude * (1.f - LinearAlpha);
	SetRenderTranslation(FVector2D(OffsetX, 0.f));

	if (LinearAlpha >= 1.f)
	{
		SetRenderTranslation(FVector2D::ZeroVector);
		bIsEquipRejectAnimating = false;
		ApplyHoverVisual();
	}
}

void ULSItemSlotWidget::SetSlotContext(ULSInventoryWidget* InInventoryWidget, const ELSInventorySlotArea InSlotArea, const int32 InSlotIndex, const bool bInHasItem, const bool bInLocked)
{
	InventoryWidget = InInventoryWidget;
	LootDropWidget.Reset();
	LobbyStorageWidget.Reset();
	ChipStationWidget.Reset();
	ChipEquipmentSlotWidget.Reset();
	SlotArea = InSlotArea;
	SlotIndex = InSlotIndex;
	EquipmentSlotIndex = INDEX_NONE;
	bHasItem = bInHasItem;
	bIsLocked = bInLocked;
	ApplyHoverVisual();
}

void ULSItemSlotWidget::SetLootSlotContext(ULSLootDropWidget* InLootDropWidget, const int32 InSlotIndex, const bool bInHasItem)
{
	LootDropWidget = InLootDropWidget;
	InventoryWidget.Reset();
	LobbyStorageWidget.Reset();
	ChipStationWidget.Reset();
	ChipEquipmentSlotWidget.Reset();
	SlotArea = ELSInventorySlotArea::Inventory;
	SlotIndex = InSlotIndex;
	EquipmentSlotIndex = INDEX_NONE;
	bHasItem = bInHasItem;
	bIsLocked = false;
	ApplyHoverVisual();
}

void ULSItemSlotWidget::SetWarehouseSlotContext(ULSLobbyStorageWidget* InStorageWidget, const ELSInventorySlotArea InSlotArea, const int32 InSlotIndex, const bool bInHasItem)
{
	LobbyStorageWidget = InStorageWidget;
	InventoryWidget.Reset();
	LootDropWidget.Reset();
	ChipStationWidget.Reset();
	ChipEquipmentSlotWidget.Reset();
	SlotArea = InSlotArea;
	SlotIndex = InSlotIndex;
	EquipmentSlotIndex = INDEX_NONE;
	bHasItem = bInHasItem;
	bIsLocked = false;
	ApplyHoverVisual();
}

void ULSItemSlotWidget::SetChipStationSlotContext(ULSChipStationWidget* InChipStationWidget, const ELSInventorySlotArea InSourceArea, const int32 InSourceSlotIndex, const FName InItemRowName, const int32 InAmount, const TArray<FLSChipResolvedStat>& InChipStats)
{
	ChipStationWidget = InChipStationWidget;
	InventoryWidget.Reset();
	LootDropWidget.Reset();
	LobbyStorageWidget.Reset();
	ChipEquipmentSlotWidget.Reset();
	SlotArea = InSourceArea;
	SlotIndex = InSourceSlotIndex;
	EquipmentSlotIndex = INDEX_NONE;
	DragItemRowName = InItemRowName;
	DragAmount = InAmount;
	DragChipStats = InChipStats;
	bHasItem = !InItemRowName.IsNone() && InAmount > 0;
	bIsLocked = false;
	ApplyHoverVisual();
}

void ULSItemSlotWidget::SetChipEquipmentSlotContext(ULSChipEquipmentSlotWidget* InChipEquipmentSlotWidget, ULSChipStationWidget* InChipStationWidget, const int32 InEquipmentSlotIndex)
{
	ChipEquipmentSlotWidget = InChipEquipmentSlotWidget;
	InventoryWidget.Reset();
	LootDropWidget.Reset();
	LobbyStorageWidget.Reset();
	ChipStationWidget = InChipStationWidget;
	SlotArea = ELSInventorySlotArea::Inventory;
	SlotIndex = INDEX_NONE;
	EquipmentSlotIndex = InEquipmentSlotIndex;
	bIsLocked = false;
	ApplyHoverVisual();
}

void ULSItemSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	bIsHovered = true;
	if (ULSLootDropWidget* OwningLootDropWidget = LootDropWidget.Get())
	{
		OwningLootDropWidget->NotifyLootSlotHovered(SlotIndex);
	}
	if (IsQuickTransferPointerEvent(InMouseEvent))
	{
		TryHandleQuickTransfer();
	}
	// 드래그 없이 호버만 해도 장착 가능 아이템이면 빈 장비칸에 후보 하이라이트를 켠다.
	UpdateEquipHoverHint();
	ApplyHoverVisual();
}

void ULSItemSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	bIsHovered = false;
	if (ULSLootDropWidget* OwningLootDropWidget = LootDropWidget.Get())
	{
		OwningLootDropWidget->NotifyLootSlotUnhovered(SlotIndex);
	}
	// 호버로 켠 장비칸 후보 하이라이트를 끈다(드래그로 이어진 경우는 아래 handoff로 이 플래그가 꺼져 건드리지 않음).
	ClearEquipHoverHint();
	ApplyHoverVisual();
}

FReply ULSItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 새 클릭 제스처 시작. 이전 더블클릭이 남긴 드래그 억제 플래그를 여기서 해제해,
	// 정상 드래그가 억제된 채로 남는 일이 없게 한다.
	bSuppressNextDragDetect = false;

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && InMouseEvent.IsShiftDown())
	{
		// 옮길 대상(룻박스/창고/장착)이 있을 때만 빠른이동으로 클릭을 소비한다.
		// 대상이 없으면(인벤토리만 열림) 아래 드래그 감지로 넘어가, 달리기(Shift)로 Shift가 눌려 있어도 월드 드랍 드래그가 가능하다.
		if (TryHandleQuickTransfer())
		{
			return FReply::Handled();
		}
	}

	if (!CanStartItemDrag())
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}

FReply ULSItemSlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && TryHandleQuickTransfer())
	{
		// 더블클릭 첫 Down이 DetectDragIfPressed로 무장한 드래그를 취소한다. 캡처를 놓지 않으면
		// 빠른이동으로 슬롯 풀이 재구성된 뒤 뒤늦게 NativeOnDragDetected가 발화해, 재사용된 슬롯의
		// (이미 다른 아이템이 된) 데이터로 드래그가 시작돼 구조가 꼬인다.
		// ReleaseMouseCapture로 무장을 풀고, 안전망으로 다음 드래그 감지 한 번을 억제한다.
		bSuppressNextDragDetect = true;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

FReply ULSItemSlotWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Shift+좌클릭을 누른 채 커서를 슬롯 위로 쓸면 지나가는 칸을 차례로 빠른이동한다.
	// (장착된 칸은 ClearItem으로 비워져 bHasItem=false가 되므로 재호출돼도 무해)
	if (IsQuickTransferPointerEvent(InMouseEvent))
	{
		TryHandleQuickTransfer();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void ULSItemSlotWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	const ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	const bool bInvalidEquipmentTarget = DragOperation && InventoryWidget.IsValid() && SlotArea == ELSInventorySlotArea::Equipment
		&& !InventoryWidget->CanAcceptEquipmentDrop(DragOperation->DragItemRowName, SlotIndex);
	SetInvalidEquipDropHighlight(bInvalidEquipmentTarget);
	bIsDragTarget = IsValidInventoryDropTarget(InOperation) || IsValidLootDropTarget(InOperation) || IsValidWarehouseDropTarget(InOperation)
		|| IsValidChipEquipmentDropTarget(InOperation) || IsValidChipStationDropTarget(InOperation);
	ApplyHoverVisual();
}

void ULSItemSlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	bIsDragTarget = false;
	SetInvalidEquipDropHighlight(false);
	ApplyHoverVisual();
}

void ULSItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	// 더블클릭 빠른이동이 방금 슬롯을 바꿨다면, 첫 Down이 무장했던 드래그가 뒤늦게 여기로 발화한 것이다.
	// 이 한 번을 억제해 재사용된(다른 아이템이 된) 슬롯을 잡지 않게 한다. (다음 Down에서 플래그 해제)
	if (bSuppressNextDragDetect)
	{
		bSuppressNextDragDetect = false;
		return;
	}

	if (!CanStartItemDrag())
	{
		return;
	}

	ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(
		UWidgetBlueprintLibrary::CreateDragDropOperation(ULSInventoryDragDropOperation::StaticClass()));
	if (!DragOperation)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to create inventory drag operation on %s."), *GetNameSafe(this));
		return;
	}

	DragOperation->SourceInventoryWidget = InventoryWidget.Get();
	DragOperation->SourceLootDropWidget = LootDropWidget.Get();
	DragOperation->SourceLobbyStorageWidget = LobbyStorageWidget.Get();
	DragOperation->SourceChipStationWidget = ChipStationWidget.Get();
	DragOperation->SourceChipEquipmentSlotWidget = ChipEquipmentSlotWidget.Get();
	DragOperation->SourceSlotWidget = this;
	DragOperation->SourceSlotIndex = SlotIndex;
	DragOperation->SourceEquipmentSlotIndex = EquipmentSlotIndex;
	DragOperation->SourceSlotArea = SlotArea;
	DragOperation->DragItemRowName = DragItemRowName;
	DragOperation->DragAmount = DragAmount;
	DragOperation->DragChipStats = DragChipStats;
	// 드래그 비주얼은 반드시 별도 인스턴스로 만든다. 살아있는 소스 위젯(this)을 비주얼로 쓰면
	// 드래그 종료 후 그 위젯의 Slate 드래그 감지가 영구히 죽어, 풀링 재사용 시 해당 슬롯이 드래그 불가가 된다.
	// SetItem만 호출해 bHasItem=true로 둔다(SetDisplayOnlySlotContext는 bHasItem=false로 만들어 PreConstruct에서 아이콘이 지워진다).
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		if (ULSItemSlotWidget* DragVisual = CreateWidget<ULSItemSlotWidget>(OwningPlayer, GetClass()))
		{
			// 드래그 비주얼은 아이템 아이콘만 커서를 따라가도록 배경 프레임을 숨긴다.
			DragVisual->bIsDragVisual = true;
			DragVisual->SetItem(DragItemRowName, DragAmount, DragChipStats);
			DragOperation->DefaultDragVisual = DragVisual;
		}
	}
	DragOperation->Pivot = EDragPivot::MouseDown;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(0.25f);
	OutOperation = DragOperation;

	// 이 아이템이 장착될 장비칸을 강조한다(로비 장비 편집 전용). 드래그 종료 시 RestoreDragSourceVisual이 끈다.
	// 호버 힌트로 이미 켜져 있었다면 드래그 하이라이트로 소유권을 넘긴다 — 플래그를 끄면 드래그 시작 직후 오는
	// NativeOnMouseLeave가 이 하이라이트를 지우지 않는다(드래그 종료 시 RestoreDragSourceVisual이 정리).
	bShowingEquipHoverHint = false;
	if (ALSPlayerControllerBase* LSPlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		if (ULSInventoryWidget* LobbyInventory = LSPlayerController->GetLobbyInventoryWidget())
		{
			LobbyInventory->SetEquipmentDragHighlight(DragItemRowName);
		}
	}
}

bool ULSItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	bIsDragTarget = false;
	SetInvalidEquipDropHighlight(false);
	ApplyHoverVisual();

	if (InventoryWidget.IsValid() && SlotArea == ELSInventorySlotArea::Equipment
		&& !InventoryWidget->CanAcceptEquipmentDrop(DragOperation->DragItemRowName, SlotIndex))
	{
		PlayEquipDropRejectedFeedback();
		return true;
	}

	if (bIsLocked)
	{
		return false;
	}

	if (ULSChipEquipmentSlotWidget* EquipmentSlotWidget = ChipEquipmentSlotWidget.Get())
	{
		return EquipmentSlotWidget->HandleChipDrop(*DragOperation);
	}

	if (ULSChipStationWidget* TargetChipStationWidget = ChipStationWidget.Get())
	{
		if (DragOperation->SourceChipEquipmentSlotWidget)
		{
			return TargetChipStationWidget->SwapEquippedChipWithStoredSlot(*DragOperation, this, SlotArea, SlotIndex);
		}

		return false;
	}

	if (ULSLootDropWidget* TargetLootDropWidget = LootDropWidget.Get())
	{
		if (DragOperation->SourceLootDropWidget)
		{
			if (DragOperation->SourceLootDropWidget != TargetLootDropWidget)
			{
				UE_LOG(LogLS, Warning, TEXT("Cannot drop loot slot because source/target loot drop widget does not match."));
				return false;
			}

			return TargetLootDropWidget->DropLootSlot(DragOperation->SourceSlotIndex, SlotIndex);
		}

		if (!DragOperation->SourceInventoryWidget)
		{
			return false;
		}

		const bool bTransferred = TargetLootDropWidget->TransferInventorySlotToLootSlot(
			DragOperation->SourceSlotArea,
			DragOperation->SourceSlotIndex,
			SlotIndex);
		if (bTransferred)
		{
			DragOperation->SourceInventoryWidget->RebuildInventorySlots();
			DragOperation->SourceInventoryWidget->RebuildConfirmedStorageSlots();
		}

		return bTransferred;
	}

	if (ULSLobbyStorageWidget* TargetStorageWidget = LobbyStorageWidget.Get())
	{
		if (!DragOperation->SourceInventoryWidget && !DragOperation->SourceLobbyStorageWidget)
		{
			return false;
		}

		const ELSInventorySlotArea SourceArea = DragOperation->SourceLobbyStorageWidget
			? ELSInventorySlotArea::Warehouse
			: DragOperation->SourceSlotArea;

		return TargetStorageWidget->HandleStorageSlotDrop(SourceArea, DragOperation->SourceSlotIndex, SlotIndex);
	}

	ULSInventoryWidget* TargetInventoryWidget = InventoryWidget.Get();
	if (TargetInventoryWidget && DragOperation->SourceLootDropWidget)
	{
		return TargetInventoryWidget->HandleLootSlotDrop(
			DragOperation->SourceLootDropWidget,
			DragOperation->SourceSlotIndex,
			SlotArea,
			SlotIndex);
	}

	if (TargetInventoryWidget && DragOperation->SourceLobbyStorageWidget)
	{
		const bool bDropped = TargetInventoryWidget->HandleInventorySlotDrop(
			ELSInventorySlotArea::Warehouse,
			DragOperation->SourceSlotIndex,
			SlotArea,
			SlotIndex);
		if (bDropped)
		{
			DragOperation->SourceLobbyStorageWidget->RefreshStorage();
		}
		return bDropped;
	}

	if (!TargetInventoryWidget || DragOperation->SourceInventoryWidget != TargetInventoryWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot because source/target inventory widget does not match."));
		return false;
	}

	return TargetInventoryWidget->HandleInventorySlotDrop(
		DragOperation->SourceSlotArea,
		DragOperation->SourceSlotIndex,
		SlotArea,
		SlotIndex);
}

void ULSItemSlotWidget::RestoreDragSourceVisual()
{
	SetVisibility(ESlateVisibility::Visible);
	SetRenderOpacity(1.0f);

	// 드래그 종료(성공/취소 공용)마다 장비칸 후보 하이라이트를 끈다.
	if (ALSPlayerControllerBase* LSPlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		if (ULSInventoryWidget* LobbyInventory = LSPlayerController->GetLobbyInventoryWidget())
		{
			LobbyInventory->ClearEquipmentDragHighlight();
		}
	}

	ApplyHoverVisual();
}

void ULSItemSlotWidget::ResetTransientSlotState()
{
	bIsHovered = false;
	bIsDragTarget = false;
	bIsEquipCandidate = false;
	bIsInvalidEquipDropTarget = false;
	bIsEquipRejectAnimating = false;
	EquipRejectElapsed = 0.f;
	SetVisibility(ESlateVisibility::Visible);
	SetRenderOpacity(1.0f);
	SetRenderScale(FVector2D::UnitVector);
	SetRenderTranslation(FVector2D::ZeroVector);
	// 진행 중이던 등장 pop-in은 멈춘다. (다음 NativeTick이 다시 적용하므로 공개 직후 슬롯은 다시 popin 됨)
	bIsPopInAnimating = false;
}

void ULSItemSlotWidget::RefreshHoverStateFromCursor()
{
	// 리빌드 과정에서 ResetTransientSlotState가 bIsHovered를 끄지만, 커서가 그대로 이 슬롯 위에 있으면
	// 풀링 재사용된 위젯 인스턴스가 바뀌지 않아 Slate가 새 MouseEnter를 보내지 않는다.
	// 실제 Slate 호버 상태를 직접 조회해 호버 강조를 복원한다.
	const bool bActuallyHovered = IsHovered();
	if (bIsHovered == bActuallyHovered)
	{
		return;
	}

	bIsHovered = bActuallyHovered;
	ApplyHoverVisual();
}

void ULSItemSlotWidget::ApplyHoverVisual()
{
	// 미공개 placeholder는 호버/등급색을 적용하지 않는다(아이콘 펄스는 NativeTick이 담당).
	if (bIsPlaceholder)
	{
		return;
	}

	FLinearColor Tint = NormalIconTint;
	if (bIsEquipRejectAnimating || bIsInvalidEquipDropTarget)
	{
		Tint = EquipRejectTint;
	}
	else if (bIsLocked)
	{
		Tint = LockedIconTint;
	}
	else if (bIsDragTarget)
	{
		Tint = DragTargetIconTint;
	}
	else if (bIsEquipCandidate)
	{
		Tint = EquipCandidateTint;
	}
	else
	{
		Tint = bIsHovered ? HoveredIconTint : NormalIconTint;
	}

	// 호버/드래그 대상/장착 후보는 피드백 틴트를 우선한다. 잠긴 슬롯은 아이콘만 흐리게 하고,
	// 아이템이 있는 배경은 등급색을 유지해 장비 등급을 계속 구분할 수 있게 한다.
	if (SlotBackgroundImage)
	{
		const bool bSpecialState = bIsDragTarget || bIsHovered || bIsEquipCandidate || bIsInvalidEquipDropTarget
			|| bIsEquipRejectAnimating || (bIsLocked && !bHasItem);
		SlotBackgroundImage->SetColorAndOpacity(bSpecialState ? Tint : CurrentGradeBackgroundColor);
	}

	if (ItemIconImage)
	{
		// 빈 칸 기본 아이콘은 실루엣이므로 특수 상태가 아닐 때만 전용 틴트로 흐리게 둔다.
		// 특수 상태에서는 기존 피드백 틴트를 그대로 써서 빈 장비칸에서도 후보/거부 강조가 보이게 한다.
		const bool bSpecialIconState = bIsDragTarget || bIsHovered || bIsEquipCandidate || bIsInvalidEquipDropTarget
			|| bIsEquipRejectAnimating || bIsLocked;
		const bool bShowingEmptySlotIcon = EmptySlotIconTexture != nullptr && DisplayedIconKey == EmptySlotIconKey;
		ItemIconImage->SetColorAndOpacity(bShowingEmptySlotIcon && !bSpecialIconState ? EmptySlotIconTint : Tint);
	}

	// 틴트에 더해 호버/드래그 타겟 시 슬롯을 살짝 키워 강조한다(잠금·드래그 비주얼 제외).
	// 장착 후보의 스케일은 NativeTick 펄스가 소유하므로 여기서는 건드리지 않는다(호버 시엔 호버 강조가 우선).
	if (!bIsEquipCandidate || bIsHovered || bIsDragTarget)
	{
		const bool bShouldEmphasize = !bIsLocked && !bIsDragVisual && (bIsHovered || bIsDragTarget);
		SetRenderScale(bShouldEmphasize ? HoveredRenderScale : FVector2D::UnitVector);
	}
}

void ULSItemSlotWidget::ApplySlotBackground()
{
	if (!SlotBackgroundImage)
	{
		UE_LOG(LogLS, Warning, TEXT("SlotBackgroundImage is not bound on %s."), *GetNameSafe(this));
		return;
	}

	// 드래그 비주얼은 아이템 아이콘만 표시하므로 배경 프레임을 숨긴다.
	if (bIsDragVisual)
	{
		SlotBackgroundImage->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// DefaultSlotTexture가 지정된 경우에만 배경 브러시를 덮어쓴다.
	// 미지정이면 WBP 디자이너에서 설정한 배경 브러시를 그대로 둔다.
	if (DefaultSlotTexture)
	{
		SlotBackgroundImage->SetBrushFromTexture(DefaultSlotTexture);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("DefaultSlotTexture is not set on %s; using WBP designer brush for slot background."), *GetNameSafe(this));
	}

	SlotBackgroundImage->SetVisibility(ESlateVisibility::Visible);
}

FLinearColor ULSItemSlotWidget::ResolveGradeBackgroundColor(const FName ItemRowName) const
{
	// 등급 토큰은 툴팁(GetGradeText)과 동일한 단일 출처에서 파싱한다.
	const FString Grade = LSInventorySlotUtils::ResolveItemGradeFromRowName(ItemRowName);
	if (Grade == TEXT("Supply"))       return SupplyGradeColor;
	if (Grade == TEXT("Standard"))     return StandardGradeColor;
	if (Grade == TEXT("Precision"))    return PrecisionGradeColor;
	if (Grade == TEXT("Tuning"))       return TuningGradeColor;
	if (Grade == TEXT("Prototype"))    return PrototypeGradeColor;
	if (Grade == TEXT("Masterpiece"))  return MasterpieceGradeColor;

	return DefaultGradeColor;
}

bool ULSItemSlotWidget::CanStartItemDrag() const
{
	if (bIsPlaceholder)
	{
		return false;
	}

	if (!bHasItem)
	{
		return false;
	}

	if (bIsLocked)
	{
		return false;
	}

	if (ChipStationWidget.IsValid())
	{
		if (ChipEquipmentSlotWidget.IsValid())
		{
			return EquipmentSlotIndex != INDEX_NONE && !DragItemRowName.IsNone() && DragAmount > 0;
		}

		return SlotIndex != INDEX_NONE && !DragItemRowName.IsNone() && DragAmount > 0;
	}

	return SlotIndex != INDEX_NONE && (InventoryWidget.IsValid() || LootDropWidget.IsValid() || LobbyStorageWidget.IsValid());
}

bool ULSItemSlotWidget::IsQuickTransferPointerEvent(const FPointerEvent& InMouseEvent) const
{
	return InMouseEvent.IsShiftDown() && InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton);
}

bool ULSItemSlotWidget::TryHandleQuickTransfer()
{
	// 미공개 placeholder는 빠른 이동(Shift+클릭) 불가.
	if (bIsPlaceholder)
	{
		return false;
	}

	// 무기/방어구 장착칸: Shift+좌클릭 -> 인벤토리 첫 빈 칸으로 해제. (SlotIndex가 유효하므로 아래 가드보다 먼저 처리)
	if (SlotArea == ELSInventorySlotArea::Equipment)
	{
		return TryHandleEquipmentQuickTransfer();
	}

	// 칩 장착 슬롯: Shift+좌클릭 -> 창고로 해제. (장착 슬롯은 SlotIndex가 INDEX_NONE이라 아래 가드보다 먼저 처리한다.)
	if (ChipEquipmentSlotWidget.IsValid())
	{
		return TryHandleChipEquipmentQuickTransfer();
	}

	if (!bHasItem || SlotIndex == INDEX_NONE)
	{
		return false;
	}

	// 칩 목록 슬롯(인벤토리/창고): Shift+좌클릭 -> 첫 빈 장착 슬롯에 순서대로 장착.
	if (ChipStationWidget.IsValid())
	{
		return TryHandleChipStationQuickTransfer();
	}

	if (LootDropWidget.IsValid())
	{
		return TryHandleLootQuickTransfer();
	}

	// 인벤토리의 장착 가능한 아이템은 Shift 빠른이동 시 빈 장비칸으로 "먼저" 장착한다.
	// 장착 불가 아이템이거나 대상 장비칸이 이미 차 있으면 false를 돌려주고 아래 컨테이너 이동으로 넘어간다.
	if (InventoryWidget.IsValid() && !bIsLocked && SlotArea == ELSInventorySlotArea::Inventory && TryHandleEquipFromInventoryQuickTransfer())
	{
		return true;
	}

	if (InventoryWidget.IsValid() && !bIsLocked)
	{
		return TryHandleInventoryQuickTransfer();
	}

	if (LobbyStorageWidget.IsValid())
	{
		return TryHandleWarehouseQuickTransfer();
	}

	return false;
}

bool ULSItemSlotWidget::TryHandleLootQuickTransfer()
{
	ULSLootDropWidget* OwningLootDropWidget = LootDropWidget.Get();
	if (!OwningLootDropWidget || !OwningLootDropWidget->TransferLootSlotToInventory(SlotIndex))
	{
		return false;
	}

	// 룻박스(소스) 슬롯은 TransferLootSlotToInventory가 내부에서 다시 그린다.
	// 대상(인벤토리/Safe 등)은 소스만 낙관적으로 비우지 않고 funnel로 전체를 데이터에서 다시 그린다.
	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		PlayerController->RefreshAllInventoryUI();
	}
	return true;
}

bool ULSItemSlotWidget::TryHandleInventoryQuickTransfer()
{
	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController || !PlayerController->TransferInventorySlotToOpenContainer(SlotArea, SlotIndex))
	{
		return false;
	}

	// 소스 슬롯만 낙관적으로 비우거나 authority 여부로 갱신을 나누지 않는다.
	// 성공하면 열려 있는 인벤토리 계열 패널 전체를 데이터에서 다시 그려 정합을 보장한다(부분 이동 포함).
	PlayerController->RefreshAllInventoryUI();
	return true;
}

bool ULSItemSlotWidget::TryHandleWarehouseQuickTransfer()
{
	ULSLobbyStorageWidget* StorageWidget = LobbyStorageWidget.Get();
	if (!StorageWidget || !StorageWidget->TransferStorageSlotToInventory(SlotIndex, false))
	{
		return false;
	}

	// 인벤토리가 가득 차 일부만 옮겨졌어도 소스(창고) 슬롯을 통째로 비우면 안 된다(남은 수량이 화면에서 사라지는 버그).
	// funnel로 창고·인벤토리를 데이터에서 다시 그려 남은 수량이 정확히 반영되게 한다.
	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		PlayerController->RefreshAllInventoryUI();
	}
	return true;
}

bool ULSItemSlotWidget::TryHandleChipEquipmentQuickTransfer()
{
	if (!bHasItem || EquipmentSlotIndex == INDEX_NONE)
	{
		return false;
	}

	ULSChipStationWidget* OwningChipStation = ChipStationWidget.Get();
	if (!OwningChipStation || !OwningChipStation->QuickUnequipEquippedChipToWarehouse(EquipmentSlotIndex))
	{
		return false;
	}

	// 장착칸 위젯 갱신은 다음 틱(QueueRefreshEquippedChipState)이라, 그때까지 bHasItem이 stale로 남는다.
	// Shift 쓸기(NativeOnMouseMove)가 같은 제스처에서 재호출해 빈 칸 해제를 반복 시도하지 않도록
	// 칩 리스트 빠른 장착과 동일하게 소스 칸을 즉시 비운다.
	ClearItem();
	return true;
}

bool ULSItemSlotWidget::TryHandleEquipmentQuickTransfer()
{
	if (!bHasItem || SlotIndex == INDEX_NONE || SlotArea != ELSInventorySlotArea::Equipment)
	{
		return false;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController)
	{
		return false;
	}

	// 인벤토리 첫 빈 칸을 찾는다. 레이드=서버가 미러링한 세션 인벤토리, 로비=클라 세이브. 최대 슬롯 수 내에서만.
	ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent();
	const bool bRaidActive = RaidInventory && RaidInventory->IsRaidActive();

	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;

	int32 EmptyInventoryIndex = INDEX_NONE;
	if (bRaidActive)
	{
		EmptyInventoryIndex = LSInventorySlotUtils::FindFirstEmptySlotIndex(
			RaidInventory->GetSessionInventory(), RaidInventory->GetMaxInventorySlotCount());
	}
	else if (SaveSubsystem)
	{
		EmptyInventoryIndex = LSInventorySlotUtils::FindFirstEmptySlotIndex(
			SaveSubsystem->GetInventory(), SaveSubsystem->GetMaxInventorySlotCount());
	}
	else
	{
		return false;
	}

	if (EmptyInventoryIndex == INDEX_NONE)
	{
		// 빈 칸이 없으면 이동하지 않고 "인벤토리가 가득 찼습니다" 알림만 띄운다. 클릭은 소비한다(드래그로 넘기지 않음).
		if (ULSInventoryWidget* OwningInventory = InventoryWidget.Get())
		{
			OwningInventory->ShowInventoryFullNotification();
		}
		return true;
	}

	bool bChanged = false;
	if (bRaidActive)
	{
		bChanged = PlayerController->DropInventorySlot(ELSInventorySlotArea::Equipment, SlotIndex, ELSInventorySlotArea::Inventory, EmptyInventoryIndex);
	}
	else
	{
		bChanged = SaveSubsystem->MoveEquipmentSlot(ELSInventorySlotArea::Equipment, SlotIndex, ELSInventorySlotArea::Inventory, EmptyInventoryIndex);
	}

	if (!bChanged)
	{
		return false;
	}

	// 열려 있는 인벤토리 계열 패널 전체를 데이터에서 다시 그린다(부분/누락 갱신 금지).
	PlayerController->RefreshAllInventoryUI();

	// 장착칸 갱신은 다음 틱(미러 RPC/RefreshAll)이라 그때까지 bHasItem이 stale로 남는다.
	// Shift 쓸기(NativeOnMouseMove)가 같은 제스처에서 재호출해 빈 칸 해제를 반복 시도하지 않도록 소스 칸을 즉시 비운다(칩 장착칸 패턴).
	ClearItem();
	return true;
}

bool ULSItemSlotWidget::TryHandleEquipFromInventoryQuickTransfer()
{
	if (SlotArea != ELSInventorySlotArea::Inventory || SlotIndex == INDEX_NONE || DragItemRowName.IsNone())
	{
		return false;
	}

	// 장착 불가 아이템이면 기존 컨테이너 이동으로 넘긴다.
	const ELSEquipmentSlot EquipType = LSInventorySlotUtils::ResolveEquipmentSlotType(DragItemRowName);
	if (EquipType == ELSEquipmentSlot::Count)
	{
		return false;
	}
	const int32 EquipmentSlotIdx = static_cast<int32>(EquipType);

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController)
	{
		return false;
	}

	ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent();
	const bool bRaidActive = RaidInventory && RaidInventory->IsRaidActive();

	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!bRaidActive && !SaveSubsystem)
	{
		return false;
	}

	// 대상 장비칸이 비어 있을 때만 "먼저 장착"한다. 차 있으면(교체 필요) false로 넘겨 컨테이너 이동에 맡긴다.
	if (!IsEquipmentSlotEmpty(EquipmentSlotIdx))
	{
		return false;
	}

	bool bChanged = false;
	if (bRaidActive)
	{
		bChanged = PlayerController->DropInventorySlot(ELSInventorySlotArea::Inventory, SlotIndex, ELSInventorySlotArea::Equipment, EquipmentSlotIdx);
	}
	else
	{
		bChanged = SaveSubsystem->MoveEquipmentSlot(ELSInventorySlotArea::Inventory, SlotIndex, ELSInventorySlotArea::Equipment, EquipmentSlotIdx);
	}

	if (!bChanged)
	{
		return false;
	}

	// 열려 있는 인벤토리 계열 패널 전체를 데이터에서 다시 그린다(부분/누락 갱신 금지).
	PlayerController->RefreshAllInventoryUI();

	// Shift 쓸기 재호출로 이미 옮긴 칸을 다시 건드리지 않도록 소스 칸을 즉시 비운다(칩 장착칸 패턴).
	ClearItem();
	return true;
}

bool ULSItemSlotWidget::IsEquipmentSlotEmpty(const int32 EquipmentSlotIdx) const
{
	const ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController)
	{
		return false;
	}

	if (const ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent())
	{
		if (RaidInventory->IsRaidActive())
		{
			FLSSessionItem EquipmentSlotItem;
			return !RaidInventory->GetSessionSlotItem(ELSInventorySlotArea::Equipment, EquipmentSlotIdx, EquipmentSlotItem)
				|| !LSInventorySlotUtils::IsFilled(EquipmentSlotItem);
		}
	}

	const UGameInstance* GameInstance = GetGameInstance();
	const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		return false;
	}
	const TArray<FLSSessionItem>& Equipment = SaveSubsystem->GetEquipmentSlots();
	return !Equipment.IsValidIndex(EquipmentSlotIdx) || !LSInventorySlotUtils::IsFilled(Equipment[EquipmentSlotIdx]);
}

ULSInventoryWidget* ULSItemSlotWidget::ResolveEquipHighlightWidget() const
{
	// 드래그 하이라이트와 동일하게 로비 인벤토리 위젯을 쓴다(로비 장비 편집 전용 하이라이트).
	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	return PlayerController ? PlayerController->GetLobbyInventoryWidget() : nullptr;
}

void ULSItemSlotWidget::UpdateEquipHoverHint()
{
	// 인벤토리의 장착 가능한 아이템을 호버하면, 대상 장비칸이 비어 있을 때만 후보 하이라이트를 켠다.
	// (드래그 없이도 "여기에 장착됨"을 알려주는 힌트. 이미 장착된 칸이면 켜지 않는다.)
	if (!InventoryWidget.IsValid() || SlotArea != ELSInventorySlotArea::Inventory || !bHasItem || DragItemRowName.IsNone())
	{
		return;
	}

	const ELSEquipmentSlot EquipType = LSInventorySlotUtils::ResolveEquipmentSlotType(DragItemRowName);
	if (EquipType == ELSEquipmentSlot::Count)
	{
		return;
	}

	if (!IsEquipmentSlotEmpty(static_cast<int32>(EquipType)))
	{
		return;
	}

	if (ULSInventoryWidget* HighlightWidget = ResolveEquipHighlightWidget())
	{
		HighlightWidget->SetEquipmentDragHighlight(DragItemRowName);
		bShowingEquipHoverHint = true;
	}
}

void ULSItemSlotWidget::ClearEquipHoverHint()
{
	if (!bShowingEquipHoverHint)
	{
		return;
	}

	bShowingEquipHoverHint = false;
	if (ULSInventoryWidget* HighlightWidget = ResolveEquipHighlightWidget())
	{
		HighlightWidget->ClearEquipmentDragHighlight();
	}
}

bool ULSItemSlotWidget::TryHandleChipStationQuickTransfer()
{
	ULSChipStationWidget* OwningChipStation = ChipStationWidget.Get();
	if (!OwningChipStation || !OwningChipStation->QuickEquipChipToFirstEmptyHardwareSlot(SlotArea, SlotIndex))
	{
		return false;
	}

	// 칩 리스트는 재정렬/리빌드하지 않고 이 소스 슬롯 한 칸만 비운다.
	ClearItem();
	return true;
}

void ULSItemSlotWidget::RefreshChipStationSlotFromStored()
{
	// 칩 스테이션 리스트 슬롯 전용. 장착/스왑으로 바뀐 저장 슬롯 내용을 이 칸에만 반영한다(정렬/리빌드 없음).
	if (!ChipStationWidget.IsValid() || SlotIndex == INDEX_NONE)
	{
		return;
	}

	// 드래그로 낮춘 가시성/투명도/스케일을 먼저 원복한다(빈 칸으로 만들 때도 hole이 정상 표시되도록).
	ResetTransientSlotState();

	const UGameInstance* GameInstance = GetGameInstance();
	const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		ClearItem();
		return;
	}

	// 칩 리스트 칸의 원본은 인벤토리 또는 창고다(SetChipStationSlotContext에서 지정).
	const TArray<FLSSessionItem>& Slots = SlotArea == ELSInventorySlotArea::Warehouse
		? SaveSubsystem->GetWarehouseItems()
		: SaveSubsystem->GetInventory();
	const FLSSessionItem* SlotItem = Slots.IsValidIndex(SlotIndex) ? &Slots[SlotIndex] : nullptr;
	if (SlotItem
		&& LSInventorySlotUtils::IsFilled(*SlotItem)
		&& ChipStationWidget->IsChipVisibleForCurrentFilter(SlotItem->ItemRowName))
	{
		SetItem(SlotItem->ItemRowName, SlotItem->Amount, SlotItem->ChipStats);
		SetChipStationSlotContext(ChipStationWidget.Get(), SlotArea, SlotIndex, SlotItem->ItemRowName, SlotItem->Amount, SlotItem->ChipStats);
		RefreshHoverStateFromCursor();
		return;
	}

	// 저장 슬롯이 비었으면(장착으로 빠져나감) 이 칸은 hole로 남긴다. 해제 시 InsertChipListSlot이 재사용한다.
	ClearItem();
}

bool ULSItemSlotWidget::IsValidInventoryDropTarget(const UDragDropOperation* InOperation) const
{
	const ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation || bIsLocked || !InventoryWidget.IsValid() || SlotIndex == INDEX_NONE || DragOperation->SourceSlotIndex == INDEX_NONE)
	{
		return false;
	}

	if (SlotArea == ELSInventorySlotArea::Equipment
		&& !InventoryWidget->CanAcceptEquipmentDrop(DragOperation->DragItemRowName, SlotIndex))
	{
		return false;
	}

	if (DragOperation->SourceLootDropWidget)
	{
		return true;
	}

	if (DragOperation->SourceInventoryWidget != InventoryWidget.Get())
	{
		return false;
	}

	return DragOperation->SourceSlotArea != SlotArea || DragOperation->SourceSlotIndex != SlotIndex;
}

bool ULSItemSlotWidget::IsValidLootDropTarget(const UDragDropOperation* InOperation) const
{
	const ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation || !LootDropWidget.IsValid() || SlotIndex == INDEX_NONE || DragOperation->SourceSlotIndex == INDEX_NONE)
	{
		return false;
	}

	if (DragOperation->SourceInventoryWidget)
	{
		return true;
	}

	return DragOperation->SourceLootDropWidget == LootDropWidget.Get() && DragOperation->SourceSlotIndex != SlotIndex;
}

bool ULSItemSlotWidget::IsValidWarehouseDropTarget(const UDragDropOperation* InOperation) const
{
	const ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation || !LobbyStorageWidget.IsValid() || SlotIndex == INDEX_NONE || DragOperation->SourceSlotIndex == INDEX_NONE)
	{
		return false;
	}

	if (DragOperation->SourceInventoryWidget)
	{
		return true;
	}

	if (DragOperation->SourceLobbyStorageWidget == LobbyStorageWidget.Get())
	{
		return DragOperation->SourceSlotIndex != SlotIndex;
	}

	return false;
}

bool ULSItemSlotWidget::IsValidChipEquipmentDropTarget(const UDragDropOperation* InOperation) const
{
	// 칩 장착 슬롯이 드롭 대상일 때의 유효 조건. HandleChipDrop의 수락 조건과 일치한다.
	const ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation || bIsLocked || !ChipEquipmentSlotWidget.IsValid())
	{
		return false;
	}

	if (!DragOperation->SourceChipStationWidget || DragOperation->DragItemRowName.IsNone() || DragOperation->DragAmount <= 0)
	{
		return false;
	}

	if (ChipStationWidget.IsValid() && DragOperation->SourceChipStationWidget != ChipStationWidget.Get())
	{
		return false;
	}

	return DragOperation->DragItemRowName.ToString().StartsWith(TEXT("Chip_"));
}

bool ULSItemSlotWidget::IsValidChipStationDropTarget(const UDragDropOperation* InOperation) const
{
	// 칩 목록 슬롯은 장착 슬롯에서 드래그해 온 경우(장착 해제 스왑)에만 유효 대상이다. NativeOnDrop의 칩 스테이션 분기와 일치한다.
	const ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation || bIsLocked || ChipEquipmentSlotWidget.IsValid() || !ChipStationWidget.IsValid())
	{
		return false;
	}

	return DragOperation->SourceChipEquipmentSlotWidget != nullptr;
}

UTexture2D* ULSItemSlotWidget::LoadIconTextureByRowName(const FName ItemRowName) const
{
	// 아이콘 경로 규칙은 LSInventorySlotUtils가 단일 출처다(인벤토리 슬롯·퀵슬롯 공용).
	return LSInventorySlotUtils::LoadItemIconTexture(ItemRowName);
}

UTexture2D* ULSItemSlotWidget::LoadDefaultIconTexture() const
{
	static const TCHAR* DefaultIconObjectPath = TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture");
	UTexture2D* DefaultIconTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, DefaultIconObjectPath));
	if (!DefaultIconTexture)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to load default inventory icon '%s'."), DefaultIconObjectPath);
	}

	return DefaultIconTexture;
}
