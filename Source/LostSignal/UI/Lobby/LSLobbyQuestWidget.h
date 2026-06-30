#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "LSLobbyQuestWidget.generated.h"

class UButton;
class ULSQuestInfoWidget;

// 로비 퀘스트 패널(WBP_LobbyQuest)의 부모 클래스. 메인 퀘스트 1개 + 서브 퀘스트 3개의
// QuestInfo(WBP_QuestInfo) 위젯을 묶는다. 서브 인덱스는 0~2.
// 토글 버튼 2개: 하나는 메인 퀘스트, 하나는 서브 퀘스트 묶음을 Collapsed/Visible로 접고 편다.
// 각 버튼의 스타일 이미지는 열림/닫힘 상태에 따라 교체한다(열림→CloseImageBrush, 닫힘→OpenImageBrush).
// 데이터 소스 연결은 추후 작업하고, 현재는 setter만 열어 둔다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSLobbyQuestWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	static constexpr int32 SubQuestCount = 3;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 메인 퀘스트 이름/목표를 설정한다.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Lobby")
	void SetMainQuest(const FText& NewName, const FText& NewObjective) const;

	// 서브 퀘스트(0~2) 이름/목표를 설정한다. 범위를 벗어나면 무시한다.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Lobby")
	void SetSubQuest(int32 SubIndex, const FText& NewName, const FText& NewObjective) const;

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSQuestInfoWidget> MainQuest;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSQuestInfoWidget> SubQuest1;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSQuestInfoWidget> SubQuest2;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<ULSQuestInfoWidget> SubQuest3;

	// 메인/서브 퀘스트 접기·펴기 토글 버튼.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UButton> MainToggleButton;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UButton> SubToggleButton;

	// 닫힘 상태에서 보여줄 "열기" 아이콘. BP에서 지정.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	FSlateBrush OpenImageBrush;

	// 열림 상태에서 보여줄 "닫기" 아이콘. BP에서 지정.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Lobby")
	FSlateBrush CloseImageBrush;

private:
	UFUNCTION()
	void HandleMainToggleClicked();

	UFUNCTION()
	void HandleSubToggleClicked();

	// 현재 상태대로 메인 퀘스트 표시와 아이콘 브러시를 갱신한다.
	void RefreshMainQuestState() const;

	// 현재 상태대로 서브 퀘스트 3개 표시와 아이콘 브러시를 갱신한다.
	void RefreshSubQuestState() const;

	// 열림 여부에 맞는 아이콘 브러시를 버튼 스타일에 적용한다(열림→닫기 아이콘, 닫힘→열기 아이콘).
	void ApplyToggleImage(UButton* ToggleButton, bool bOpen) const;

	// 서브 인덱스(0~2)를 바인딩된 서브 QuestInfo로 변환한다. 범위를 벗어나면 nullptr.
	ULSQuestInfoWidget* GetSubQuest(int32 SubIndex) const;

	// 메인/서브 퀘스트가 펼쳐져 있는지. 기본은 펼침.
	bool bMainQuestOpen = true;
	bool bSubQuestOpen = true;
};
