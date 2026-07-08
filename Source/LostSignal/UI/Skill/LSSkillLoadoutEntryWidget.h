#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSSkillLoadoutEntryWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class ULSSkillDataAsset;
struct FLSCharacterSkillRow;

// 후보 클릭 시 부모(ULSSkillLoadoutWidget)에 Skill_ID를 전달한다. C++ 전용 바인딩.
DECLARE_DELEGATE_OneParam(FLSOnSkillLoadoutEntryClicked, int32 /*SkillID*/);

// 로비 스킬 로드아웃 후보 리스트의 개별 엔트리. 스킬 아이콘/이름을 표시하고 클릭하면 장착을 요청한다.
UCLASS()
class LOSTSIGNAL_API ULSSkillLoadoutEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 표시할 후보 스킬 데이터를 지정한다.
	void SetSkillData(ULSSkillDataAsset* InSkillData);

	void SetDisplayOnly(bool bInDisplayOnly);
	void SetNamePrefix(const FText& InNamePrefix);
	void SetEmptyDisplayText(const FText& InNameText, const FText& InDescriptionText);

	ULSSkillDataAsset* GetSkillData() const { return SkillData; }

	FLSOnSkillLoadoutEntryClicked OnEntryClicked;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UButton> SelectButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UImage> IconImage;

	// 스킬 이름. DataTable row의 Skill_Name(없으면 DataAsset DisplayName)을 표시.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UTextBlock> NameText;

	// 스킬 설명. DataTable row의 Skill_Info(없으면 DataAsset Description)를 표시.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Skill")
	TObjectPtr<UTextBlock> DescriptionText;

private:
	UFUNCTION()
	void HandleSelectButtonClicked();

	void RefreshDisplay();
	void RefreshButtonState();
	void RefreshIcon();
	const FLSCharacterSkillRow* ResolveSkillRow() const;
	void RefreshName(const FLSCharacterSkillRow* Row);
	void RefreshDescription(const FLSCharacterSkillRow* Row);

	UPROPERTY(Transient)
	TObjectPtr<ULSSkillDataAsset> SkillData;

	FText NamePrefixText;
	FText EmptyNameText;
	FText EmptyDescriptionText;

	bool bDisplayOnly = false;
};
