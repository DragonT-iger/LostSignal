#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSQuestInfoWidget.generated.h"

class UTextBlock;

// 퀘스트 한 줄(WBP_QuestInfo)의 부모 클래스. 퀘스트 이름과 목표 텍스트 2개를 묶는다.
// 로비 퀘스트 위젯(WBP_LobbyQuest)에서 메인 1개 + 서브 3개로 배치한다.
// 데이터 소스 연결은 추후 작업하고, 현재는 setter만 열어 둔다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSQuestInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// 퀘스트 이름과 목표를 한 번에 설정한다.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Lobby")
	void SetQuestInfo(const FText& NewName, const FText& NewObjective) const;

	UFUNCTION(BlueprintCallable, Category="LS/UI|Lobby")
	void SetQuestName(const FText& NewName) const;

	UFUNCTION(BlueprintCallable, Category="LS/UI|Lobby")
	void SetQuestObjective(const FText& NewObjective) const;

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UTextBlock> QuestNameText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Lobby")
	TObjectPtr<UTextBlock> QuestObjectiveText;
};
