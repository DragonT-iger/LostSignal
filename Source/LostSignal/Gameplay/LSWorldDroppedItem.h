#pragma once

#include "CoreMinimal.h"
#include "Gameplay/LSInteractableObject.h"
#include "Session/LSSessionSubsystem.h"
#include "TimerManager.h"
#include "LSWorldDroppedItem.generated.h"

class UTexture2D;
class ULSMinimapMarkerComponent;
class ULSWorldDroppedItemIconWidget;
class UWidgetComponent;

UCLASS()
class LOSTSIGNAL_API ALSWorldDroppedItem : public ALSInteractableObject
{
	GENERATED_BODY()

public:
	ALSWorldDroppedItem();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool CanInteract_Implementation(APawn* Interactor) override;
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual FText GetInteractText_Implementation() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeDroppedItem(const FLSSessionItem& InItem, const FVector& InDropAnimationStartLocation);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Drop")
	TObjectPtr<UWidgetComponent> ItemIconWidgetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop", meta=(ClampMin="0.0"))
	float GroundOffsetZ = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	FVector2D IconDrawSize = FVector2D(64.0f, 64.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Drop|Animation", meta=(ClampMin="0.0"))
	float DropAnimationDurationSeconds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Drop|Animation", meta=(ClampMin="0.0"))
	float DropAnimationArcHeight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Minimap")
	TObjectPtr<ULSMinimapMarkerComponent> MinimapMarkerComponent;

private:
	UPROPERTY(ReplicatedUsing=OnRep_ItemData)
	FName ItemRowName;

	UPROPERTY(ReplicatedUsing=OnRep_ItemData)
	int32 Amount = 0;

	UPROPERTY(ReplicatedUsing=OnRep_ItemData)
	TArray<FLSChipResolvedStat> ChipStats;

	UPROPERTY(Replicated)
	FVector_NetQuantize10 DropAnimationStartLocation;

	UPROPERTY(ReplicatedUsing=OnRep_HasLanded)
	bool bHasLanded;

	UFUNCTION()
	void OnRep_ItemData();

	UFUNCTION()
	void OnRep_HasLanded();

	void RefreshItemVisual();
	void StartDropVisualAnimation();
	void UpdateDropVisualAnimation(float NormalizedTime);
	void FinishDropVisualAnimation();
	void CompleteDropLanding();
	UTexture2D* LoadIconTextureByRowName(FName InItemRowName) const;
	UTexture2D* LoadDefaultIconTexture() const;
	static FString BuildIconObjectPath(const FString& IconNameOrPath, const FString& BaseFolder);
	static FString GetIconBaseFolderByRowName(FName InItemRowName);

	bool bDropVisualAnimating;
	float DropVisualAnimationElapsedSeconds;
	FVector DropVisualLandedRelativeLocation;
	FTimerHandle DropLandingTimerHandle;
};
