#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSControlSettingsWidget.generated.h"

class UButton;
class UCheckBox;
class ULSControlSkillRowWidget;
class ULSSkillCastSettingsSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLSControlSettingsClosed);

// 컨트롤 설정 화면(WBP_ControlSettings)의 부모 클래스.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSControlSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(BlueprintAssignable, Category="LS/UI|Settings")
	FLSControlSettingsClosed OnClosed;

	void CloseControlSettings();

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<UButton> BackButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<UCheckBox> SmartKeyPreviewCheckBox;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<ULSControlSkillRowWidget> Skill1Row;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<ULSControlSkillRowWidget> Skill2Row;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Settings")
	TObjectPtr<ULSControlSkillRowWidget> Skill3Row;

private:
	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleSmartKeyPreviewCheckChanged(bool bIsChecked);

	void InitializeSkillRows();
	void InitializeCommonSettings();
	void BindWidgetEvents();
	void UnbindWidgetEvents();
	void LogMissingBindings() const;
	ULSSkillCastSettingsSubsystem* GetCastSettingsSubsystem() const;

	bool bRefreshingCommonCheckBox = false;
};
