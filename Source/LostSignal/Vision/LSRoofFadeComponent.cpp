#include "Vision/LSRoofFadeComponent.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Vision/LSVisionSettings.h"

ULSRoofFadeComponent::ULSRoofFadeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	bTickInEditor = true;
}

// Ensures the optional trigger volume exists before play so editor-created instances stay in sync with their source mesh.
void ULSRoofFadeComponent::OnRegister()
{
	Super::OnRegister();

	EnsureTriggerVolume();
	UpdateTriggerVolumeFromSourceMesh();
}

#if WITH_EDITOR
// Keeps the auto trigger box synced when designers tweak scale, padding, or target mesh references in the editor.
void ULSRoofFadeComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	EnsureTriggerVolume();
	UpdateTriggerVolumeFromSourceMesh();
}
#endif

// Creates MIDs for the target roof meshes so cylinder fade parameters can be updated at runtime.
void ULSRoofFadeComponent::BeginPlay()
{
	Super::BeginPlay();

	EnsureTriggerVolume();
	UpdateTriggerVolumeFromSourceMesh();
	CreateShadowProxyMeshes();
	InitializeFadeMaterials();
	ApplyCustomDepthStencil();

	if (GetWorld() != nullptr && GetWorld()->IsNetMode(NM_DedicatedServer))
	{
		SetComponentTickEnabled(false);
	}
}

// Resets the fade so destroyed roof actors do not leave stale enabled parameters behind.
void ULSRoofFadeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ApplyFadeParameters(0.0f, FVector::ZeroVector);

	for (UStaticMeshComponent* ShadowProxyMeshComponent : ShadowProxyMeshComponents)
	{
		if (ShadowProxyMeshComponent != nullptr)
		{
			ShadowProxyMeshComponent->DestroyComponent();
		}
	}

	ShadowProxyMeshComponents.Reset();

	for (int32 SourceIndex = 0; SourceIndex < ShadowSourceMeshComponents.Num(); ++SourceIndex)
	{
		UStaticMeshComponent* SourceMeshComponent = ShadowSourceMeshComponents[SourceIndex].Get();
		if (SourceMeshComponent != nullptr && ShadowSourceCastShadowStates.IsValidIndex(SourceIndex))
		{
			SourceMeshComponent->SetCastShadow(ShadowSourceCastShadowStates[SourceIndex]);
		}
	}

	ShadowSourceMeshComponents.Reset();
	ShadowSourceCastShadowStates.Reset();

	Super::EndPlay(EndPlayReason);
}

// Updates the cylinder center from the local player and writes the latest mask parameters into every MID.
void ULSRoofFadeComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDrawDebugTriggerVolume)
	{
		DrawDebugTriggerVolume();
	}

	if (RoofFadeMaterialInstances.IsEmpty())
	{
		return;
	}

	APawn* PlayerPawn = ResolveLocalPlayerPawn();
	if (PlayerPawn == nullptr)
	{
		ApplyFadeParameters(0.0f, FVector::ZeroVector);
		return;
	}

	if (bUseTriggerVolume && TriggerVolume != nullptr && !TriggerVolume->IsOverlappingActor(PlayerPawn))
	{
		ApplyFadeParameters(0.0f, FVector::ZeroVector);
		return;
	}

	ApplyFadeParameters(1.0f, PlayerPawn->GetActorLocation() + FadeCenterOffset);
}

// Collects explicit target primitives when provided, otherwise falls back to every mesh component on the owner.
void ULSRoofFadeComponent::GatherTargetMeshComponents(TArray<UMeshComponent*>& OutMeshComponents) const
{
	OutMeshComponents.Reset();

	if (TargetPrimitives.Num() > 0)
	{
		for (UPrimitiveComponent* Primitive : TargetPrimitives)
		{
			if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(Primitive))
			{
				if (!ShadowProxyMeshComponents.Contains(Cast<UStaticMeshComponent>(MeshComponent)))
				{
					OutMeshComponents.AddUnique(MeshComponent);
				}
			}
		}

		return;
	}

	if (const AActor* OwnerActor = GetOwner())
	{
		TArray<UMeshComponent*> OwnerMeshComponents;
		OwnerActor->GetComponents<UMeshComponent>(OwnerMeshComponents);
		for (UMeshComponent* MeshComponent : OwnerMeshComponents)
		{
			if (!ShadowProxyMeshComponents.Contains(Cast<UStaticMeshComponent>(MeshComponent)))
			{
				OutMeshComponents.AddUnique(MeshComponent);
			}
		}
	}
}

