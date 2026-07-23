#include "Vision/LSVisionGhostComponent.h"

#include "Combat/LSCharacterCombatComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/LSMonsterPresentationSettings.h"
#include "GameFramework/Character.h"
#include "LostSignal.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Vision/LSVisionTargetComponent.h"
#include "Vision/LSVisionTypes.h"

void ULSVisionGhostMeshComponent::GetDefaultMaterialSlotsOverlayMaterial(
	TArray<TObjectPtr<UMaterialInterface>>& OutMaterialSlotOverlayMaterials) const
{
	OutMaterialSlotOverlayMaterials.Reset();
}

UMaterialInterface* ULSVisionGhostMeshComponent::GetDefaultOverlayMaterial() const
{
	return nullptr;
}

ULSVisionGhostComponent::ULSVisionGhostComponent()
{
	// 잔상 페이드가 진행 중일 때만 틱을 켠다.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

// VisionTarget의 가시성 변경을 구독한다. 로컬 전용 비주얼이므로 데디케이티드 서버에서는 동작하지 않는다.
void ULSVisionGhostComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (const ULSMonsterPresentationSettings* Settings = GetDefault<ULSMonsterPresentationSettings>())
	{
		ResolvedGhostMaterial = Settings->GhostMaterial.LoadSynchronous();
	}

	ULSVisionTargetComponent* VisionTarget =
		GetOwner() != nullptr ? GetOwner()->FindComponentByClass<ULSVisionTargetComponent>() : nullptr;
	if (VisionTarget == nullptr)
	{
		UE_LOG(LogLS, Warning, TEXT("LSVisionGhostComponent on '%s' could not find a ULSVisionTargetComponent."), *GetNameSafe(GetOwner()));
		return;
	}

	VisibilityChangedHandle = VisionTarget->OnLocalVisibilityChanged.AddUObject(
		this, &ULSVisionGhostComponent::HandleLocalVisibilityChanged);
}

// 델리게이트 언바인드 후 런타임 생성한 잔상 메쉬를 정리한다.
void ULSVisionGhostComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (VisibilityChangedHandle.IsValid())
	{
		if (ULSVisionTargetComponent* VisionTarget =
			GetOwner() != nullptr ? GetOwner()->FindComponentByClass<ULSVisionTargetComponent>() : nullptr)
		{
			VisionTarget->OnLocalVisibilityChanged.Remove(VisibilityChangedHandle);
		}
		VisibilityChangedHandle.Reset();
	}

	if (GhostMeshComponent != nullptr)
	{
		GhostMeshComponent->DestroyComponent();
		GhostMeshComponent = nullptr;
	}

	GhostMaterialInstances.Reset();

	Super::EndPlay(EndPlayReason);
}

// 페이드 진행: 경과시간 기반 선형 보간으로 불투명도가 0에 도달하면 잔상을 정리하고 틱을 끈다.
void ULSVisionGhostComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bGhostActive)
	{
		SetComponentTickEnabled(false);
		return;
	}

	FadeElapsed += DeltaTime;

	const float Opacity = 1.0f - FMath::Clamp((FadeElapsed - FadeStartDelay) / FadeDuration, 0.0f, 1.0f);
	ApplyGhostOpacity(Opacity);

	if (Opacity <= 0.0f)
	{
		ClearGhostImmediate();
	}
}

USkeletalMeshComponent* ULSVisionGhostComponent::ResolveSourceMeshComponent() const
{
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		return OwnerCharacter->GetMesh();
	}

	return GetOwner() != nullptr ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

// 잔상 전용 PoseableMesh를 최초 필요 시 지연 생성한다.
// attach하지 않는 것이 핵심: VisionTarget의 SetVisibility(bVisible, true) 자식 전파에서 격리되고,
// 적 액터 이동에도 딸려가지 않는다. HideExempt 태그로 GatherRenderPrimitives 수집에서도 제외.
void ULSVisionGhostComponent::CreateGhostMeshComponent()
{
	if (GhostMeshComponent != nullptr)
	{
		return;
	}

	USkeletalMeshComponent* SourceMeshComponent = ResolveSourceMeshComponent();
	if (SourceMeshComponent == nullptr)
	{
		UE_LOG(LogLS, Warning, TEXT("LSVisionGhostComponent on '%s' could not find a source skeletal mesh."), *GetNameSafe(GetOwner()));
		return;
	}

	GhostMeshComponent = NewObject<ULSVisionGhostMeshComponent>(
		GetOwner(),
		ULSVisionGhostMeshComponent::StaticClass(),
		TEXT("VisionGhostMesh"),
		RF_Transient);

	if (GhostMeshComponent == nullptr)
	{
		UE_LOG(LogLS, Warning, TEXT("LSVisionGhostComponent on '%s' failed to allocate ghost mesh."), *GetNameSafe(GetOwner()));
		return;
	}

	GetOwner()->AddInstanceComponent(GhostMeshComponent);
	GhostMeshComponent->SetSkinnedAssetAndUpdate(SourceMeshComponent->GetSkeletalMeshAsset());
	GhostMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GhostMeshComponent->SetGenerateOverlapEvents(false);
	GhostMeshComponent->SetCanEverAffectNavigation(false);
	GhostMeshComponent->SetCastShadow(false);
	GhostMeshComponent->bReceivesDecals = false;
	GhostMeshComponent->ComponentTags.Add(LSVisionTags::HideExempt);
	GhostMeshComponent->SetVisibility(false, true);

	if (bEnableCustomDepth)
	{
		GhostMeshComponent->SetRenderCustomDepth(true);
		GhostMeshComponent->SetCustomDepthStencilValue(CustomDepthStencilValue);
	}

	GhostMeshComponent->RegisterComponent();

	CreateGhostMaterialInstances(SourceMeshComponent);
}

