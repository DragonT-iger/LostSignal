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

	void InitializeDroppedItem(const FLSSessionItem& InItem, const FVector& InDropAnimationStartLocation, float InDropAnimationDurationScale = 1.0f);

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

	// 포물선 연출이 진행 중이면 근접 폰이 없어도 Tick을 유지한다(달리며 버려 상호작용 범위를 벗어나도 연출이 끊기지 않게).
	virtual bool ShouldKeepTickEnabled() const override;

private:
	UPROPERTY(ReplicatedUsing=OnRep_ItemData)
	FName ItemRowName;

	UPROPERTY(ReplicatedUsing=OnRep_ItemData)
	int32 Amount = 0;

	UPROPERTY(ReplicatedUsing=OnRep_ItemData)
	TArray<FLSChipResolvedStat> ChipStats;

	UPROPERTY(Replicated)
	FVector_NetQuantize10 DropAnimationStartLocation;

	UPROPERTY(Replicated)
	float DropAnimationDurationScale;

	UPROPERTY(ReplicatedUsing=OnRep_HasLanded)
	bool bHasLanded;

	UFUNCTION()
	void OnRep_ItemData();

	UFUNCTION()
	void OnRep_HasLanded();

	void RefreshItemVisual();
	float GetDropVisualAnimationDurationSeconds() const;
	void StartDropVisualAnimation();
	void UpdateDropVisualAnimation(float NormalizedTime);
	void ApplyDropLandedVisualState();
	void CompressRemainingDropVisualAnimation();
	void FinishDropVisualAnimation();
	void CompleteDropLanding();
	UTexture2D* LoadIconTextureByRowName(FName InItemRowName) const;
	UTexture2D* LoadDefaultIconTexture() const;
	static FString BuildIconObjectPath(const FString& IconNameOrPath, const FString& BaseFolder);
	static FString GetIconBaseFolderByRowName(FName InItemRowName);

	bool bDropVisualAnimating;
	float DropVisualAnimationElapsedSeconds;
	// 연출 재생 배속. 착지 확정(bHasLanded)이 연출보다 먼저 도착했을 때 남은 구간을 빠르게 이어 붙이는 데만 쓴다(1.0 = 정상).
	float DropVisualPlayRateMultiplier;
	FVector DropVisualLandedRelativeLocation;
	FTimerHandle DropLandingTimerHandle;
};
