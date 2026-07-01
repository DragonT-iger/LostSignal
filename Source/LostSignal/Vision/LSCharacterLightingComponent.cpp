// Copyright Epic Games, Inc. All Rights Reserved.

#include "Vision/LSCharacterLightingComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "LostSignal.h"
#include "Materials/MaterialInstanceDynamic.h"

ULSCharacterLightingComponent::ULSCharacterLightingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void ULSCharacterLightingComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentLevel = SunlitLevel;
	TargetLevel = SunlitLevel;

	InitializeMaterialInstances();
	ResolveSunLight();
}

// 그림자 판정과 light-level 보간은 코스메틱이므로 데디케이티드 서버에서는 돌리지 않는다.
void ULSCharacterLightingComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetNetMode() == NM_DedicatedServer || CharacterMaterialInstances.Num() == 0)
	{
		return;
	}

	Accumulator += DeltaTime;
	if (Accumulator >= CheckInterval)
	{
		Accumulator = 0.0f;
		UpdateShadowState();
	}

	CurrentLevel = FMath::FInterpTo(CurrentLevel, TargetLevel, DeltaTime, InterpSpeed);
	ApplyLightLevel();
}

UMeshComponent* ULSCharacterLightingComponent::ResolveSourceMeshComponent() const
{
	// 캐릭터의 메인 스켈레탈 메시를 우선 사용한다(XRay 오버레이·무기 메시를 잘못 잡지 않도록).
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		return OwnerCharacter->GetMesh();
	}

	return GetOwner() != nullptr ? GetOwner()->FindComponentByClass<UMeshComponent>() : nullptr;
}

// 메인 메시의 각 머티리얼 슬롯을 DMI로 교체해 LS_LightLevel을 런타임에 먹일 수 있게 한다.
void ULSCharacterLightingComponent::InitializeMaterialInstances()
{
	UMeshComponent* MeshComponent = ResolveSourceMeshComponent();
	if (MeshComponent == nullptr)
	{
		UE_LOG(LogLS, Warning, TEXT("LSCharacterLightingComponent on '%s' could not find a mesh component."), *GetNameSafe(GetOwner()));
		return;
	}

	CharacterMaterialInstances.Reset();

	const int32 MaterialCount = MeshComponent->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		if (UMaterialInstanceDynamic* MID = MeshComponent->CreateAndSetMaterialInstanceDynamic(MaterialIndex))
		{
			CharacterMaterialInstances.Add(MID);
		}
	}
}

// 렌더링에 영향을 주는 디렉셔널 라이트 중 ForwardShadingPriority가 가장 높은 라이트를 태양으로 채택한다.
void ULSCharacterLightingComponent::ResolveSunLight()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	ADirectionalLight* BestLight = nullptr;
	int32 BestPriority = INDEX_NONE;
	float BestBrightnessSquared = -1.0f;

	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		ADirectionalLight* CandidateLight = *It;
		UDirectionalLightComponent* LightComponent = CandidateLight != nullptr
			? Cast<UDirectionalLightComponent>(CandidateLight->GetLightComponent())
			: nullptr;
		if (LightComponent == nullptr || !LightComponent->bAffectsWorld || !LightComponent->IsVisible())
		{
			continue;
		}

		const int32 Priority = LightComponent->ForwardShadingPriority;
		const FLinearColor LightColor = LightComponent->GetLightColor() * LightComponent->Intensity;
		const float BrightnessSquared =
			LightColor.R * LightColor.R + LightColor.G * LightColor.G + LightColor.B * LightColor.B;

		if (Priority > BestPriority || (Priority == BestPriority && BrightnessSquared > BestBrightnessSquared))
		{
			BestLight = CandidateLight;
			BestPriority = Priority;
			BestBrightnessSquared = BrightnessSquared;
		}
	}

	SunLight = BestLight;

	if (!SunLight.IsValid())
	{
		UE_LOG(LogLS, Warning, TEXT("LSCharacterLightingComponent on '%s': 레벨에 디렉셔널 라이트가 없어 명암 적응을 비활성한다."), *GetNameSafe(GetOwner()));
	}
}

// 캐릭터→태양 방향으로 트레이스해 막히면(실내/외부 그림자) TargetLevel을 어둡게 잡는다.
void ULSCharacterLightingComponent::UpdateShadowState()
{
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (Owner == nullptr || World == nullptr || !SunLight.IsValid())
	{
		TargetLevel = SunlitLevel;
		return;
	}

	// 디렉셔널 라이트의 전방 벡터 = 빛이 진행하는 방향. 태양 쪽은 그 반대.
	const FVector ToSun = -SunLight->GetActorForwardVector();

	// 캐릭터가 실제로 서 있는 발 위치를 기준으로 한다(캡슐 절반 높이만큼 아래).
	float FootZOffset = 0.0f;
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
	{
		if (const UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent())
		{
			FootZOffset = Capsule->GetScaledCapsuleHalfHeight();
		}
	}

	const FVector FootLocation = Owner->GetActorLocation() - FVector(0.0f, 0.0f, FootZOffset);
	const FVector Start = FootLocation + FVector(0.0f, 0.0f, TraceStartHeight);
	const FVector End = Start + ToSun * TraceDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(LSCharacterLighting), false, Owner);

	FHitResult Hit;
	const bool bBlocked = World->LineTraceSingleByChannel(Hit, Start, End, TraceChannel.GetValue(), Params);
	TargetLevel = bBlocked ? ShadowedLevel : SunlitLevel;
}

void ULSCharacterLightingComponent::ApplyLightLevel() const
{
	for (UMaterialInstanceDynamic* MID : CharacterMaterialInstances)
	{
		if (MID != nullptr)
		{
			MID->SetScalarParameterValue(LightLevelParamName, CurrentLevel);
		}
	}
}
