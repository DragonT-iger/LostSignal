#include "Vision/LSPlayerXRayComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Components/SkeletalMeshComponent.h"

ULSPlayerXRayComponent::ULSPlayerXRayComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

// Creates the overlay mesh that will render the XRay material on top of the main player mesh.
void ULSPlayerXRayComponent::BeginPlay()
{
	Super::BeginPlay();

	CreateOverlayMeshComponent();
	UpdateOverlayMaterialParameters();
	RefreshOverlayVisibility();
}

// Cleans up the runtime-created overlay mesh when the owning actor is torn down.
void ULSPlayerXRayComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OverlayMeshComponent != nullptr)
	{
		OverlayMeshComponent->DestroyComponent();
		OverlayMeshComponent = nullptr;
	}

	OverlayMaterialInstances.Reset();

	Super::EndPlay(EndPlayReason);
}

// Keeps XRay visibility in sync with local possession state and pushes any updated material parameters.
void ULSPlayerXRayComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RefreshOverlayVisibility();
	UpdateOverlayMaterialParameters();
}

USkeletalMeshComponent* ULSPlayerXRayComponent::ResolveSourceMeshComponent() const
{
	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		return OwnerPawn->FindComponentByClass<USkeletalMeshComponent>();
	}

	return GetOwner() != nullptr ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

// Creates a hidden runtime mesh that follows the main skeletal mesh pose and uses the XRay overlay material.
void ULSPlayerXRayComponent::CreateOverlayMeshComponent()
{
	if (OverlayMeshComponent != nullptr)
	{
		return;
	}

	USkeletalMeshComponent* SourceMeshComponent = ResolveSourceMeshComponent();
	if (SourceMeshComponent == nullptr)
	{
		UE_LOG(LogLS, Warning, TEXT("LSPlayerXRayComponent on '%s' could not find a source skeletal mesh."), *GetNameSafe(GetOwner()));
		return;
	}

	OverlayMeshComponent = NewObject<USkeletalMeshComponent>(
		GetOwner(),
		USkeletalMeshComponent::StaticClass(),
		TEXT("XRayOverlayMesh"),
		RF_Transient);

	if (OverlayMeshComponent == nullptr)
	{
		UE_LOG(LogLS, Warning, TEXT("LSPlayerXRayComponent on '%s' failed to allocate overlay skeletal mesh."), *GetNameSafe(GetOwner()));
		return;
	}

	GetOwner()->AddInstanceComponent(OverlayMeshComponent);
	OverlayMeshComponent->SetupAttachment(SourceMeshComponent);
	OverlayMeshComponent->SetSkeletalMeshAsset(SourceMeshComponent->GetSkeletalMeshAsset());
	OverlayMeshComponent->SetLeaderPoseComponent(SourceMeshComponent);
	OverlayMeshComponent->SetRelativeTransform(FTransform::Identity);
	OverlayMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OverlayMeshComponent->SetGenerateOverlapEvents(false);
	OverlayMeshComponent->SetCanEverAffectNavigation(false);
	OverlayMeshComponent->SetCastShadow(false);
	OverlayMeshComponent->bCastDynamicShadow = false;
	OverlayMeshComponent->bCastStaticShadow = false;
	OverlayMeshComponent->bReceivesDecals = false;
	OverlayMeshComponent->SetHiddenInGame(true);
	OverlayMeshComponent->SetVisibility(false, true);
	OverlayMeshComponent->RegisterComponent();

	OverlayMaterialInstances.Reset();

	if (OverlayMaterial == nullptr)
	{
		return;
	}

	const int32 MaterialCount = FMath::Max(SourceMeshComponent->GetNumMaterials(), 1);
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		UMaterialInstanceDynamic* OverlayMID = UMaterialInstanceDynamic::Create(OverlayMaterial, this);
		if (OverlayMID == nullptr)
		{
			continue;
		}

		OverlayMaterialInstances.Add(OverlayMID);
		OverlayMeshComponent->SetMaterial(MaterialIndex, OverlayMID);
	}
}

// Shows the overlay only for the intended player view, typically the locally controlled pawn.
void ULSPlayerXRayComponent::RefreshOverlayVisibility()
{
	if (OverlayMeshComponent == nullptr)
	{
		return;
	}

	bool bShouldShowOverlay = true;
	if (bOnlyLocallyControlled)
	{
		const APawn* OwnerPawn = Cast<APawn>(GetOwner());
		const APlayerController* PlayerController = OwnerPawn != nullptr ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
		bShouldShowOverlay = PlayerController != nullptr && PlayerController->IsLocalPlayerController();
	}

	if (bLastOverlayVisible == bShouldShowOverlay)
	{
		return;
	}

	OverlayMeshComponent->SetHiddenInGame(!bShouldShowOverlay);
	OverlayMeshComponent->SetVisibility(bShouldShowOverlay, true);
	bLastOverlayVisible = bShouldShowOverlay;
}

// Pushes the runtime XRay parameters into every overlay MID so the material can compare player depth vs scene depth.
void ULSPlayerXRayComponent::UpdateOverlayMaterialParameters()
{
	for (UMaterialInstanceDynamic* OverlayMID : OverlayMaterialInstances)
	{
		if (OverlayMID == nullptr)
		{
			continue;
		}

		OverlayMID->SetScalarParameterValue(DepthBiasParamName, DepthBias);
		OverlayMID->SetScalarParameterValue(XRayOpacityParamName, XRayOpacity);
		OverlayMID->SetVectorParameterValue(XRayColorParamName, XRayColor);
	}
}
