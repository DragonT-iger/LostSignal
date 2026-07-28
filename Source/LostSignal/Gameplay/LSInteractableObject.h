#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/LSInteractable.h"
#include "LSInteractableObject.generated.h"

class USphereComponent;
class UWidgetComponent;
class UMeshComponent;
class ULSInteractHintWidget;
class ULSDistanceMarkerComponent;

UCLASS(Abstract, BlueprintType)
class LOSTSIGNAL_API ALSInteractableObject : public AActor, public ILSInteractable
{
	GENERATED_BODY()

public:
	ALSInteractableObject();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual bool CanInteract_Implementation(APawn* Interactor) override;
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual FText GetInteractText_Implementation() override;

	// 현재 범위 내 로컬 폰 기준으로 위젯 표시 여부를 갱신
	void RefreshWidgetVisibility();

	// 근접 아웃라인을 런타임에 켜고 끈다. 끄면 즉시 반영(예: 루팅 완료 후 하이라이트 해제).
	UFUNCTION(BlueprintCallable, Category="LS/Interact|Outline")
	void SetProximityOutlineEnabled(bool bEnabled);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Interact")
	TObjectPtr<USphereComponent> InteractionSphere;

	// 상호작용 힌트 UI (블루프린트에서 WidgetClass 설정)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Interact")
	TObjectPtr<UWidgetComponent> InteractWidget;

	// 거리 기반 빌보드 마커(원 UI 등). MarkerWidgetClass를 BP에서 지정한 오브젝트만 표시된다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Interact")
	TObjectPtr<ULSDistanceMarkerComponent> DistanceMarkerComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact")
	FText InteractText;

	// 근접 시 스텐실 아웃라인(PP_Outline)을 켤지. 자체 마커가 있는 오브젝트는 BP/생성자에서 false로 끈다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Outline")
	bool bEnableProximityOutline = true;

	// 아웃라인용 CustomStencil 값. PP_Outline의 StencilTarget과 일치시켜야 한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Outline", meta=(ClampMin="0", ClampMax="255"))
	int32 OutlineStencilValue = 20;

	virtual void HandleLocalPawnEndOverlap(APawn* Pawn);

private:
	TWeakObjectPtr<APawn> FocusedLocalPawn;
	bool bLoggedMissingInteractWidget = false;
	bool bLoggedInvalidInteractWidget = false;

	// BeginPlay에서 수집한 아웃라인 대상 메시(위젯·기존 커스텀뎁스 점유 메시 제외). 순수 로컬 렌더용.
	TArray<TWeakObjectPtr<UMeshComponent>> OutlineMeshes;
	bool bOutlineActive = false;

	void GatherOutlineMeshes();
	void ApplyOutlineState(bool bWantOutline);

	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	APawn* FindOverlappingLocalPawn() const;
	void UpdateHintWidget(APawn* Pawn);
};
