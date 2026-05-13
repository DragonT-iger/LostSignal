#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/LSInteractable.h"
#include "LSInteractableObject.generated.h"

class USphereComponent;
class UWidgetComponent;
class ULSInteractHintWidget;

UCLASS(Abstract, BlueprintType)
class LOSTSIGNAL_API ALSInteractableObject : public AActor, public ILSInteractable
{
	GENERATED_BODY()

public:
	ALSInteractableObject();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual bool CanInteract_Implementation(APawn* Interactor) override;
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual FText GetInteractText_Implementation() override;

	// 현재 범위 내 로컬 폰 기준으로 위젯 표시 여부를 갱신
	void RefreshWidgetVisibility();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Interact")
	TObjectPtr<USphereComponent> InteractionSphere;

	// 상호작용 힌트 UI (블루프린트에서 WidgetClass 설정)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Interact")
	TObjectPtr<UWidgetComponent> InteractWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact")
	FText InteractText;

private:
	TWeakObjectPtr<APawn> FocusedLocalPawn;

	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	APawn* FindOverlappingLocalPawn() const;
	void UpdateHintWidget(APawn* Pawn);
};
