#include "Skills/Preview/LSSkillPreviewComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Vision/LSVisionTypes.h"

ULSSkillPreviewComponent::ULSSkillPreviewComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMeshFinder.Succeeded())
	{
		DefaultPreviewMesh = PlaneMeshFinder.Object;
	}
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

	if (!ActivePreviewMesh)
	{
		ActivePreviewMesh = NewObject<UStaticMeshComponent>(OwnerActor);
		if (!ActivePreviewMesh)
		{
			return false;
		}

		ActivePreviewMesh->SetMobility(EComponentMobility::Movable);
		ActivePreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ActivePreviewMesh->SetGenerateOverlapEvents(false);
		ActivePreviewMesh->SetCastShadow(false);
		ActivePreviewMesh->SetVisibility(true, true);
		// 시전자가 시야 밖이어도 위험 범위 표시는 유지 — VisionTarget의 프리미티브 숨김에서 제외.
		ActivePreviewMesh->ComponentTags.Add(LSVisionTags::HideExempt);
		ActivePreviewMesh->RegisterComponent();
	}

	ActivePreviewMesh->SetStaticMesh(DefaultPreviewMesh);
	if (!ActivePreviewMesh->GetStaticMesh())
	{
		EndAreaPreview();
		return false;
	}

	ActivePreviewMaterial = UMaterialInstanceDynamic::Create(PreviewSpec.Material, this);
	if (!ActivePreviewMaterial)
	{
		EndAreaPreview();
		return false;
	}

	ActivePreviewMesh->SetMaterial(0, ActivePreviewMaterial);
	ActivePreviewMesh->SetVisibility(true, true);
	ApplyMeshScale();
	ApplyMaterialParameters(ActivePreviewMesh->GetComponentRotation().Yaw);
	return true;
}

void ULSSkillPreviewComponent::UpdateAreaPreview(const FVector& WorldLocation, const FRotator& WorldRotation)
{
	if (!ActivePreviewMesh)
	{
		return;
	}

	FVector AdjustedLocation = WorldLocation;
	AdjustedLocation.Z = ResolveOwnerFootZ(WorldLocation.Z) + ActivePreviewSpec.WorldZOffset;

	const float MeshYaw = ActivePreviewSpec.Shape == ELSSkillAreaShape::Box ? WorldRotation.Yaw : 0.0f;
	const FRotator MeshRotation(0.0f, MeshYaw, 0.0f);
	ActivePreviewMesh->SetWorldLocationAndRotation(AdjustedLocation, MeshRotation);
	ApplyMaterialParameters(WorldRotation.Yaw);
}

void ULSSkillPreviewComponent::SetAreaFillAmount(float NewFillAmount)
{
	ActivePreviewSpec.FillAmount = NewFillAmount;
	if (!ActivePreviewMaterial)
	{
		return;
	}

	// 모양별 fill 파라미터만 갱신(ApplyMaterialParameters와 동일 매핑). 다른 파라미터·트랜스폼은 보존.
	const FName FillParameterName = ActivePreviewSpec.Shape == ELSSkillAreaShape::Circle
		? FName(TEXT("Outer Radius"))
		: FName(TEXT("Fill Amount"));
	ActivePreviewMaterial->SetScalarParameterValue(FillParameterName, NewFillAmount);
}

void ULSSkillPreviewComponent::EndAreaPreview()
{
	if (ActivePreviewMesh)
	{
		ActivePreviewMesh->DestroyComponent();
		ActivePreviewMesh = nullptr;
	}

	ActivePreviewMaterial = nullptr;
}

void ULSSkillPreviewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndAreaPreview();

	Super::EndPlay(EndPlayReason);
}

void ULSSkillPreviewComponent::ApplyMeshScale()
{
	if (!ActivePreviewMesh)
	{
		return;
	}

	constexpr float DefaultPlaneSize = 100.0f;
	if (ActivePreviewSpec.Shape == ELSSkillAreaShape::Circle)
	{
		const float Diameter = FMath::Max(ActivePreviewSpec.Radius * 2.0f, 1.0f);
		ActivePreviewMesh->SetWorldScale3D(FVector(Diameter / DefaultPlaneSize, Diameter / DefaultPlaneSize, 1.0f));
		return;
	}

	ActivePreviewMesh->SetWorldScale3D(FVector(
		FMath::Max(ActivePreviewSpec.BoxLength, 1.0f) / DefaultPlaneSize,
		FMath::Max(ActivePreviewSpec.BoxWidth, 1.0f) / DefaultPlaneSize,
		1.0f));
}

void ULSSkillPreviewComponent::ApplyMaterialParameters(float WorldYaw)
{
	if (!ActivePreviewMaterial)
	{
		return;
	}

	if (ActivePreviewSpec.Shape == ELSSkillAreaShape::Circle)
	{
		const float RotationTurns = FRotator::ClampAxis(WorldYaw + ActivePreviewSpec.RotationOffsetDegrees) / 360.0f;
		ActivePreviewMaterial->SetScalarParameterValue(TEXT("Degrees"), ActivePreviewSpec.Degrees);
		ActivePreviewMaterial->SetScalarParameterValue(TEXT("Fade Intensity"), ActivePreviewSpec.FadeIntensity);
		ActivePreviewMaterial->SetScalarParameterValue(TEXT("Outer Radius"), ActivePreviewSpec.FillAmount);
		ActivePreviewMaterial->SetScalarParameterValue(TEXT("InnerRadius"), ActivePreviewSpec.InnerRadius);
		ActivePreviewMaterial->SetScalarParameterValue(TEXT("Rotation"), RotationTurns);
		return;
	}

	ActivePreviewMaterial->SetScalarParameterValue(TEXT("Fill Amount"), ActivePreviewSpec.FillAmount);
}

float ULSSkillPreviewComponent::ResolveOwnerFootZ(float FallbackZ) const
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	const UCapsuleComponent* CapsuleComponent = OwnerCharacter ? OwnerCharacter->GetCapsuleComponent() : nullptr;
	if (!OwnerCharacter || !CapsuleComponent)
	{
		return FallbackZ;
	}

	return OwnerCharacter->GetActorLocation().Z - CapsuleComponent->GetScaledCapsuleHalfHeight();
}
