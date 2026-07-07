#include "UI/Skill/LSSkillLoadoutWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Data/LSCharacterSkillRow.h"
#include "Data/LSGameDataSubsystem.h"
#include "Engine/GameInstance.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "Skills/LSSkillDataAsset.h"
#include "Skills/LSSkillPoolDataAsset.h"
#include "UI/Skill/LSSkillLoadoutEntryWidget.h"

namespace
{
constexpr int32 SkillLoadoutSlotCount = 3;
}

void ULSSkillLoadoutWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Slot1Button) { Slot1Button->OnClicked.AddDynamic(this, &ULSSkillLoadoutWidget::HandleSlot1Clicked); }
	if (Slot2Button) { Slot2Button->OnClicked.AddDynamic(this, &ULSSkillLoadoutWidget::HandleSlot2Clicked); }
	if (Slot3Button) { Slot3Button->OnClicked.AddDynamic(this, &ULSSkillLoadoutWidget::HandleSlot3Clicked); }

	if (!SkillPool)
	{
		UE_LOG(LogLS, Warning, TEXT("SkillPool is not assigned on %s. Check WBP skill loadout defaults."), *GetNameSafe(this));
	}
	if (!CandidateEntryClass)
	{
		UE_LOG(LogLS, Warning, TEXT("CandidateEntryClass is not assigned on %s. Check WBP skill loadout defaults."), *GetNameSafe(this));
	}
	if (!CandidateContainer)
	{
		UE_LOG(LogLS, Warning, TEXT("CandidateContainer is not bound on %s."), *GetNameSafe(this));
	}

	if (ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem())
	{
		SkillLoadoutChangedHandle = SaveSubsystem->OnSkillLoadoutChanged.AddUObject(this, &ULSSkillLoadoutWidget::HandleSkillLoadoutChanged);
	}

	RefreshSkillLoadout();
}

void ULSSkillLoadoutWidget::NativeDestruct()
{
	if (Slot1Button) { Slot1Button->OnClicked.RemoveDynamic(this, &ULSSkillLoadoutWidget::HandleSlot1Clicked); }
	if (Slot2Button) { Slot2Button->OnClicked.RemoveDynamic(this, &ULSSkillLoadoutWidget::HandleSlot2Clicked); }
	if (Slot3Button) { Slot3Button->OnClicked.RemoveDynamic(this, &ULSSkillLoadoutWidget::HandleSlot3Clicked); }

	if (SkillLoadoutChangedHandle.IsValid())
	{
		if (ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem())
		{
			SaveSubsystem->OnSkillLoadoutChanged.Remove(SkillLoadoutChangedHandle);
		}
		SkillLoadoutChangedHandle.Reset();
	}

	Super::NativeDestruct();
}

void ULSSkillLoadoutWidget::RefreshSkillLoadout()
{
	RebuildCandidateList();
	RefreshEquippedSlots();
}

void ULSSkillLoadoutWidget::HandleSlot1Clicked() { ClearSlotByIndex(0); }
void ULSSkillLoadoutWidget::HandleSlot2Clicked() { ClearSlotByIndex(1); }
void ULSSkillLoadoutWidget::HandleSlot3Clicked() { ClearSlotByIndex(2); }

void ULSSkillLoadoutWidget::HandleEntryClicked(const int32 SkillID)
{
	ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem();
	if (!SaveSubsystem || SkillID == 0)
	{
		return;
	}

	const TArray<int32>& EquippedSkillIDs = SaveSubsystem->GetEquippedSkillIDs();

	// 이미 장착돼 있으면 무시(엔트리도 비활성화 상태다).
	if (EquippedSkillIDs.Contains(SkillID))
	{
		return;
	}

	// 첫 빈 슬롯을 찾아 장착한다.
	for (int32 SlotIndex = 0; SlotIndex < SkillLoadoutSlotCount; ++SlotIndex)
	{
		const bool bEmpty = !EquippedSkillIDs.IsValidIndex(SlotIndex) || EquippedSkillIDs[SlotIndex] == 0;
		if (bEmpty)
		{
			SaveSubsystem->SetEquippedSkillSlot(SlotIndex, SkillID);
			return;
		}
	}

	// 빈 슬롯이 없다: 사용자가 먼저 한 칸을 비워야 한다.
	UE_LOG(LogLS, Log, TEXT("[SkillLoadout] All %d skill slots are full. Skill %d not equipped."), SkillLoadoutSlotCount, SkillID);
}

