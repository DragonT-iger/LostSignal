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
constexpr int32 SkillLoadoutSlotCount = 4;
}

#define LOCTEXT_NAMESPACE "LSSkillLoadoutWidget"

void ULSSkillLoadoutWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Slot1Button) { Slot1Button->OnClicked.AddDynamic(this, &ULSSkillLoadoutWidget::HandleSlot1Clicked); }
	if (Slot2Button) { Slot2Button->OnClicked.AddDynamic(this, &ULSSkillLoadoutWidget::HandleSlot2Clicked); }
	if (Slot3Button) { Slot3Button->OnClicked.AddDynamic(this, &ULSSkillLoadoutWidget::HandleSlot3Clicked); }
	if (Slot4Button) { Slot4Button->OnClicked.AddDynamic(this, &ULSSkillLoadoutWidget::HandleSlot4Clicked); }

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
	if (!SelectedSlotEntry)
	{
		UE_LOG(LogLS, Warning, TEXT("SelectedSlotEntry is not bound on %s."), *GetNameSafe(this));
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
	if (Slot4Button) { Slot4Button->OnClicked.RemoveDynamic(this, &ULSSkillLoadoutWidget::HandleSlot4Clicked); }

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
	// 최초 진입 시 세이브가 비어 있으면 풀의 기본 로드아웃을 1회 시딩한다.
	// (시딩되면 OnSkillLoadoutChanged로 재진입하지만 세이브 플래그로 1회만 실행되고, 아래에서 갱신된 상태를 그린다.)
	if (SkillPool)
	{
		if (ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem())
		{
			SaveSubsystem->TrySeedDefaultSkillLoadout(SkillPool->CharacterID, SkillPool->DefaultEquippedSkillIDs);
		}
	}

	RebuildCandidateList();
	RefreshEquippedSlots();
	RefreshSelectedSlotEntry();
}

void ULSSkillLoadoutWidget::HandleSlot1Clicked() { SelectSlotByIndex(0); }
void ULSSkillLoadoutWidget::HandleSlot2Clicked() { SelectSlotByIndex(1); }
void ULSSkillLoadoutWidget::HandleSlot3Clicked() { SelectSlotByIndex(2); }
void ULSSkillLoadoutWidget::HandleSlot4Clicked() { SelectSlotByIndex(3); }

void ULSSkillLoadoutWidget::HandleEntryClicked(const int32 SkillID)
{
	ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem();
	if (!SaveSubsystem || !SkillPool || SkillID == 0)
	{
		return;
	}

	const int32 CharacterID = SkillPool->CharacterID;
	SaveSubsystem->SetEquippedSkillSlot(CharacterID, SelectedSlotIndex, SkillID);
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
		Entry->OnEntryClicked.BindUObject(this, &ULSSkillLoadoutWidget::HandleEntryClicked);

		CandidateContainer->AddChild(Entry);
		CandidateEntries.Add(Entry);
	}
}

void ULSSkillLoadoutWidget::RefreshEquippedSlots()
{
	const int32 CharacterID = SkillPool ? SkillPool->CharacterID : 0;

	for (int32 SlotIndex = 0; SlotIndex < SkillLoadoutSlotCount; ++SlotIndex)
	{
		UImage* SlotIcon = GetSlotIcon(SlotIndex);
		if (!SlotIcon)
		{
			continue;
		}

		int32 SkillID = 0;
		ULSSkillDataAsset* SkillData = ResolveSkillDataForSlot(SlotIndex, SkillID);

		// 세이브엔 Skill_ID가 있는데 풀에서 못 찾으면(SelectableSkills 누락) 아이콘이 비어 보인다 — 원인 로그.
		if (SkillID != 0 && !SkillData)
		{
			UE_LOG(LogLS, Warning, TEXT("[SkillLoadout] Slot %d의 Skill_ID %d를 SkillPool(char %d)에서 못 찾음. SelectableSkills에 등록됐는지 확인하세요."),
				SlotIndex, SkillID, CharacterID);
		}

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

void ULSSkillLoadoutWidget::RefreshSelectedSlotEntry()
{
	if (!SelectedSlotEntry)
	{
		return;
	}

	int32 SkillID = 0;
	ULSSkillDataAsset* SkillData = ResolveSkillDataForSlot(SelectedSlotIndex, SkillID);
	const FText SlotLabel = FText::Format(
		LOCTEXT("SelectedSlotPrefix", "스킬 슬롯 {0}"),
		FText::AsNumber(SelectedSlotIndex + 1));

	SelectedSlotEntry->SetDisplayOnly(true);
	SelectedSlotEntry->SetNamePrefix(SlotLabel);
	if (SkillData)
	{
		SelectedSlotEntry->SetSkillData(SkillData);
	}
	else
	{
		SelectedSlotEntry->SetEmptyDisplayText(
			FText::GetEmpty(),
			LOCTEXT("SelectedSlotEmptyDescription", "장착된 스킬이 없습니다."));
	}
}

void ULSSkillLoadoutWidget::SelectSlotByIndex(const int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= SkillLoadoutSlotCount)
	{
		return;
	}

	SelectedSlotIndex = SlotIndex;
	RefreshSelectedSlotEntry();
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
	case 3: return Slot4Icon;
	default: return nullptr;
	}
}

ULSSkillDataAsset* ULSSkillLoadoutWidget::ResolveSkillDataForSlot(const int32 SlotIndex, int32& OutSkillID) const
{
	OutSkillID = 0;
	if (!SkillPool)
	{
		return nullptr;
	}

	const ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem();
	const TArray<int32> EquippedSkillIDs = SaveSubsystem ? SaveSubsystem->GetEquippedSkillIDs(SkillPool->CharacterID) : TArray<int32>();
	OutSkillID = EquippedSkillIDs.IsValidIndex(SlotIndex) ? EquippedSkillIDs[SlotIndex] : 0;
	return OutSkillID != 0 ? SkillPool->FindSkillByID(OutSkillID) : nullptr;
}

#undef LOCTEXT_NAMESPACE
