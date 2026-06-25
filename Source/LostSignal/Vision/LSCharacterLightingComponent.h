// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "LSCharacterLightingComponent.generated.h"

class ADirectionalLight;
class UMaterialInstanceDynamic;
class UMeshComponent;

// Unlit 툰 캐릭터가 환경 그림자(실내/외부 건물 그림자)에 들어가면 전체 명암을 어둡게 적응시킨다.
// 캐릭터→태양 방향 라인트레이스로 그림자 여부를 판정해 0~1 light-level 스칼라를 머티리얼에 전달한다.
// 코스메틱 전용: 클라이언트 로컬에서 계산하며 리플리케이션하지 않는다.
UCLASS(ClassGroup = (Vision), meta = (BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSCharacterLightingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSCharacterLightingComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 직사광을 받을 때의 light-level
	UPROPERTY(EditDefaultsOnly, Category = "LS/Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SunlitLevel = 1.0f;

	// 그림자에 들어갔을 때의 light-level
	UPROPERTY(EditDefaultsOnly, Category = "LS/Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShadowedLevel = 0.35f;

	// light-level 보간 속도 (클수록 빠르게 전환)
	UPROPERTY(EditDefaultsOnly, Category = "LS/Lighting", meta = (ClampMin = "0.0"))
	float InterpSpeed = 6.0f;

	// 그림자 판정 트레이스 간격(초). 비용보다는 경계 깜빡임 방지·보간용.
	UPROPERTY(EditDefaultsOnly, Category = "LS/Lighting", meta = (ClampMin = "0.0"))
	float CheckInterval = 0.05f;

	// 캐릭터→태양 트레이스 길이(cm)
	UPROPERTY(EditDefaultsOnly, Category = "LS/Lighting", meta = (ClampMin = "0.0"))
	float TraceDistance = 5000.0f;

	// 발 위치에서 살짝 띄우는 높이(cm). 바닥 자기적중을 피하기 위한 작은 오프셋.
	UPROPERTY(EditDefaultsOnly, Category = "LS/Lighting")
	float TraceStartHeight = 10.0f;

	// 머티리얼이 읽는 스칼라 파라미터 이름
	UPROPERTY(EditDefaultsOnly, Category = "LS/Lighting")
	FName LightLevelParamName = TEXT("LS_LightLevel");

	// 그림자 판정에 사용할 콜리전 채널 (기본 Visibility — 지붕·벽이 Block하는 콜리전 전제)
	UPROPERTY(EditDefaultsOnly, Category = "LS/Lighting")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

private:
	UMeshComponent* ResolveSourceMeshComponent() const;
	void InitializeMaterialInstances();
	void ResolveSunLight();
	void UpdateShadowState();
	void ApplyLightLevel() const;

	// 메인 메시 슬롯마다 생성한 DMI들. 여기에 LS_LightLevel을 매 프레임 적용한다.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> CharacterMaterialInstances;

	TWeakObjectPtr<ADirectionalLight> SunLight;

	float CurrentLevel = 1.0f;
	float TargetLevel = 1.0f;
	float Accumulator = 0.0f;
};