// Resolves the first static mesh component that represents the roof geometry this component should size itself against.
UStaticMeshComponent* ULSRoofFadeComponent::ResolvePrimaryStaticMeshComponent() const
{
	if (TargetPrimitives.Num() > 0)
	{
		for (UPrimitiveComponent* Primitive : TargetPrimitives)
		{
			UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Primitive);
			if (StaticMeshComponent != nullptr && StaticMeshComponent->GetStaticMesh() != nullptr)
			{
				return StaticMeshComponent;
			}
		}
	}

	if (const AActor* OwnerActor = GetOwner())
	{
		TArray<UStaticMeshComponent*> StaticMeshComponents;
		OwnerActor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);

		for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
		{
			if (StaticMeshComponent != nullptr && StaticMeshComponent->GetStaticMesh() != nullptr && !ShadowProxyMeshComponents.Contains(StaticMeshComponent))
			{
				return StaticMeshComponent;
			}
		}
	}

	return nullptr;
}

// Creates a query-only trigger box once so roof fading can be gated by an engine overlap check instead of manual math.
void ULSRoofFadeComponent::EnsureTriggerVolume()
{
	if (!bUseTriggerVolume || !bAutoCreateTriggerVolume || GetOwner() == nullptr || TriggerVolume != nullptr)
	{
		return;
	}

	UStaticMeshComponent* SourceStaticMeshComponent = ResolvePrimaryStaticMeshComponent();
	USceneComponent* AttachParent = SourceStaticMeshComponent != nullptr ? static_cast<USceneComponent*>(SourceStaticMeshComponent) : GetOwner()->GetRootComponent();
	if (AttachParent == nullptr)
	{
		return;
	}

	TriggerVolume = NewObject<UBoxComponent>(
		GetOwner(),
		UBoxComponent::StaticClass(),
		TEXT("RoofFadeTriggerVolume"),
		RF_Transactional);

	if (TriggerVolume == nullptr)
	{
		return;
	}

	GetOwner()->AddInstanceComponent(TriggerVolume);
	TriggerVolume->SetupAttachment(AttachParent);
	TriggerVolume->SetRelativeTransform(FTransform::Identity);
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->SetCanEverAffectNavigation(false);
	TriggerVolume->SetHiddenInGame(true);
	TriggerVolume->OnComponentCreated();
	TriggerVolume->RegisterComponent();
}

// Sizes the trigger box from the source static mesh local bounds, then applies offset, scale, and padding adjustments.
void ULSRoofFadeComponent::UpdateTriggerVolumeFromSourceMesh()
{
	if (!bUseTriggerVolume || TriggerVolume == nullptr)
	{
		return;
	}

	UStaticMeshComponent* SourceStaticMeshComponent = ResolvePrimaryStaticMeshComponent();
	if (SourceStaticMeshComponent == nullptr)
	{
		return;
	}

	if (TriggerVolume->GetAttachParent() != SourceStaticMeshComponent)
	{
		TriggerVolume->AttachToComponent(SourceStaticMeshComponent, FAttachmentTransformRules::KeepWorldTransform);
	}

	FVector LocalBoundsMin = FVector::ZeroVector;
	FVector LocalBoundsMax = FVector::ZeroVector;
	SourceStaticMeshComponent->GetLocalBounds(LocalBoundsMin, LocalBoundsMax);

	const FVector BaseExtent = (LocalBoundsMax - LocalBoundsMin) * 0.5f;
	const FVector SafeScale(
		FMath::Max(TriggerExtentScale.X, 0.0f),
		FMath::Max(TriggerExtentScale.Y, 0.0f),
		FMath::Max(TriggerExtentScale.Z, 0.0f));
	const FVector SafePadding(
		FMath::Max(TriggerExtentPadding.X, 0.0f),
		FMath::Max(TriggerExtentPadding.Y, 0.0f),
		FMath::Max(TriggerExtentPadding.Z, 0.0f));
	const FVector FinalExtent = FVector(
		FMath::Max((BaseExtent.X * SafeScale.X) + SafePadding.X, 1.0f),
		FMath::Max((BaseExtent.Y * SafeScale.Y) + SafePadding.Y, 1.0f),
		FMath::Max((BaseExtent.Z * SafeScale.Z) + SafePadding.Z, 1.0f));

	const FVector DesiredWorldCenter(
		SourceStaticMeshComponent->Bounds.Origin.X,
		SourceStaticMeshComponent->Bounds.Origin.Y,
		TriggerGroundZ + FinalExtent.Z);
	const FVector RelativeCenter = SourceStaticMeshComponent->GetComponentTransform().InverseTransformPosition(DesiredWorldCenter);

	TriggerVolume->SetRelativeLocation(RelativeCenter + TriggerCenterOffset);
	TriggerVolume->SetRelativeRotation(FRotator::ZeroRotator);
	TriggerVolume->SetBoxExtent(FinalExtent);
}

