#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "LSInventoryItemSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSInventoryItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void SetItem(FName ItemRowName, int32 Amount);

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void ClearItem();

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UImage> ItemIconImage;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UTextBlock> AmountText;

private:
	UTexture2D* LoadIconTextureByRowName(FName ItemRowName) const;
	static FString BuildIconObjectPath(const FString& IconNameOrPath, const FString& BaseFolder);
	static FString GetIconBaseFolderByRowName(FName ItemRowName);
};
