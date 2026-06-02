#pragma once

#include "CoreMinimal.h"
#include "Gameplay/LSInteractableObject.h"
#include "Session/LSSessionSubsystem.h"
#include "LSWorldDroppedItem.generated.h"

class UTexture2D;
class ULSWorldDroppedItemIconWidget;
class UWidgetComponent;

UCLASS()
class LOSTSIGNAL_API ALSWorldDroppedItem : public ALSInteractableObject
{
	GENERATED_BODY()

public:
	ALSWorldDroppedItem();

	virtual void BeginPlay() override;
	virtual bool CanInteract_Implementation(APawn* Interactor) override;
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual FText GetInteractText_Implementation() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeDroppedItem(const FLSSessionItem& InItem);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Drop")
	TObjectPtr<UWidgetComponent> ItemIconWidgetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop", meta=(ClampMin="0.0"))
	float GroundOffsetZ = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	FVector2D IconDrawSize = FVector2D(64.0f, 64.0f);

private:
	UPROPERTY(ReplicatedUsing=OnRep_ItemData)
	FName ItemRowName;

	UPROPERTY(ReplicatedUsing=OnRep_ItemData)
	int32 Amount = 0;

	UPROPERTY(ReplicatedUsing=OnRep_ItemData)
	TArray<FLSChipResolvedStat> ChipStats;

	UFUNCTION()
	void OnRep_ItemData();

	void RefreshItemVisual();
	UTexture2D* LoadIconTextureByRowName(FName InItemRowName) const;
	UTexture2D* LoadDefaultIconTexture() const;
	static FString BuildIconObjectPath(const FString& IconNameOrPath, const FString& BaseFolder);
	static FString GetIconBaseFolderByRowName(FName InItemRowName);
};
