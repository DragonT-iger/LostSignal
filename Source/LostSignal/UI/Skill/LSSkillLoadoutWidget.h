#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSSkillLoadoutWidget.generated.h"

class UButton;
class UImage;
class UPanelWidget;
class ULSSkillDataAsset;
class ULSSkillLoadoutEntryWidget;
class ULSSkillPoolDataAsset;
class ULSSaveSubsystem;

// 로비 개인정비 스킬 탭의 콘텐츠. 캐릭터 스킬 풀에서 액티브/궁극기 후보를 나열하고,
// 슬롯 선택 후 후보 클릭으로 3칸(Skill1~3)에 장착한다. 데이터는 ULSSaveSubsystem의 캐릭터별 스킬 로드아웃(SkillPool->CharacterID 키)이 단일 출처.
UCLASS()
class LOSTSIGNAL_API ULSSkillLoadoutWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 후보 리스트와 장착 슬롯 표시를 세이브 기준으로 다시 그린다. 탭을 열 때/변경될 때 호출.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Skill")
	void RefreshSkillLoadout();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 후보 스킬 엔트리를 담을 컨테이너(WrapBox/VerticalBox 등). 아트가 WBP에 배치.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UPanelWidget> CandidateContainer;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UButton> Slot1Button;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UButton> Slot2Button;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UButton> Slot3Button;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UImage> Slot1Icon;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UImage> Slot2Icon;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UImage> Slot3Icon;

	// 현재 선택/변경 중인 슬롯의 스킬 정보를 표시하는 전용 엔트리. WBP에 WBP_SkillLoadoutEntry 인스턴스를 배치한다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<ULSSkillLoadoutEntryWidget> SelectedSlotEntry;

	// 후보 엔트리 위젯 클래스. BP에서 WBP_SkillLoadoutEntry를 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Skill")
	TSubclassOf<ULSSkillLoadoutEntryWidget> CandidateEntryClass;

	// 이 캐릭터의 선택 가능 스킬 풀. BP에서 DA_*_SkillPool을 매핑한다(런타임 SkillComponent와 같은 자산).
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Skill")
	TObjectPtr<ULSSkillPoolDataAsset> SkillPool;

private:
	UFUNCTION()
	void HandleSlot1Clicked();

	UFUNCTION()
	void HandleSlot2Clicked();

	UFUNCTION()
	void HandleSlot3Clicked();

	// 후보 클릭: 현재 선택 슬롯에 장착한다. 같은 스킬이 다른 슬롯에 있으면 저장 계층에서 기존 슬롯을 비운다.
	void HandleEntryClicked(int32 SkillID);

	void HandleSkillLoadoutChanged();
	void RebuildCandidateList();
	void RefreshEquippedSlots();
	void RefreshSelectedSlotEntry();
	void SelectSlotByIndex(int32 SlotIndex);

	ULSSaveSubsystem* ResolveSaveSubsystem() const;
	// Skill_ID가 액티브/궁극기 타입이면 true(선택 가능). DataTable(Skill_Type)이 단일 출처.
	bool IsSelectableSkillType(int32 SkillID) const;
	UImage* GetSlotIcon(int32 SlotIndex) const;
	ULSSkillDataAsset* ResolveSkillDataForSlot(int32 SlotIndex, int32& OutSkillID) const;

	FDelegateHandle SkillLoadoutChangedHandle;
	int32 SelectedSlotIndex = 0;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSSkillLoadoutEntryWidget>> CandidateEntries;
};
