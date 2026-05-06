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

	// 블루프린트에서 드랍 결과를 받아 처리 (아이템 박스 열림 애니메이션, 파티클 등)
	UFUNCTION(BlueprintImplementableEvent, Category="LS/Loot")
	void OnLootResultReceived(const TArray<FLSDropResult>& Results);

protected:
	// 프로젝트 설정 > LS Drop Settings의 RootingObjectTable Row 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/Loot")
	FName RootingObjectRowName;

private:
	UPROPERTY(Replicated)
	bool bIsOpened = false;
};
