#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/LSInteractable.h"
#include "LSRatStealCabinet.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * 본편 월드에 배치하는 RatSteal 진입 오브젝트 (31_Flow_EntryReturn).
 * 본편 IA_Interact로 상호작용 → 복귀 정보 저장 후 미니게임 레벨로 전환.
 */
UCLASS()
class LOSTSIGNAL_API ALSRatStealCabinet : public AActor, public ILSInteractable
{
	GENERATED_BODY()

public:
	ALSRatStealCabinet();

	virtual bool CanInteract_Implementation(APawn* Interactor) override;
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual FText GetInteractText_Implementation() override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<UStaticMeshComponent> CabinetMesh;

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<UBoxComponent> InteractBox;

	/** 미니게임 레벨 (MG_RatSteal 또는 MG_RatSteal_Tutorial) */
	UPROPERTY(EditAnywhere, Category = "LS/RatSteal")
	TSoftObjectPtr<UWorld> MiniGameLevel;

	UPROPERTY(EditAnywhere, Category = "LS/RatSteal")
	FText InteractText;
};
