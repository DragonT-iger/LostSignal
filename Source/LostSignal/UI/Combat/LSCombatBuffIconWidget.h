#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Combat/LSCombatBuffTypes.h"

#include "LSCombatBuffIconWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;
class UMaterialInstanceDynamic;

UCLASS()
class LOSTSIGNAL_API ULSCombatBuffIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI|Combat")
	void SetBuffDisplay(const FLSCombatBuffDisplayData& InDisplayData);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<UTextBlock> StackText;

	// 버프 남은시간 방사형(시계방향 파이 와이프) 마스크. 아이콘과 같은 크기로 겹쳐 아이콘 위를 덮는다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<UImage> DurationMaskImage;

	// 마스크 머티리얼의 진행도(0~1) 스칼라 파라미터 이름.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Combat")
	FName BuffProgressParameterName = TEXT("Progress");

	// false(기본): 남은시간(남은/총)을 채움값으로 넣어 덮인 부채꼴이 줄어든다.
	// true: 진행도(1 - 남은/총)를 넣어 지속시간이 지날수록 부채꼴이 차오른다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Combat")
	bool bBuffFillByElapsed = false;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DurationMaskMaterial;
};
