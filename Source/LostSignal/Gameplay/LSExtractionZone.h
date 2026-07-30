#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LSExtractionZone.generated.h"

class UBoxComponent;
class ULSMinimapMarkerComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class LOSTSIGNAL_API ALSExtractionZone : public AActor
{
	GENERATED_BODY()

public:
	ALSExtractionZone();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	// 아래에서 위로 올라가며 퍼지는 링 개수(연기 상승 느낌 임시 표현).
	static constexpr int32 RisingRingCount = 0;

	UPROPERTY(VisibleAnywhere, Category="LS/Extraction")
	TObjectPtr<UBoxComponent> ExtractionBox;

	// 바닥에 깔리는 넓은 원반 — 트리거 범위(발판)를 위에서 봐도 직관적으로 보이게 한다.
	UPROPERTY(VisibleAnywhere, Category="LS/Extraction")
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

	//// 바닥에서 위로 솟아오르며 퍼지는 링들(연기/포탈 상승 느낌). Tick에서 위치·스케일을 순환 애니메이션.
	//UPROPERTY(VisibleAnywhere, Category="LS/Extraction")
	//TArray<TObjectPtr<UStaticMeshComponent>> RisingRings;

	UPROPERTY(VisibleAnywhere, Category="LS/Extraction")
	TObjectPtr<UTextRenderComponent> MarkerText;

	UPROPERTY(VisibleAnywhere, Category="LS/Extraction")
	TObjectPtr<UPointLightComponent> MarkerLight;

	// 실제 연기 VFX(아트가 WBP/BP에서 Niagara 에셋을 매핑). 미할당이면 위 임시 링 애니메이션만 보인다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Extraction")
	TObjectPtr<UNiagaraSystem> ExtractionSmokeEffect;

	UPROPERTY(VisibleAnywhere, Category="LS/Extraction")
	TObjectPtr<UNiagaraComponent> SmokeEffectComponent;

	UPROPERTY(VisibleAnywhere, Category="LS/Minimap")
	TObjectPtr<ULSMinimapMarkerComponent> MinimapMarkerComponent;

	// 마커 애니메이션 누적 시간(링 상승·라이트 펄스 구동).
	float MarkerAnimTime = 0.f;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
};
