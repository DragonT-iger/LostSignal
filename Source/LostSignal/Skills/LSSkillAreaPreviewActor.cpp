#include "Skills/LSSkillAreaPreviewActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

ALSSkillAreaPreviewActor::ALSSkillAreaPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	AreaMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AreaMesh"));
	AreaMeshComponent->SetupAttachment(SceneRoot);
	AreaMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AreaMeshComponent->SetGenerateOverlapEvents(false);
	AreaMeshComponent->SetCastShadow(false);
	AreaMeshComponent->bReceivesDecals = false;
}

void ALSSkillAreaPreviewActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyAreaSpec();
}

void ALSSkillAreaPreviewActor::SetAreaSpec(const FLSSkillAreaPreviewSpec& InSpec)
{
	AreaSpec = InSpec;
	ApplyAreaSpec();
}

void ALSSkillAreaPreviewActor::SetPreviewTransform(const FVector& WorldLocation, const FRotator& WorldRotation)
{
	FVector AdjustedLocation = WorldLocation;
	AdjustedLocation.Z += AreaSpec.WorldZOffset;

	SetActorLocationAndRotation(AdjustedLocation, WorldRotation);
	ApplyMaterialParameters();
}

void ALSSkillAreaPreviewActor::SetPreviewVisible(bool bVisible)
{
	SetActorHiddenInGame(!bVisible);

	if (AreaMeshComponent)
	{
		AreaMeshComponent->SetVisibility(bVisible, true);
	}
}

void ALSSkillAreaPreviewActor::ApplyAreaSpec()
{
	if (!AreaMeshComponent)
	{
		return;
	}

	EnsureMaterialInstance();
	ApplyMeshScale();
	ApplyMaterialParameters();
}

void ALSSkillAreaPreviewActor::ApplyMeshScale()
{
	if (!AreaMeshComponent)
	{
		return;
	}

	const float SafeMeshBaseSize = FMath::Max(MeshBaseSize, 1.0f);
	float DesiredLength = 0.0f;
	float DesiredWidth = 0.0f;

	if (AreaSpec.Shape == ELSSkillAreaShape::Circle)
	{
		const float Diameter = AreaSpec.Radius * 2.0f;
		DesiredLength = Diameter;
		DesiredWidth = Diameter;
	}
	else
	{
		DesiredLength = AreaSpec.BoxLength;
		DesiredWidth = AreaSpec.BoxWidth;
	}

	AreaMeshComponent->SetRelativeScale3D(FVector(
		DesiredLength / SafeMeshBaseSize,
		DesiredWidth / SafeMeshBaseSize,
		1.0f));
}

void ALSSkillAreaPreviewActor::ApplyMaterialParameters()
{
	if (!PreviewMaterialInstance)
	{
		return;
	}

	if (AreaSpec.Shape == ELSSkillAreaShape::Circle)
	{
		PreviewMaterialInstance->SetScalarParameterValue(TEXT("Degrees"), AreaSpec.Degrees);
		PreviewMaterialInstance->SetScalarParameterValue(TEXT("Fade Intensity"), AreaSpec.FadeIntensity);
		PreviewMaterialInstance->SetScalarParameterValue(TEXT("Outer Radius"), AreaSpec.FillAmount);
		PreviewMaterialInstance->SetScalarParameterValue(TEXT("InnerRadius"), AreaSpec.InnerRadius);
		PreviewMaterialInstance->SetScalarParameterValue(TEXT("Rotation"), GetActorRotation().Yaw + AreaSpec.RotationOffsetDegrees);
		return;
	}

	PreviewMaterialInstance->SetScalarParameterValue(TEXT("Fill Amount"), AreaSpec.FillAmount);
	PreviewMaterialInstance->SetScalarParameterValue(TEXT("Fade Intensity"), AreaSpec.FadeIntensity);
	PreviewMaterialInstance->SetScalarParameterValue(TEXT("Outline Thickness"), AreaSpec.OutlineThickness);
}

void ALSSkillAreaPreviewActor::EnsureMaterialInstance()
{
	if (!AreaMeshComponent)
	{
		return;
	}

	UMaterialInterface* SourceMaterial = AreaSpec.Material ? AreaSpec.Material.Get() : AreaMeshComponent->GetMaterial(0);
	if (!SourceMaterial)
	{
		PreviewMaterialInstance = nullptr;
		return;
	}

	if (PreviewMaterialInstance && PreviewMaterialInstance->Parent == SourceMaterial)
	{
		return;
	}

	PreviewMaterialInstance = UMaterialInstanceDynamic::Create(SourceMaterial, this);
	AreaMeshComponent->SetMaterial(0, PreviewMaterialInstance);
}
