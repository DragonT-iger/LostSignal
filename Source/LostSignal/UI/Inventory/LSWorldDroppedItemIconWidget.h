#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "LSWorldDroppedItemIconWidget.generated.h"

class SImage;
class UTexture2D;

UCLASS()
class LOSTSIGNAL_API ULSWorldDroppedItemIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	ULSWorldDroppedItemIconWidget(const FObjectInitializer& ObjectInitializer);

	void SetIconTexture(UTexture2D* InIconTexture);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;

private:
	FSlateBrush IconBrush;
	TSharedPtr<SImage> IconImage;
};
