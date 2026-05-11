#include "Skills/Preview/LSSkillPreviewComponent.h"

#include "Components/DecalComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

ULSSkillPreviewComponent::ULSSkillPreviewComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool ULSSkillPreviewComponent::BeginAreaPreview(const FLSSkillAreaPreviewSpec& PreviewSpec)
{
	if (GetNetMode() == NM_DedicatedServer || !PreviewSpec.Material)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return false;
	}

	ActivePreviewSpec = PreviewSpec;

	if (!ActivePreviewDecal)
	{
		ActivePreviewDecal = NewObject<UDecalComponent>(OwnerActor);
		if (!ActivePreviewDecal)
		{
			return false;
		}

		ActivePreviewDecal->SetMobility(EComponentMobility::Movable);
		ActivePreviewDecal->SetFadeScreenSize(0.0f);
		ActivePreviewDecal->SetVisibility(true, true);
		ActivePreviewDecal->RegisterComponent();
	}

	ActivePreviewMaterial = UMaterialInstanceDynamic::Create(PreviewSpec.Material, this);
	if (!ActivePreviewMaterial)
	{
		EndAreaPreview();
		return false;
	}

	ActivePreviewDecal->SetDecalMaterial(ActivePreviewMaterial);
	ActivePreviewDecal->SetVisibility(true, true);
	ApplyDecalSize();
	ApplyMaterialParameters(ActivePreviewDecal->GetComponentRotation().Yaw);
	return true;
}

void ULSSkillPreviewComponent::UpdateAreaPreview(const FVector& WorldLocation, const FRotator& WorldRotation)
{
	if (!ActivePreviewDecal)
	{
		return;
	}

	FVector AdjustedLocation = WorldLocation;
	AdjustedLocation.Z += ActivePreviewSpec.WorldZOffset;

	const FRotator DecalRotation(-90.0f, WorldRotation.Yaw, 0.0f);
	ActivePreviewDecal->SetWorldLocationAndRotation(AdjustedLocation, DecalRotation);
	ApplyMaterialParameters(WorldRotation.Yaw);
}

void ULSSkillPreviewComponent::EndAreaPreview()
{
	if (ActivePreviewDecal)
	{
		ActivePreviewDecal->DestroyComponent();
		ActivePreviewDecal = nullptr;
	}

	ActivePreviewMaterial = nullptr;
}

void ULSSkillPreviewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndAreaPreview();

	Super::EndPlay(EndPlayReason);
}

void ULSSkillPreviewComponent::ApplyDecalSize()
{
	if (!ActivePreviewDecal)
	{
		return;
	}

	const float ProjectionDepth = FMath::Max(ActivePreviewSpec.DecalProjectionDepth, 1.0f);
	if (ActivePreviewSpec.Shape == ELSSkillAreaShape::Circle)
	{
		ActivePreviewDecal->DecalSize = FVector(ProjectionDepth, ActivePreviewSpec.Radius, ActivePreviewSpec.Radius);
		return;
	}

	ActivePreviewDecal->DecalSize = FVector(
		ProjectionDepth,
		ActivePreviewSpec.BoxWidth,
		ActivePreviewSpec.BoxLength);
}

void ULSSkillPreviewComponent::ApplyMaterialParameters(float WorldYaw)
{
	if (!ActivePreviewMaterial)
	{
		return;
	}

	if (ActivePreviewSpec.Shape == ELSSkillAreaShape::Circle)
	{
		ActivePreviewMaterial->SetScalarParameterValue(TEXT("Degrees"), ActivePreviewSpec.Degrees);
		ActivePreviewMaterial->SetScalarParameterValue(TEXT("Fade Intensity"), ActivePreviewSpec.FadeIntensity);
		ActivePreviewMaterial->SetScalarParameterValue(TEXT("Outer Radius"), ActivePreviewSpec.FillAmount);
		ActivePreviewMaterial->SetScalarParameterValue(TEXT("InnerRadius"), ActivePreviewSpec.InnerRadius);
		ActivePreviewMaterial->SetScalarParameterValue(TEXT("Rotation"), WorldYaw + ActivePreviewSpec.RotationOffsetDegrees);
		return;
	}

	ActivePreviewMaterial->SetScalarParameterValue(TEXT("Fill Amount"), ActivePreviewSpec.FillAmount);
	ActivePreviewMaterial->SetScalarParameterValue(TEXT("Fade Intensity"), ActivePreviewSpec.FadeIntensity);
	ActivePreviewMaterial->SetScalarParameterValue(TEXT("Outline Thickness"), ActivePreviewSpec.OutlineThickness);
}