// 소스 메쉬의 슬롯 수만큼 잔상 MID를 만들어 컬러를 주입한다.
void ULSVisionGhostComponent::CreateGhostMaterialInstances(const USkeletalMeshComponent* SourceMeshComponent)
{
	GhostMaterialInstances.Reset();

	const int32 MaterialCount = FMath::Max(SourceMeshComponent->GetNumMaterials(), 1);
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		UMaterialInstanceDynamic* GhostMID = UMaterialInstanceDynamic::Create(ResolvedGhostMaterial, this);
		if (GhostMID == nullptr)
		{
			continue;
		}

		GhostMID->SetVectorParameterValue(GhostColorParamName, GhostColor);
		GhostMaterialInstances.Add(GhostMID);
		GhostMeshComponent->SetMaterial(MaterialIndex, GhostMID);
	}
}

void ULSVisionGhostComponent::HandleLocalVisibilityChanged(const bool bLocallyVisible)
{
	if (bLocallyVisible)
	{
		// 재진입: 본체는 VisionTarget이 같은 프레임에 켜므로 잔상만 즉시 제거한다.
		ClearGhostImmediate();
	}
	else
	{
		BeginGhostFade();
	}
}

// 시야 이탈 순간의 위치·포즈를 스냅샷해 잔상 페이드를 시작한다.
void ULSVisionGhostComponent::BeginGhostFade()
{
	USkeletalMeshComponent* SourceMeshComponent = ResolveSourceMeshComponent();
	if (SourceMeshComponent == nullptr)
	{
		return;
	}

	// 최근 실제 렌더된 적이 없으면(시작부터 시야 밖 등) 잔상 없이 기존 숨김만 적용한다.
	if (!SourceMeshComponent->WasRecentlyRendered(RecentlyRenderedTolerance))
	{
		return;
	}

	if (bSuppressWhenDead)
	{
		const ULSCharacterCombatComponent* CombatComponent = GetOwner()->FindComponentByClass<ULSCharacterCombatComponent>();
		if (CombatComponent != nullptr && CombatComponent->IsDead())
		{
			return;
		}
	}

	if (ResolvedGhostMaterial == nullptr)
	{
		if (!bWarnedMissingGhostMaterial)
		{
			UE_LOG(LogLS, Warning, TEXT("LSVisionGhostComponent on '%s' has no GhostMaterial in LS Monster Presentation Settings. Ghost silhouette disabled."), *GetNameSafe(GetOwner()));
			bWarnedMissingGhostMaterial = true;
		}
		return;
	}

	CreateGhostMeshComponent();
	if (GhostMeshComponent == nullptr)
	{
		return;
	}

	// 본체가 숨겨져도 본 트랜스폼은 마지막 렌더 값이 남아 있어 "사라지는 순간"의 포즈가 그대로 잡힌다.
	GhostMeshComponent->SetWorldTransform(SourceMeshComponent->GetComponentTransform());
	GhostMeshComponent->CopyPoseFromSkeletalComponent(SourceMeshComponent);
	ApplyGhostOpacity(1.0f);
	GhostMeshComponent->SetVisibility(true, true);

	FadeElapsed = 0.0f;
	bGhostActive = true;
	SetComponentTickEnabled(true);
}

void ULSVisionGhostComponent::ClearGhostImmediate()
{
	bGhostActive = false;
	SetComponentTickEnabled(false);

	if (GhostMeshComponent != nullptr)
	{
		GhostMeshComponent->SetVisibility(false, true);
	}
}

void ULSVisionGhostComponent::ApplyGhostOpacity(const float Opacity)
{
	for (UMaterialInstanceDynamic* GhostMID : GhostMaterialInstances)
	{
		if (GhostMID != nullptr)
		{
			GhostMID->SetScalarParameterValue(GhostOpacityParamName, Opacity);
		}
	}
}
