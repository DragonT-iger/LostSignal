#pragma once

#include "CoreMinimal.h"
#include "Gameplay/LSInteractableObject.h"
#include "Data/LSDropSubsystem.h"
#include "LSLootBox.generated.h"

UCLASS()
class LOSTSIGNAL_API ALSLootBox : public ALSInteractableObject
{
	GENERATED_BODY()

public:
	virtual bool CanInteract_Implementation(APawn* Interactor) override;
	virtual void Interact_Implementation(APawn* Interactor) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintImplementableEvent, Category="LS/Loot")
	void OnLootResultReceived(const TArray<FLSDropResult>& Results);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/Loot")
	FName RootingObjectRowName;

private:
	UPROPERTY(ReplicatedUsing=OnRep_IsOpened)
	bool bIsOpened = false;

	UFUNCTION()
	void OnRep_IsOpened();
};