// Shows the current trigger volume in editor/game so designers can verify the overlap region without selecting the box component.
void ULSRoofFadeComponent::DrawDebugTriggerVolume() const
{
	if (GetWorld() == nullptr || TriggerVolume == nullptr)
	{
		return;
	}

	DrawDebugBox(
		GetWorld(),
		TriggerVolume->GetComponentLocation(),
		TriggerVolume->GetScaledBoxExtent(),
		TriggerVolume->GetComponentQuat(),
		TriggerDebugColor,
		false,
		0.0f,
		0,
		TriggerDebugThickness);
}

// Creates hidden static-mesh duplicates that only cast shadows, letting the visible roof keep its fade/discard material.
void ULSRoofFadeComponent::CreateShadowProxyMeshes()
{
	ShadowProxyMeshComponents.Reset();
	ShadowSourceMeshComponents.Reset();
	ShadowSourceCastShadowStates.Reset();

	if (!bCreateShadowProxyMesh || GetOwner() == nullptr)
	{
		return;
	}

	TArray<UMeshComponent*> MeshComponents;
	GatherTargetMeshComponents(MeshComponents);

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		UStaticMeshComponent* SourceStaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent);
		if (SourceStaticMeshComponent == nullptr || SourceStaticMeshComponent->GetStaticMesh() == nullptr)
		{
			continue;
		}

		ShadowSourceMeshComponents.Add(SourceStaticMeshComponent);
		ShadowSourceCastShadowStates.Add(SourceStaticMeshComponent->CastShadow);

		UStaticMeshComponent* ShadowProxyMeshComponent = NewObject<UStaticMeshComponent>(
			GetOwner(),
			UStaticMeshComponent::StaticClass(),
			MakeUniqueObjectName(GetOwner(), UStaticMeshComponent::StaticClass(), FName(*FString::Printf(TEXT("%s_ShadowProxy"), *SourceStaticMeshComponent->GetName()))),
			RF_Transient);

		if (ShadowProxyMeshComponent == nullptr)
		{
			continue;
		}

		GetOwner()->AddInstanceComponent(ShadowProxyMeshComponent);
		ShadowProxyMeshComponent->SetupAttachment(SourceStaticMeshComponent);
		ShadowProxyMeshComponent->SetRelativeTransform(FTransform::Identity);
		ShadowProxyMeshComponent->SetMobility(SourceStaticMeshComponent->Mobility);
		ShadowProxyMeshComponent->SetStaticMesh(SourceStaticMeshComponent->GetStaticMesh());
		ShadowProxyMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ShadowProxyMeshComponent->SetGenerateOverlapEvents(false);
		ShadowProxyMeshComponent->SetCanEverAffectNavigation(false);
		ShadowProxyMeshComponent->SetRenderInMainPass(false);
		ShadowProxyMeshComponent->SetRenderInDepthPass(false);
		ShadowProxyMeshComponent->SetRenderCustomDepth(false);
		ShadowProxyMeshComponent->SetCastShadow(SourceStaticMeshComponent->CastShadow);
		ShadowProxyMeshComponent->bCastHiddenShadow = true;
		ShadowProxyMeshComponent->SetHiddenInGame(true);
		ShadowProxyMeshComponent->SetVisibility(false, true);
		ShadowProxyMeshComponent->bReceivesDecals = false;

		const int32 MaterialCount = SourceStaticMeshComponent->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			UMaterialInterface* ProxyMaterial = ResolveShadowProxyMaterial();
			if (ProxyMaterial == nullptr && bUseSourceMaterialsForShadowProxy)
			{
				ProxyMaterial = SourceStaticMeshComponent->GetMaterial(MaterialIndex);
			}

			if (ProxyMaterial != nullptr)
			{
				ShadowProxyMeshComponent->SetMaterial(MaterialIndex, ProxyMaterial);
			}
		}

		SourceStaticMeshComponent->SetCastShadow(false);
		ShadowProxyMeshComponent->RegisterComponent();
		ShadowProxyMeshComponents.Add(ShadowProxyMeshComponent);
	}
}