void ULSSkillLoadoutWidget::HandleSkillLoadoutChanged()
{
	RefreshSkillLoadout();
}

void ULSSkillLoadoutWidget::RebuildCandidateList()
{
	CandidateEntries.Reset();
	if (CandidateContainer)
	{
		CandidateContainer->ClearChildren();
	}

	if (!SkillPool || !CandidateEntryClass || !CandidateContainer)
	{
		return;
	}

	const ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem();
	const TArray<int32> EquippedSkillIDs = SaveSubsystem ? SaveSubsystem->GetEquippedSkillIDs() : TArray<int32>();

	for (const TObjectPtr<ULSSkillDataAsset>& CandidateSkill : SkillPool->SelectableSkills)
	{
		if (!CandidateSkill)
		{
			continue;
		}

		const int32 SkillID = CandidateSkill->GetSkillID();
		if (!IsSelectableSkillType(SkillID))
		{
			continue;
		}

		ULSSkillLoadoutEntryWidget* Entry = CreateWidget<ULSSkillLoadoutEntryWidget>(this, CandidateEntryClass);
		if (!Entry)
		{
			continue;
		}

		Entry->SetSkillData(CandidateSkill);
		Entry->SetEquipped(EquippedSkillIDs.Contains(SkillID));
		Entry->OnEntryClicked.BindUObject(this, &ULSSkillLoadoutWidget::HandleEntryClicked);

		CandidateContainer->AddChild(Entry);
		CandidateEntries.Add(Entry);
	}
}

void ULSSkillLoadoutWidget::RefreshEquippedSlots()
{
	const ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem();
	const TArray<int32> EquippedSkillIDs = SaveSubsystem ? SaveSubsystem->GetEquippedSkillIDs() : TArray<int32>();

	for (int32 SlotIndex = 0; SlotIndex < SkillLoadoutSlotCount; ++SlotIndex)
	{
		UImage* SlotIcon = GetSlotIcon(SlotIndex);
		if (!SlotIcon)
		{
			continue;
		}

		const int32 SkillID = EquippedSkillIDs.IsValidIndex(SlotIndex) ? EquippedSkillIDs[SlotIndex] : 0;
		ULSSkillDataAsset* SkillData = (SkillID != 0 && SkillPool) ? SkillPool->FindSkillByID(SkillID) : nullptr;

		if (SkillData && SkillData->Icon)
		{
			SlotIcon->SetBrushFromTexture(SkillData->Icon);
			SlotIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			SlotIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ULSSkillLoadoutWidget::ClearSlotByIndex(const int32 SlotIndex)
{
	if (ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem())
	{
		SaveSubsystem->ClearEquippedSkillSlot(SlotIndex);
	}
}

ULSSaveSubsystem* ULSSkillLoadoutWidget::ResolveSaveSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
}

bool ULSSkillLoadoutWidget::IsSelectableSkillType(const int32 SkillID) const
{
	if (SkillID == 0)
	{
		return false;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	if (!GameDataSubsystem)
	{
		return false;
	}

	const FLSCharacterSkillRow* Row = GameDataSubsystem->FindActiveSkillRowByID(SkillID, TEXT("LSSkillLoadoutWidget"));
	if (!Row)
	{
		return false;
	}

	return Row->Skill_Type == ELSCharacterSkillType::Active || Row->Skill_Type == ELSCharacterSkillType::Ultimate;
}

UImage* ULSSkillLoadoutWidget::GetSlotIcon(const int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0: return Slot1Icon;
	case 1: return Slot2Icon;
	case 2: return Slot3Icon;
	default: return nullptr;
	}
}
