#pragma once

#include "CoreMinimal.h"
#include "UI/Common/LSLayoutRevealWidget.h"
#include "LSConfirmDialogWidget.generated.h"

class UButton;
class URichTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLSConfirmDialogConfirmed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLSConfirmDialogCancelled);

// 공용 확인 다이얼로그. 메시지와 확인/취소 버튼을 두고, 클릭 시 결과를 브로드캐스트한 뒤 스스로 닫힌다.
// WBP_ConfirmDialog의 부모 클래스로 사용한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSConfirmDialogWidget : public ULSLayoutRevealWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ESC/TAB으로 취소(닫기). 다이얼로그가 키보드 포커스를 쥐고 있어, 뒤의 세팅 화면으로 ESC가 새지 않는다.
	// TAB은 Slate 기본 포커스 이동으로 새지 않게 여기서 소비한다.
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(BlueprintAssignable, Category="LS/UI|Common")
	FLSConfirmDialogConfirmed OnConfirmed;

	UPROPERTY(BlueprintAssignable, Category="LS/UI|Common")
	FLSConfirmDialogCancelled OnCancelled;

	// 외부 코드에서 취소와 동일하게 닫는다(OnCancelled 브로드캐스트 후 제거). 탭 전환 등으로 강제로 닫을 때 사용.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Common")
	void Cancel();

	// 다이얼로그 본문 메시지를 설정한다.
	// RichTextBlock이라 스타일 세트 행 이름 마크업으로 부분 강조가 가능하다. 예: "아직 <Emph>구현</>되지 않았습니다."
	UFUNCTION(BlueprintCallable, Category="LS/UI|Common")
	void SetMessage(const FText& InMessage) const;

protected:
	// 마크업 없는 일반 텍스트는 스타일 세트의 Default 행으로 렌더링된다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Common")
	TObjectPtr<URichTextBlock> MessageText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Common")
	TObjectPtr<UButton> ConfirmButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Common")
	TObjectPtr<UButton> CancelButton;

private:
	UFUNCTION()
	void HandleConfirmClicked();

	UFUNCTION()
	void HandleCancelClicked();
};