// Uses the explicit override first, then falls back to the project-wide vision settings asset reference.
UMaterialInterface* ULSRoofFadeComponent::ResolveShadowProxyMaterial() const
{
	if (ShadowProxyMaterialOverride != nullptr)
	{
		return ShadowProxyMaterialOverride;
	}

	const ULSVisionSettings* VisionSettings = GetDefault<ULSVisionSettings>();
	if (VisionSettings == nullptr || VisionSettings->DefaultShadowProxyMaterial.IsNull())
	{
		return nullptr;
	}

	return VisionSettings->DefaultShadowProxyMaterial.LoadSynchronous();
}

// Replaces source materials with MIDs so the roof can evaluate a player-centered cylinder mask in material code.
void ULSRoofFadeComponent::InitializeFadeMaterials()
{
	RoofFadeMaterialInstances.Reset();

	TArray<UMeshComponent*> MeshComponents;
	GatherTargetMeshComponents(MeshComponents);

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (MeshComponent == nullptr)
		{
			continue;
		}

		const int32 MaterialCount = MeshComponent->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			UMaterialInterface* SourceMaterial = RoofFadeMaterialOverride;
			if (SourceMaterial == nullptr && bUseExistingMaterials)
			{
				SourceMaterial = MeshComponent->GetMaterial(MaterialIndex);
			}

			if (SourceMaterial == nullptr)
			{
				continue;
			}

			UMaterialInstanceDynamic* RoofFadeMID = UMaterialInstanceDynamic::Create(SourceMaterial, this);
			if (RoofFadeMID == nullptr)
			{
				continue;
			}

			RoofFadeMaterialInstances.Add(RoofFadeMID);
			MeshComponent->SetMaterial(MaterialIndex, RoofFadeMID);
		}
	}
}

// Enables custom depth/stencil on the configured roof primitives so roof-specific effects can filter against stencil 10.
void ULSRoofFadeComponent::ApplyCustomDepthStencil() const
{
	TArray<UMeshComponent*> MeshComponents;
	GatherTargetMeshComponents(MeshComponents);

	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (MeshComponent == nullptr)
		{
			continue;
		}

		MeshComponent->SetRenderCustomDepth(true);
		MeshComponent->SetCustomDepthStencilValue(StencilValue);
	}
}

// Resolves the first local player pawn so every client can drive its own roof fade without sharing state.
APawn* ULSRoofFadeComponent::ResolveLocalPlayerPawn() const
{
	if (GetWorld() == nullptr)
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (PlayerController == nullptr || !PlayerController->IsLocalPlayerController())
		{
			continue;
		}

		APawn* PlayerPawn = PlayerController->GetPawn();
		if (PlayerPawn == nullptr)
		{
			continue;
		}

		return PlayerPawn;
	}

	return nullptr;
}

// Writes the enabled flag plus cylinder dimensions so the material can build a smooth discard mask around the player.
void ULSRoofFadeComponent::ApplyFadeParameters(const float EnabledValue, const FVector& FadeCenterWS) const
{
	for (UMaterialInstanceDynamic* RoofFadeMID : RoofFadeMaterialInstances)
	{
		if (RoofFadeMID == nullptr)
		{
			continue;
		}

		RoofFadeMID->SetScalarParameterValue(FadeEnabledParamName, EnabledValue);
		RoofFadeMID->SetVectorParameterValue(FadeCenterParamName, FLinearColor(FadeCenterWS.X, FadeCenterWS.Y, FadeCenterWS.Z, 1.0f));
		RoofFadeMID->SetScalarParameterValue(FadeRadiusParamName, FadeRadius);
		RoofFadeMID->SetScalarParameterValue(FadeWidthParamName, FadeWidth);
		RoofFadeMID->SetScalarParameterValue(FadeHalfHeightParamName, FadeHalfHeight);
		RoofFadeMID->SetScalarParameterValue(HeightFadeWidthParamName, HeightFadeWidth);
	}
}
