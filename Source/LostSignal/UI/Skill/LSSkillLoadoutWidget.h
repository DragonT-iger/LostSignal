#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSSkillLoadoutWidget.generated.h"

class UButton;
class UImage;
class UPanelWidget;
class ULSSkillLoadoutEntryWidget;
class ULSSkillPoolDataAsset;
class ULSSaveSubsystem;

// 로비 개인정비 스킬 탭의 콘텐츠. 캐릭터 스킬 풀에서 액티브/궁극기 후보를 나열하고,
// 클릭 배치로 3칸(Skill1~3)에 장착/해제한다. 데이터는 ULSSaveSubsystem의 EquippedSkillIDs가 단일 출처.
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

	// 후보 클릭: 첫 빈 슬롯에 장착한다. 빈 슬롯이 없으면 무시하고 경고를 남긴다.
	void HandleEntryClicked(int32 SkillID);

	void HandleSkillLoadoutChanged();
	void RebuildCandidateList();
	void RefreshEquippedSlots();
	void ClearSlotByIndex(int32 SlotIndex);

	ULSSaveSubsystem* ResolveSaveSubsystem() const;
	// Skill_ID가 액티브/궁극기 타입이면 true(선택 가능). DataTable(Skill_Type)이 단일 출처.
	bool IsSelectableSkillType(int32 SkillID) const;
	UImage* GetSlotIcon(int32 SlotIndex) const;

	FDelegateHandle SkillLoadoutChangedHandle;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSSkillLoadoutEntryWidget>> CandidateEntries;
};
